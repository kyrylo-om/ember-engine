module ember.core;

import ember.ecs;
import std;

namespace ember::core
{

Application::Application(ApplicationConfig config)
    : m_config{config}, m_fixed_timestep{config.fixed_step, config.max_fixed_steps}
{
    contract_assert(config.fixed_step > Seconds::zero());
    contract_assert(config.max_fixed_steps > 0);
    contract_assert(config.max_frame_time >= Seconds::zero());

    m_world.insert_resource(Time{.fixed_delta = config.fixed_step});
    m_world.insert_resource(EventBus{});
}

Application::~Application() = default;

Application& Application::add_system(Stage stage, std::string_view name, System system)
{
    m_scheduler.add_system(stage, name, std::move(system));
    return *this;
}

ecs::World& Application::world()
{
    return m_world;
}

const ecs::World& Application::world() const
{
    return m_world;
}

Scheduler& Application::scheduler()
{
    return m_scheduler;
}

void Application::run()
{
    start();

    auto frame_start = std::chrono::steady_clock::now();
    while (m_is_running)
    {
        const auto now = std::chrono::steady_clock::now();
        update(std::chrono::duration_cast<Seconds>(now - frame_start));
        frame_start = now;
    }

    m_scheduler.run_stage(Stage::Shutdown, m_world);
}

void Application::update(Seconds frame_time)
{
    start();

    // Events published during the previous frame, or during startup, become readable now.
    EventBus& events = m_world.resource<EventBus>();
    events.advance_frame();
    if (!events.read<ExitRequested>().empty())
    {
        request_exit();
    }

    // A clock that jumps back must not move the game back in time.
    const Seconds delta = std::clamp(frame_time, Seconds::zero(), m_config.max_frame_time);
    const int fixed_steps = m_fixed_timestep.advance(delta);

    Time& time = m_world.resource<Time>();
    time.delta = delta;
    time.fixed_delta = m_fixed_timestep.step();
    time.elapsed += delta;
    time.fixed_alpha = m_fixed_timestep.alpha();

    m_scheduler.run_stage(Stage::PreUpdate, m_world);
    for (int step = 0; step < fixed_steps; ++step)
    {
        m_scheduler.run_stage(Stage::FixedUpdate, m_world);
    }
    m_scheduler.run_stage(Stage::Update, m_world);
    m_scheduler.run_stage(Stage::PostUpdate, m_world);
    m_scheduler.run_stage(Stage::PreRender, m_world);
    m_scheduler.run_stage(Stage::Render, m_world);
    m_scheduler.run_stage(Stage::PostRender, m_world);

    // Looked up again, because a system may have replaced the resource.
    ++m_world.resource<Time>().frame;
}

void Application::request_exit()
{
    m_is_running = false;
}

bool Application::is_running() const
{
    return m_is_running;
}

void Application::start()
{
    if (m_has_started)
    {
        return;
    }
    m_has_started = true;
    m_scheduler.run_stage(Stage::Startup, m_world);
}

} // namespace ember::core

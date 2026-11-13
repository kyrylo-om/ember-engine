module ember.core;

import ember.ecs;
import std;

namespace ember::core
{

namespace
{

std::size_t stage_index(Stage stage)
{
    return static_cast<std::size_t>(stage);
}

} // namespace

Scheduler::Scheduler() = default;
Scheduler::~Scheduler() = default;

void Scheduler::add_system(Stage stage, std::string_view name, System system)
{
    contract_assert(system != nullptr);
    m_systems[stage_index(stage)].emplace_back(std::string{name}, std::move(system));
}

void Scheduler::run_stage(Stage stage, ecs::World& world)
{
    for (NamedSystem& entry : m_systems[stage_index(stage)])
    {
        try
        {
            entry.system(world);
        }
        catch (const std::exception& error)
        {
            log_error("System '{}' failed: {}", entry.name, error.what());
            throw;
        }
    }
    world.apply_commands();
}

std::size_t Scheduler::system_count(Stage stage) const
{
    return m_systems[stage_index(stage)].size();
}

} // namespace ember::core

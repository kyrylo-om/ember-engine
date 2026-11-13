module ember.particles;

import ember.common;
import ember.core;
import ember.ecs;
import ember.platform;
import std;

namespace ember::particles
{

namespace
{

// Live particles of one emitter and the state that emits them.
struct EmitterParticles
{
    ParticlePool pool;
    ParticleSpawner spawner;
};

// Particles of every emitter, stored as a world resource. They are kept outside the Emitter
// component, so the component stays plain data that can be saved in a scene.
struct ParticleStorage
{
    std::map<ecs::Entity, EmitterParticles> emitters{};
    // Gives every spawner its own seed. Seeds that differ only by one would make the first
    // particles of the emitters nearly the same.
    std::mt19937 seeds{};
};

void remove_particles_without_emitter(ParticleStorage& storage, const ecs::World& world)
{
    std::erase_if(storage.emitters,
                  [&world](const auto& entry)
                  {
                      const ecs::Entity entity = entry.first;
                      return !world.has<ecs::Emitter>(entity) || !world.has<ecs::Transform>(entity);
                  });
}

void simulate_particles(ecs::World& world)
{
    const ParticleSettings settings = world.resource<ParticleSettings>();
    const core::Seconds delta = world.resource<core::Time>().delta;
    ParticleStorage& storage = world.resource<ParticleStorage>();

    remove_particles_without_emitter(storage, world);

    for (auto [entity, transform, emitter] : world.view<const ecs::Transform, ecs::Emitter>())
    {
        auto found = storage.emitters.find(entity);
        if (found == storage.emitters.end())
        {
            const auto seed = static_cast<std::uint32_t>(storage.seeds());
            found = storage.emitters
                        .try_emplace(entity, ParticlePool{settings.max_particles_per_emitter},
                                     ParticleSpawner{seed})
                        .first;
        }
        EmitterParticles& particles = found->second;

        // Existing particles move before new ones are emitted, so a new particle is first
        // seen at the position of the emitter with age 0.
        if (settings.use_simd)
        {
            particles.pool.simulate_simd(delta, emitter.acceleration, emitter.lifetime);
        }
        else
        {
            particles.pool.simulate_scalar(delta, emitter.acceleration, emitter.lifetime);
        }
        particles.spawner.spawn(emitter, transform.position, delta, particles.pool);
    }
}

std::uint8_t mix(std::uint8_t start, std::uint8_t end, float progress)
{
    return static_cast<std::uint8_t>(
        std::lround(std::lerp(static_cast<float>(start), static_cast<float>(end), progress)));
}

Color mix(Color start, Color end, float progress)
{
    return {mix(start.r, end.r, progress), mix(start.g, end.g, progress),
            mix(start.b, end.b, progress), mix(start.a, end.a, progress)};
}

// Returns the view of the first camera in the world, or of a default camera, which shows the
// world origin in the center of the window.
platform::View camera_view(const ecs::World& world, Vec2 window_size)
{
    ecs::Camera camera{};
    for (auto [entity, found] : world.view<ecs::Camera>())
    {
        camera = found;
        break;
    }
    return {.target = camera.position,
            .offset = window_size / 2.0f,
            .zoom = camera.zoom,
            .rotation = camera.rotation};
}

void draw_particles(ecs::World& world)
{
    // Without the platform plugin, for example in tests and benchmarks, particles are only
    // simulated.
    platform::Graphics* graphics = world.try_resource<platform::Graphics>();
    const platform::Window* window = world.try_resource<platform::Window>();
    if (graphics == nullptr || window == nullptr)
    {
        return;
    }
    const ecs::World& const_world = world;

    graphics->begin_view(camera_view(const_world, window->size()));
    for (const auto& [entity, particles] : world.resource<ParticleStorage>().emitters)
    {
        const ecs::Emitter* emitter = const_world.try_get<ecs::Emitter>(entity);
        if (emitter == nullptr)
        {
            continue;
        }

        for (std::size_t index = 0; index < particles.pool.size(); ++index)
        {
            const Particle particle = particles.pool.particle(index);
            // How much of its life the particle has lived, from 0 to 1.
            const float progress = std::clamp(particle.age / emitter->lifetime, 0.0f, 1.0f);
            const float size = std::lerp(emitter->start_size, emitter->end_size, progress);
            if (size <= 0.0f)
            {
                continue;
            }

            // A square centered on the particle; squares are much cheaper to draw than
            // circles and look the same at the size of a particle.
            const Vec2 square{size, size};
            graphics->draw_rect({.position = particle.position - square / 2.0f, .size = square},
                                mix(emitter->start_color, emitter->end_color, progress));
        }
    }
    graphics->end_view();
}

} // namespace

void ParticlePlugin::build(core::Application& app) const
{
    app.world().insert_resource(settings);
    app.world().insert_resource(ParticleStorage{});
    app.add_system(core::Stage::Update, "simulate_particles", simulate_particles);
    app.add_system(core::Stage::Render, "draw_particles", draw_particles);
}

std::size_t particle_count(const ecs::World& world)
{
    const ParticleStorage* storage = world.try_resource<ParticleStorage>();
    if (storage == nullptr)
    {
        return 0;
    }
    return std::ranges::fold_left(storage->emitters | std::views::values, std::size_t{0},
                                  [](std::size_t count, const EmitterParticles& particles)
                                  { return count + particles.pool.size(); });
}

} // namespace ember::particles

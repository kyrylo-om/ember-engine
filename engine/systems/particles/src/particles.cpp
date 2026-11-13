module ember.particles;

import ember.core;
import ember.ecs;
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

} // namespace

void ParticlePlugin::build(core::Application& app) const
{
    app.world().insert_resource(settings);
    app.world().insert_resource(ParticleStorage{});
    app.add_system(core::Stage::Update, "simulate_particles", simulate_particles);
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

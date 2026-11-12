module ember.ecs;

import std;

namespace ember::ecs
{

Commands::Commands(World& world) : m_world{&world}
{
}

Commands::~Commands() = default;

Entity Commands::create()
{
    const Entity entity = m_world->reserve_entity();
    m_operations.emplace_back([entity](World& world) { world.activate(entity); });
    return entity;
}

void Commands::destroy(Entity entity)
{
    m_operations.emplace_back([entity](World& world) { world.destroy(entity); });
}

} // namespace ember::ecs

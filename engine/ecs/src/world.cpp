module ember.ecs;

import std;

namespace
{

// Generation 0 is never given to an entity, so the null entity is never alive.
constexpr std::uint32_t first_generation = 1;

} // namespace

namespace ember::ecs
{

World::World() : m_commands{*this}
{
}

World::~World()
{
    // A vector does not promise the order in which it destroys its elements.
    while (!m_resources.empty())
    {
        m_resources.pop_back();
    }
}

Entity World::create()
{
    const Entity entity = reserve_entity();
    activate(entity);
    return entity;
}

void World::destroy(Entity entity)
{
    if (!is_alive(entity))
    {
        return;
    }

    for (const auto& set : m_sets | std::views::values)
    {
        set->remove(entity);
    }
    m_is_alive[entity.index] = false;
    // Makes every copy of the entity stale before the index is given out again.
    ++m_generations[entity.index];
    m_free_indices.push_back(entity.index);
}

bool World::is_alive(Entity entity) const
{
    return entity.index < m_generations.size() &&
           m_generations[entity.index] == entity.generation && m_is_alive[entity.index];
}

Commands& World::commands()
{
    return m_commands;
}

void World::apply_commands()
{
    for (auto& operation : m_commands.m_operations)
    {
        operation(*this);
    }
    m_commands.m_operations.clear();
}

Entity World::reserve_entity()
{
    if (!m_free_indices.empty())
    {
        const std::uint32_t index = m_free_indices.back();
        m_free_indices.pop_back();
        return Entity{index, m_generations[index]};
    }

    const auto index = static_cast<std::uint32_t>(m_generations.size());
    m_generations.push_back(first_generation);
    m_is_alive.push_back(false);
    return Entity{index, first_generation};
}

void World::activate(Entity entity)
{
    m_is_alive[entity.index] = true;
}

} // namespace ember::ecs

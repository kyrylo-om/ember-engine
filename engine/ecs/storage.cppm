// Containers behind World and View: a sparse set per component type and the holders of the
// resources. An interface partition, because World and View have members of these types, but
// it exports no names.
export module ember.ecs:storage;

import :entity;
import std;

namespace ember::ecs
{

// Components of one type behind a base without that type, so the world keeps all sets in one
// map and can remove an entity from every set.
class SparseSetBase
{
public:
    SparseSetBase() = default;
    virtual ~SparseSetBase() = default;

    SparseSetBase(const SparseSetBase&) = delete;
    SparseSetBase& operator=(const SparseSetBase&) = delete;
    SparseSetBase(SparseSetBase&&) = delete;
    SparseSetBase& operator=(SparseSetBase&&) = delete;

    // Compares the whole entity, so a stale entity is not found after its index is reused.
    bool contains(Entity entity) const
    {
        return entity.index < m_sparse.size() && m_sparse[entity.index] != empty_slot &&
               m_entities[m_sparse[entity.index]] == entity;
    }

    // Entities that have a component, in the order of the components.
    std::span<const Entity> entities() const
    {
        return m_entities;
    }

    virtual void remove(Entity entity) = 0;

protected:
    static constexpr std::uint32_t empty_slot = std::numeric_limits<std::uint32_t>::max();

    // Position of the component in the dense arrays for every entity index, or empty_slot.
    std::vector<std::uint32_t> m_sparse{};
    // Owner of every component, packed without holes.
    std::vector<Entity> m_entities{};
};

template <typename T>
class SparseSet final : public SparseSetBase
{
public:
    T& emplace(Entity entity, T component)
    {
        if (contains(entity))
        {
            T& stored = m_components[m_sparse[entity.index]];
            stored = std::move(component);
            return stored;
        }

        if (entity.index >= m_sparse.size())
        {
            m_sparse.resize(entity.index + 1uz, empty_slot);
        }
        m_sparse[entity.index] = static_cast<std::uint32_t>(m_entities.size());
        m_entities.push_back(entity);
        m_components.push_back(std::move(component));
        return m_components.back();
    }

    T* try_get(Entity entity)
    {
        return contains(entity) ? &m_components[m_sparse[entity.index]] : nullptr;
    }

    const T* try_get(Entity entity) const
    {
        return contains(entity) ? &m_components[m_sparse[entity.index]] : nullptr;
    }

    void remove(Entity entity) override
    {
        if (!contains(entity))
        {
            return;
        }

        // The last component moves into the hole, so the dense arrays stay packed.
        const std::uint32_t slot = m_sparse[entity.index];
        const Entity last = m_entities.back();
        if (last != entity)
        {
            m_entities[slot] = last;
            m_components[slot] = std::move(m_components.back());
            m_sparse[last.index] = slot;
        }
        m_sparse[entity.index] = empty_slot;
        m_entities.pop_back();
        m_components.pop_back();
    }

private:
    // Parallel to m_entities.
    std::vector<T> m_components{};
};

// A resource behind a base without its type, so the world keeps all resources in one list.
class ResourceBase
{
public:
    ResourceBase() = default;
    virtual ~ResourceBase() = default;

    ResourceBase(const ResourceBase&) = delete;
    ResourceBase& operator=(const ResourceBase&) = delete;
    ResourceBase(ResourceBase&&) = delete;
    ResourceBase& operator=(ResourceBase&&) = delete;
};

template <typename T>
class ResourceHolder final : public ResourceBase
{
public:
    explicit ResourceHolder(T value) : m_value(std::move(value))
    {
    }

    T& value()
    {
        return m_value;
    }

private:
    T m_value;
};

} // namespace ember::ecs

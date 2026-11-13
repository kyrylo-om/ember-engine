// Storage of resources that are referred to by handles.
module ember.platform:resource_table;

import ember.common;
import std;

namespace ember::platform
{

// Owns resources of one kind and gives out handles to them.
//
// A freed slot is reused by a later resource. Its generation grows every time, so handles to
// the freed resource stop matching the slot and are detected as stale.
// Tag is the tag type of the handles, Resource the stored type.
template <typename Tag, typename Resource>
class ResourceTable
{
public:
    Handle<Tag> add(Resource resource)
    {
        if (m_free_slots.empty())
        {
            m_slots.emplace_back();
            m_free_slots.push_back(static_cast<std::uint32_t>(m_slots.size() - 1));
        }
        const std::uint32_t index = m_free_slots.back();
        m_free_slots.pop_back();

        Slot& slot = m_slots[index];
        slot.resource.emplace(std::move(resource));
        return {.index = index, .generation = slot.generation};
    }

    // Frees the resource of a handle. Nothing happens for a null or stale handle.
    void remove(Handle<Tag> handle)
    {
        if (find(handle) == nullptr)
        {
            return;
        }
        Slot& slot = m_slots[handle.index];
        slot.resource.reset();
        ++slot.generation;
        m_free_slots.push_back(handle.index);
    }

    // Returns the resource of a handle, or nullptr for a null or stale handle.
    Resource* find(Handle<Tag> handle)
    {
        if (handle.index >= m_slots.size())
        {
            return nullptr;
        }
        Slot& slot = m_slots[handle.index];
        if (slot.generation != handle.generation || !slot.resource.has_value())
        {
            return nullptr;
        }
        return &*slot.resource;
    }

    const Resource* find(Handle<Tag> handle) const
    {
        if (handle.index >= m_slots.size())
        {
            return nullptr;
        }
        const Slot& slot = m_slots[handle.index];
        if (slot.generation != handle.generation || !slot.resource.has_value())
        {
            return nullptr;
        }
        return &*slot.resource;
    }

    // Calls a function for every stored resource.
    template <typename Function>
    void for_each(Function function)
    {
        for (Slot& slot : m_slots)
        {
            if (slot.resource.has_value())
            {
                function(*slot.resource);
            }
        }
    }

private:
    struct Slot
    {
        std::optional<Resource> resource{};
        // Starts from 1, so a default-constructed handle with generation 0 never matches.
        std::uint32_t generation = 1;
    };

    std::vector<Slot> m_slots{};
    std::vector<std::uint32_t> m_free_slots{};
};

} // namespace ember::platform

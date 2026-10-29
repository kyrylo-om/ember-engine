export module ember.ecs:name;

import std;

export namespace ember::ecs
{

/// Readable name of an entity, shown by the debug tools.
struct Name
{
    /// The name; several entities may have the same one.
    std::string value{};
};

} // namespace ember::ecs

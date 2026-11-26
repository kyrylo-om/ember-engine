export module ember.ecs:rigid_body;

export namespace ember::ecs
{

/// How a rigid body is moved by physics.
enum class BodyType
{
    /// Never moves, for example walls and the ground.
    Static,
    /// Moved by its velocity, gravity and collisions.
    Dynamic,
    /// Moved only by its velocity; pushes dynamic bodies but is not pushed by them.
    Kinematic
};

/// Makes an entity with a `Transform` and a `Collider` part of the physics simulation.
struct RigidBody
{
    /// How the body is moved.
    BodyType type = BodyType::Dynamic;
    /// Mass of the body in kilograms. Must be greater than 0 for dynamic bodies.
    float mass = 1.0f;
    /// Bounciness, from 0 (no bounce) to 1 (keeps all its speed).
    float restitution = 0.0f;
    /// Friction against the surfaces the body touches, usually from 0 (slides like on ice)
    /// to 1.
    float friction = 0.6f;
    /// Factor applied to the gravity of the physics settings; 0 disables gravity.
    float gravity_scale = 1.0f;
    /// Whether the body keeps its rotation instead of being turned by collisions, for
    /// example a player character that must stay upright.
    bool is_rotation_fixed = false;

    /// Compares two rigid bodies field by field.
    constexpr bool operator==(const RigidBody&) const = default;
};

} // namespace ember::ecs

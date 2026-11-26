// The Box2D world with one body for every entity that has a Transform, a RigidBody and a
// Collider. The only part of the engine that uses Box2D: it converts components to Box2D bodies
// and back, and world units to the meters Box2D works in.
module ember.physics:simulation;

import ember.common;
import ember.core;
import ember.ecs;
import box2d;
import std;

namespace ember::physics
{

// Stored as a world resource. Owns the Box2D world, so it is movable but not copyable.
class Simulation
{
public:
    explicit Simulation(float units_per_meter);
    ~Simulation();

    Simulation(const Simulation&) = delete;
    Simulation& operator=(const Simulation&) = delete;
    Simulation(Simulation&& other) noexcept;
    Simulation& operator=(Simulation&&) = delete;

    // Creates the body of an entity, or passes the changes that the game made to its
    // components since the previous call. Without a Velocity component the velocity of the
    // body is left to the simulation.
    void sync_body(ecs::Entity entity, const ecs::Transform& transform,
                   const ecs::RigidBody& rigid_body, const ecs::Collider& collider,
                   const ecs::Velocity* velocity);

    // Destroys the bodies of the entities for which should_remove returns true.
    template <typename Predicate>
    void remove_bodies_if(Predicate should_remove);

    // Advances the simulation. Gravity is in world units per second squared.
    void step(core::Seconds delta, Vec2 gravity, int substep_count);

    // Writes the position, rotation and velocity of a moving body into the components of its
    // entity. Static bodies and entities without a body are left unchanged.
    void write_body(ecs::Entity entity, ecs::Transform& transform, ecs::Velocity* velocity);

private:
    struct Body
    {
        b2BodyId id{};
        // The components as they were last passed to or written from the simulation. A
        // difference to the current components means that the game changed them.
        ecs::Transform transform{};
        ecs::RigidBody rigid_body{};
        ecs::Collider collider{};
        ecs::Velocity velocity{};
    };

    void create_body(ecs::Entity entity, const ecs::Transform& transform,
                     const ecs::RigidBody& rigid_body, const ecs::Collider& collider,
                     const ecs::Velocity& velocity);
    void destroy_body(const Body& body);

    b2Vec2 to_meters(Vec2 vector) const;
    Vec2 to_units(b2Vec2 vector) const;
    float area_in_square_meters(const ecs::Collider& collider) const;

    b2WorldId m_world{};
    float m_units_per_meter = 0.0f;
    // Gravity last given to Box2D; zero until the first step.
    Vec2 m_gravity{};
    std::map<ecs::Entity, Body> m_bodies{};
};

template <typename Predicate>
void Simulation::remove_bodies_if(Predicate should_remove)
{
    std::erase_if(m_bodies,
                  [this, &should_remove](const auto& entry)
                  {
                      if (!should_remove(entry.first))
                      {
                          return false;
                      }
                      destroy_body(entry.second);
                      return true;
                  });
}

namespace
{

b2BodyType to_box2d(ecs::BodyType type)
{
    switch (type)
    {
        case ecs::BodyType::Static:
            return b2_staticBody;
        case ecs::BodyType::Dynamic:
            return b2_dynamicBody;
        case ecs::BodyType::Kinematic:
            return b2_kinematicBody;
    }
    std::unreachable();
}

bool has_valid_size(const ecs::Collider& collider)
{
    if (collider.shape == ecs::ColliderShape::Box)
    {
        return collider.size.x > 0.0f && collider.size.y > 0.0f;
    }
    return collider.radius > 0.0f;
}

} // namespace

Simulation::Simulation(float units_per_meter) : m_units_per_meter{units_per_meter}
{
    contract_assert(units_per_meter > 0.0f);

    b2WorldDef definition = b2DefaultWorldDef();
    // Set in the first step from the settings, which can change while the game runs.
    definition.gravity = b2Vec2{0.0f, 0.0f};
    m_world = b2CreateWorld(&definition);
}

Simulation::~Simulation()
{
    // A moved-from simulation owns no world. Destroying the world destroys all its bodies.
    if (b2World_IsValid(m_world))
    {
        b2DestroyWorld(m_world);
    }
}

Simulation::Simulation(Simulation&& other) noexcept
    : m_world{std::exchange(other.m_world, b2WorldId{})},
      m_units_per_meter{other.m_units_per_meter}, m_gravity{other.m_gravity},
      m_bodies{std::move(other.m_bodies)}
{
}

void Simulation::sync_body(ecs::Entity entity, const ecs::Transform& transform,
                           const ecs::RigidBody& rigid_body, const ecs::Collider& collider,
                           const ecs::Velocity* velocity)
{
    const auto found = m_bodies.find(entity);
    if (found == m_bodies.end())
    {
        create_body(entity, transform, rigid_body, collider,
                    velocity != nullptr ? *velocity : ecs::Velocity{});
        return;
    }

    Body& body = found->second;
    if (body.rigid_body != rigid_body || body.collider != collider)
    {
        // Recreated instead of updated field by field: games rarely change these components,
        // and creating the body again handles every field in one place.
        const ecs::Velocity current =
            velocity != nullptr
                ? *velocity
                : ecs::Velocity{.value = to_units(b2Body_GetLinearVelocity(body.id)),
                                .angular = b2Body_GetAngularVelocity(body.id)};
        destroy_body(body);
        m_bodies.erase(found);
        create_body(entity, transform, rigid_body, collider, current);
        return;
    }

    if (transform != body.transform)
    {
        b2Body_SetTransform(body.id, to_meters(transform.position), b2MakeRot(transform.rotation));
        // Moving a body does not wake it, and a sleeping body would hang where it was put.
        b2Body_SetAwake(body.id, true);
        body.transform = transform;
    }
    if (velocity != nullptr && *velocity != body.velocity)
    {
        b2Body_SetLinearVelocity(body.id, to_meters(velocity->value));
        b2Body_SetAngularVelocity(body.id, velocity->angular);
        body.velocity = *velocity;
    }
}

void Simulation::step(core::Seconds delta, Vec2 gravity, int substep_count)
{
    contract_assert(delta > core::Seconds::zero());
    contract_assert(substep_count > 0);

    if (gravity != m_gravity)
    {
        b2World_SetGravity(m_world, to_meters(gravity));
        // Sleeping bodies ignore gravity, so without waking them bodies resting on the ground
        // would stay there even if gravity turned upward.
        for (const Body& body : m_bodies | std::views::values)
        {
            b2Body_SetAwake(body.id, true);
        }
        m_gravity = gravity;
    }
    b2World_Step(m_world, delta.count(), substep_count);
}

void Simulation::write_body(ecs::Entity entity, ecs::Transform& transform, ecs::Velocity* velocity)
{
    const auto found = m_bodies.find(entity);
    if (found == m_bodies.end() || found->second.rigid_body.type == ecs::BodyType::Static)
    {
        return;
    }

    Body& body = found->second;
    transform.position = to_units(b2Body_GetPosition(body.id));
    // Box2D converts angles with approximations accurate to about 0.001 radians, so a rotation
    // that cannot change is kept exactly as the game set it.
    if (!body.rigid_body.is_rotation_fixed)
    {
        transform.rotation = b2Rot_GetAngle(b2Body_GetRotation(body.id));
    }
    body.transform = transform;
    if (velocity != nullptr)
    {
        velocity->value = to_units(b2Body_GetLinearVelocity(body.id));
        velocity->angular = b2Body_GetAngularVelocity(body.id);
        body.velocity = *velocity;
    }
}

void Simulation::create_body(ecs::Entity entity, const ecs::Transform& transform,
                             const ecs::RigidBody& rigid_body, const ecs::Collider& collider,
                             const ecs::Velocity& velocity)
{
    contract_assert(has_valid_size(collider));
    contract_assert(rigid_body.type != ecs::BodyType::Dynamic || rigid_body.mass > 0.0f);

    b2BodyDef body_definition = b2DefaultBodyDef();
    body_definition.type = to_box2d(rigid_body.type);
    body_definition.position = to_meters(transform.position);
    // Box2D measures angles counterclockwise with y up. With y down, as in Ember, the same
    // angle turns clockwise on the screen, so angles need no conversion.
    body_definition.rotation = b2MakeRot(transform.rotation);
    body_definition.linearVelocity = to_meters(velocity.value);
    body_definition.angularVelocity = velocity.angular;
    body_definition.gravityScale = rigid_body.gravity_scale;
    body_definition.motionLocks.angularZ = rigid_body.is_rotation_fixed;
    const b2BodyId id = b2CreateBody(m_world, &body_definition);

    b2ShapeDef shape_definition = b2DefaultShapeDef();
    // Box2D derives the mass from the density, so the density is chosen to give the mass of
    // the component. Static and kinematic bodies ignore it.
    shape_definition.density = rigid_body.mass / area_in_square_meters(collider);
    shape_definition.material.friction = rigid_body.friction;
    shape_definition.material.restitution = rigid_body.restitution;
    shape_definition.filter.categoryBits = collider.layer;
    shape_definition.filter.maskBits = collider.mask;
    shape_definition.isSensor = collider.is_trigger;

    const b2Vec2 center = to_meters(collider.offset);
    if (collider.shape == ecs::ColliderShape::Box)
    {
        const b2Polygon box =
            b2MakeOffsetBox(collider.size.x / 2.0f / m_units_per_meter,
                            collider.size.y / 2.0f / m_units_per_meter, center, b2MakeRot(0.0f));
        b2CreatePolygonShape(id, &shape_definition, &box);
    }
    else
    {
        const b2Circle circle{.center = center, .radius = collider.radius / m_units_per_meter};
        b2CreateCircleShape(id, &shape_definition, &circle);
    }

    m_bodies.insert_or_assign(entity, Body{.id = id,
                                           .transform = transform,
                                           .rigid_body = rigid_body,
                                           .collider = collider,
                                           .velocity = velocity});
}

void Simulation::destroy_body(const Body& body)
{
    b2DestroyBody(body.id);
}

b2Vec2 Simulation::to_meters(Vec2 vector) const
{
    return {vector.x / m_units_per_meter, vector.y / m_units_per_meter};
}

Vec2 Simulation::to_units(b2Vec2 vector) const
{
    return {vector.x * m_units_per_meter, vector.y * m_units_per_meter};
}

float Simulation::area_in_square_meters(const ecs::Collider& collider) const
{
    if (collider.shape == ecs::ColliderShape::Box)
    {
        return (collider.size.x / m_units_per_meter) * (collider.size.y / m_units_per_meter);
    }
    const float radius = collider.radius / m_units_per_meter;
    return std::numbers::pi_v<float> * radius * radius;
}

} // namespace ember::physics

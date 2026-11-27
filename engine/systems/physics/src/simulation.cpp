// The Box2D world with one body for every entity that has a Transform, a RigidBody and a
// Collider. The only part of the engine that uses Box2D: it converts components to Box2D bodies
// and back, and world units to the meters Box2D works in.
module ember.physics:simulation;

import :events;
import :raycast_hit;
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
    // body is left to the simulation. Recreating a body ends its collisions and triggers.
    void sync_body(ecs::Entity entity, const ecs::Transform& transform,
                   const ecs::RigidBody& rigid_body, const ecs::Collider& collider,
                   const ecs::Velocity* velocity, core::EventBus& events);

    // Destroys the bodies of the entities for which should_remove returns true and publishes
    // the end of their collisions and triggers.
    template <typename Predicate>
    void remove_bodies_if(Predicate should_remove, core::EventBus& events);

    // Advances the simulation and publishes the collisions and triggers that started or ended
    // in this step. Gravity is in world units per second squared.
    void step(core::Seconds delta, Vec2 gravity, int substep_count, core::EventBus& events);

    // Writes the position, rotation and velocity of a moving body into the components of its
    // entity. Static bodies and entities without a body are left unchanged.
    void write_body(ecs::Entity entity, ecs::Transform& transform, ecs::Velocity* velocity);

    // Finds the closest collider that is not a trigger on the line between two points in world
    // units.
    std::optional<RaycastHit> raycast(Vec2 from, Vec2 to, std::uint32_t mask) const;

    // Applies an impulse to the body of the entity in the next step, after the changes to the
    // components. The impulse is in kilograms times world units per second.
    void queue_impulse(ecs::Entity entity, Vec2 impulse);

private:
    struct Body
    {
        // The user data of the shape points here, so contact events can name the entity of a
        // shape. Elements of a std::map never move, so the pointer stays valid.
        ecs::Entity entity{};
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
    void destroy_body(const Body& body, core::EventBus& events);
    void publish_collision_events(core::EventBus& events);
    void publish_trigger_events(core::EventBus& events);

    b2Vec2 to_meters(Vec2 vector) const;
    Vec2 to_units(b2Vec2 vector) const;
    float area_in_square_meters(const ecs::Collider& collider) const;

    b2WorldId m_world{};
    float m_units_per_meter = 0.0f;
    // Gravity last given to Box2D; zero until the first step.
    Vec2 m_gravity{};
    std::map<ecs::Entity, Body> m_bodies{};
    // Pairs of entities whose colliders touch, the smaller entity first. Box2D cannot name the
    // entities of a destroyed body, so the collisions it ends are taken from here.
    std::set<std::pair<ecs::Entity, ecs::Entity>> m_collisions{};
    // Pairs of a trigger and an entity inside it, for the same reason.
    std::set<std::pair<ecs::Entity, ecs::Entity>> m_trigger_overlaps{};
    std::vector<std::pair<ecs::Entity, Vec2>> m_impulses{};
};

template <typename Predicate>
void Simulation::remove_bodies_if(Predicate should_remove, core::EventBus& events)
{
    std::erase_if(m_bodies,
                  [this, &should_remove, &events](const auto& entry)
                  {
                      if (!should_remove(entry.first))
                      {
                          return false;
                      }
                      destroy_body(entry.second, events);
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

ecs::Entity entity_of(b2ShapeId shape)
{
    return *static_cast<const ecs::Entity*>(b2Shape_GetUserData(shape));
}

std::pair<ecs::Entity, ecs::Entity> ordered(ecs::Entity first, ecs::Entity second)
{
    return {std::min(first, second), std::max(first, second)};
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
      m_bodies{std::move(other.m_bodies)}, m_collisions{std::move(other.m_collisions)},
      m_trigger_overlaps{std::move(other.m_trigger_overlaps)},
      m_impulses{std::move(other.m_impulses)}
{
}

void Simulation::sync_body(ecs::Entity entity, const ecs::Transform& transform,
                           const ecs::RigidBody& rigid_body, const ecs::Collider& collider,
                           const ecs::Velocity* velocity, core::EventBus& events)
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
        destroy_body(body, events);
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

void Simulation::step(core::Seconds delta, Vec2 gravity, int substep_count, core::EventBus& events)
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

    for (const auto& [entity, impulse] : m_impulses)
    {
        if (const auto found = m_bodies.find(entity); found != m_bodies.end())
        {
            b2Body_ApplyLinearImpulseToCenter(found->second.id, to_meters(impulse), true);
        }
    }
    m_impulses.clear();

    b2World_Step(m_world, delta.count(), substep_count);
    publish_collision_events(events);
    publish_trigger_events(events);
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

std::optional<RaycastHit> Simulation::raycast(Vec2 from, Vec2 to, std::uint32_t mask) const
{
    if (from == to)
    {
        return std::nullopt;
    }

    // The ray belongs to every layer, so only its own mask decides what it hits.
    const b2QueryFilter filter{.categoryBits = std::numeric_limits<std::uint64_t>::max(),
                               .maskBits = mask};
    b2RayResult closest{};
    // Box2D calls the function for every collider on the ray. Returning the fraction of the ray
    // shortens it to that hit, so in the end only the closest one is stored; -1 skips a collider.
    b2World_CastRay(
        m_world, to_meters(from), to_meters(to - from), filter,
        [](b2ShapeId shape, b2Pos point, b2Vec2 normal, float fraction, void* context)
        {
            // A fraction of 0 means that the ray starts inside the collider.
            if (b2Shape_IsSensor(shape) || fraction == 0.0f)
            {
                return -1.0f;
            }
            b2RayResult& result = *static_cast<b2RayResult*>(context);
            result.shapeId = shape;
            result.point = point;
            result.normal = normal;
            result.fraction = fraction;
            result.hit = true;
            return fraction;
        },
        &closest);

    if (!closest.hit)
    {
        return std::nullopt;
    }
    return RaycastHit{.entity = entity_of(closest.shapeId),
                      .point = to_units(closest.point),
                      .normal = {closest.normal.x, closest.normal.y},
                      .distance = closest.fraction * distance(from, to)};
}

void Simulation::queue_impulse(ecs::Entity entity, Vec2 impulse)
{
    m_impulses.emplace_back(entity, impulse);
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
    Body& body = m_bodies
                     .emplace(entity, Body{.entity = entity,
                                           .id = id,
                                           .transform = transform,
                                           .rigid_body = rigid_body,
                                           .collider = collider,
                                           .velocity = velocity})
                     .first->second;

    b2ShapeDef shape_definition = b2DefaultShapeDef();
    // Box2D derives the mass from the density, so the density is chosen to give the mass of
    // the component. Static and kinematic bodies ignore it.
    shape_definition.density = rigid_body.mass / area_in_square_meters(collider);
    shape_definition.material.friction = rigid_body.friction;
    shape_definition.material.restitution = rigid_body.restitution;
    shape_definition.filter.categoryBits = collider.layer;
    shape_definition.filter.maskBits = collider.mask;
    shape_definition.isSensor = collider.is_trigger;
    // Both are off by default in Box2D.
    shape_definition.enableContactEvents = true;
    shape_definition.enableSensorEvents = true;
    shape_definition.userData = &body.entity;

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
}

void Simulation::destroy_body(const Body& body, core::EventBus& events)
{
    b2DestroyBody(body.id);

    const ecs::Entity entity = body.entity;
    const auto involves_entity = [entity](const std::pair<ecs::Entity, ecs::Entity>& pair)
    {
        return pair.first == entity || pair.second == entity;
    };
    std::erase_if(m_collisions,
                  [&](const auto& pair)
                  {
                      if (!involves_entity(pair))
                      {
                          return false;
                      }
                      events.publish(CollisionEnded{.first = pair.first, .second = pair.second});
                      return true;
                  });
    std::erase_if(m_trigger_overlaps,
                  [&](const auto& pair)
                  {
                      if (!involves_entity(pair))
                      {
                          return false;
                      }
                      events.publish(TriggerExited{.trigger = pair.first, .other = pair.second});
                      return true;
                  });
}

void Simulation::publish_collision_events(core::EventBus& events)
{
    const b2ContactEvents contacts = b2World_GetContactEvents(m_world);

    for (const b2ContactBeginTouchEvent& begin :
         std::span{contacts.beginEvents, static_cast<std::size_t>(contacts.beginCount)})
    {
        const ecs::Entity first = entity_of(begin.shapeIdA);
        const ecs::Entity second = entity_of(begin.shapeIdB);
        if (m_collisions.insert(ordered(first, second)).second)
        {
            const b2ContactData contact = b2Contact_GetData(begin.contactId);
            // Box2D's normal points from shape A to shape B, like the one of the event.
            events.publish(
                CollisionStarted{.first = first,
                                 .second = second,
                                 .normal = {contact.manifold.normal.x, contact.manifold.normal.y}});
        }
    }

    for (const b2ContactEndTouchEvent& end :
         std::span{contacts.endEvents, static_cast<std::size_t>(contacts.endCount)})
    {
        // The shapes of destroyed bodies are no longer valid; their collisions were ended when
        // the bodies were destroyed.
        if (!b2Shape_IsValid(end.shapeIdA) || !b2Shape_IsValid(end.shapeIdB))
        {
            continue;
        }
        const ecs::Entity first = entity_of(end.shapeIdA);
        const ecs::Entity second = entity_of(end.shapeIdB);
        if (m_collisions.erase(ordered(first, second)) > 0)
        {
            events.publish(CollisionEnded{.first = first, .second = second});
        }
    }
}

void Simulation::publish_trigger_events(core::EventBus& events)
{
    const b2SensorEvents sensors = b2World_GetSensorEvents(m_world);

    for (const b2SensorBeginTouchEvent& begin :
         std::span{sensors.beginEvents, static_cast<std::size_t>(sensors.beginCount)})
    {
        const ecs::Entity trigger = entity_of(begin.sensorShapeId);
        const ecs::Entity other = entity_of(begin.visitorShapeId);
        if (m_trigger_overlaps.insert({trigger, other}).second)
        {
            events.publish(TriggerEntered{.trigger = trigger, .other = other});
        }
    }

    for (const b2SensorEndTouchEvent& end :
         std::span{sensors.endEvents, static_cast<std::size_t>(sensors.endCount)})
    {
        if (!b2Shape_IsValid(end.sensorShapeId) || !b2Shape_IsValid(end.visitorShapeId))
        {
            continue;
        }
        const ecs::Entity trigger = entity_of(end.sensorShapeId);
        const ecs::Entity other = entity_of(end.visitorShapeId);
        if (m_trigger_overlaps.erase({trigger, other}) > 0)
        {
            events.publish(TriggerExited{.trigger = trigger, .other = other});
        }
    }
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

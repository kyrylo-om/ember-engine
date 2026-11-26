// The C API of Box2D as the C++ module `box2d`, so physics imports it like every other
// dependency. Only the names that physics uses are exported. In C++ the Box2D functions are
// `extern "C"` and its inline functions are plain `inline`, so they can be exported with
// using-declarations. Macros and the `static const` null ids cannot be exported; a
// value-initialized id such as `b2BodyId{}` is null.
module;

#include <box2d/box2d.h>

export module box2d;

// Math
export
{
    using ::b2MakeRot;
    using ::b2Pos;
    using ::b2Rot;
    using ::b2Rot_GetAngle;
    using ::b2Vec2;
}

// World
export
{
    using ::b2CreateWorld;
    using ::b2DefaultWorldDef;
    using ::b2DestroyWorld;
    using ::b2World_IsValid;
    using ::b2World_SetGravity;
    using ::b2World_Step;
    using ::b2WorldDef;
    using ::b2WorldId;
}

// Bodies
export
{
    using ::b2_dynamicBody;
    using ::b2_kinematicBody;
    using ::b2_staticBody;
    using ::b2Body_ApplyForceToCenter;
    using ::b2Body_ApplyLinearImpulseToCenter;
    using ::b2Body_GetAngularVelocity;
    using ::b2Body_GetLinearVelocity;
    using ::b2Body_GetPosition;
    using ::b2Body_GetRotation;
    using ::b2Body_GetUserData;
    using ::b2Body_IsValid;
    using ::b2Body_SetAngularVelocity;
    using ::b2Body_SetGravityScale;
    using ::b2Body_SetLinearVelocity;
    using ::b2Body_SetMotionLocks;
    using ::b2Body_SetTransform;
    using ::b2Body_SetType;
    using ::b2Body_SetUserData;
    using ::b2BodyDef;
    using ::b2BodyId;
    using ::b2BodyType;
    using ::b2CreateBody;
    using ::b2DefaultBodyDef;
    using ::b2DestroyBody;
    using ::b2MotionLocks;
}

// Shapes
export
{
    using ::b2Circle;
    using ::b2CreateCircleShape;
    using ::b2CreatePolygonShape;
    using ::b2DefaultShapeDef;
    using ::b2DestroyShape;
    using ::b2Filter;
    using ::b2MakeBox;
    using ::b2MakeOffsetBox;
    using ::b2Polygon;
    using ::b2Shape_GetBody;
    using ::b2Shape_IsValid;
    using ::b2ShapeDef;
    using ::b2ShapeId;
    using ::b2SurfaceMaterial;
}

// Events
export
{
    using ::b2ContactBeginTouchEvent;
    using ::b2ContactEndTouchEvent;
    using ::b2ContactEvents;
    using ::b2SensorBeginTouchEvent;
    using ::b2SensorEndTouchEvent;
    using ::b2SensorEvents;
    using ::b2World_GetContactEvents;
    using ::b2World_GetSensorEvents;
}

// Queries
export
{
    using ::b2DefaultQueryFilter;
    using ::b2QueryFilter;
    using ::b2RayResult;
    using ::b2World_CastRayClosest;
}

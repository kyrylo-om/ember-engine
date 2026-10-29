# ecs

Entity component system, the storage for all game objects: entities with generation
counters, component storage in sparse sets, views over entities that have a given set of
components, resources (one instance per type, such as services and settings), deferred
commands, and the component registry used by scene loading and the inspector.

Also defines all engine components, one partition per component in `components/`:
`Transform`, `Velocity`, `Collider`, `RigidBody`, `Sprite`, `Camera`, `Emitter` and `Name`.
Their fields and conventions (units, origin, angle direction) are the contract between
modules, so other modules ask the ECS maintainer for changes instead of editing them.

# ecs

Entity component system, the storage for all game objects: entities with generation
counters, component storage in sparse sets, views over entities that have a given set of
components, resources (one instance per type, such as services and settings), deferred
commands, and the component registry used by scene loading and the inspector.

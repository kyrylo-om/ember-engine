# physics

2D physics for entities with a `Transform`, a `RigidBody` and a `Collider`: static, dynamic
and kinematic bodies with box and circle shapes that rotate, movement with the fixed time
step, collision response, and collision and trigger events.

The simulation is done by [Box2D](https://box2d.org), which is used only in `src/`; no
Box2D type appears in the interfaces. Pure logic without Raylib, tested with unit tests
only.

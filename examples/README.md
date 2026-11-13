# Examples

Programs built with Ember. Each example is a separate folder and executable:

```text
examples/<name>/
├── CMakeLists.txt
├── main.cpp
└── assets/        images, sounds and scenes embedded into the executable
```

The sample game, which uses every module, is the final test of the engine. Smaller
examples are used to try out a single module during development.

## Examples

| Name       | What it shows                                                                  |
|------------|--------------------------------------------------------------------------------|
| `yozakura` | A small game: input, a fixed time step, events, commands, particles and sound  |

An example may have more source files than `main.cpp`; `yozakura` keeps its pictures and
sounds, which are made in code, in two modules of their own.

`run.sh` in the repository root builds one example and runs it, without building or running
the tests:

```sh
./run.sh                  # yozakura, release build
./run.sh yozakura debug   # another example or preset
```

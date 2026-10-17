# Coding conventions

These rules keep the code consistent no matter who wrote it. Where they say nothing,
follow the style of the surrounding code.

## Formatting

Formatting is done by clang-format 18 or newer with the `.clang-format` file in the
repository root. Format every file you change before committing; most editors can do it
on save.

clang-format takes care of the following:

- Braces always start on a new line: namespaces, classes, functions, control statements
  and lambdas. The only exception is a short lambda passed as an argument, which stays
  on one line.
- `if`, `else`, `for` and `while` always have braces, even around a single statement.
- Indentation is 4 spaces, lines are at most 100 characters.
- `*` and `&` belong to the type: `int* count`, `const World& world`.

```cpp
void clamp_fall_speed(Vec2& velocity)
{
    if (velocity.y > max_fall_speed)
    {
        velocity.y = max_fall_speed;
    }
}
```

clang-format does not check these, so keep them in mind yourself:

- One declaration per line.
- `const` goes before the type: `const Vec2&`, not `Vec2 const&`.
- Imports are grouped in a fixed order (see [Files and imports](#files-and-imports)).

## Naming

| What                                                | Style                   | Example                      |
|-----------------------------------------------------|-------------------------|------------------------------|
| Types, type aliases, concepts, template parameters  | `PascalCase`            | `EventBus`, `Handle<T>`      |
| Functions, variables, parameters, constants         | `snake_case`            | `add_system()`, `fixed_step` |
| Private and protected data members (not constants)  | `m_` + `snake_case`     | `m_systems`                  |
| Public data members                                 | `snake_case`            | `position`                   |
| Enumerators                                         | `PascalCase`            | `Stage::FixedUpdate`         |
| Namespaces                                          | `snake_case`            | `ember::physics`             |
| Modules and partitions                              | `snake_case`            | `ember.core:event_bus`       |
| Macros (only from CMake)                            | `EMBER_` + `UPPER_CASE` | `EMBER_DEBUG`                |
| Files and folders                                   | `snake_case`            | `event_bus.cppm`             |

- Names are full English words. Short names (`i`, `x`, `dt`) only for loop counters and math.
- Boolean names read as a yes/no question: `is_running`, `has_collider`.
- Getters are named after the value, setters start with `set_`: `width()`, `set_width()`.
- Files whose names are fixed by tools keep them: `CMakeLists.txt`, `README.md`.

## Files and imports

- Module interfaces are `.cppm`, implementation units are `.cpp`. A file is named after the
  main type in it: `EventBus` lives in `event_bus.cppm` and `src/event_bus.cpp`.
- Inside a module folder, interfaces go to the module root, implementation units to `src/`
  (see `engine/README.md`). The primary interface `<module>.cppm` exports module
  `ember.<module>` and re-exports its partitions; every other interface is a partition
  named after its file: `event_bus.cppm` is `ember.core:event_bus`.
- Everything is imported, nothing is included: modules of this repository by name
  (`import ember.core;`), the standard library with `import std;`, third-party libraries
  through their modules (`import raylib;`).
- Import order: the file's own module declaration, modules of this repository,
  third-party libraries, the standard library.
- Import what you use; don't rely on something being re-exported indirectly.
- No `using namespace` in module interfaces, and never `using namespace std`.
- Engine code lives in namespace `ember`, each module in its own nested namespace named
  like the module: `ember::physics`. Only the public API is exported. Helpers used by one
  `.cpp` file go into an anonymous namespace, internal helpers in interfaces stay
  unexported.

## Constants and macros

No magic numbers:

- Every number in the code has a name. Plain literals are fine only where their meaning
  is obvious: `0`, `1`, `size / 2`.
- A literal is also fine where it immediately gets a name: in the definition of a constant
  or as the default value of a data member (`float mass = 1.0f;`). Tests may use literals
  as test data.
- Constants are `constexpr` variables: `static constexpr` inside a class, `constexpr` in a
  namespace in a module interface or in an anonymous namespace in a `.cpp` file.
- Use what the standard library already defines: `std::numbers::pi_v<float>`,
  `std::numeric_limits<int>::max()`.
- Durations are `std::chrono` types, never bare numbers of seconds or milliseconds.

```cpp
namespace
{

constexpr float max_fall_speed = 980.0f;
constexpr auto blink_interval = std::chrono::milliseconds{500};

} // namespace
```

No `#define`:

- Constants are `constexpr` variables, macro functions are regular functions or
  templates.
- The only macros are compile flags defined by CMake with `target_compile_definitions`.
  The code only reads them:

```cpp
void draw_debug_shapes(const World& world)
{
#if EMBER_DEBUG
    draw_collider_outlines(world);
#endif
}
```

## Modern C++ instead of C

| Use                                                  | Instead of                                         |
|------------------------------------------------------|----------------------------------------------------|
| `static_cast<float>(x)` and other C++ casts          | `(float)x`                                         |
| `std::unique_ptr`, `std::make_unique`, containers    | `new`, `delete`, `malloc`, `free`                  |
| `std::array`, `std::vector`, `std::span`             | C arrays, pointer and size pairs                   |
| `std::string`, `std::string_view`                    | `char*` strings, C string functions                |
| `std::print`, `std::format`                          | `printf`, `sprintf`                                |
| `constexpr` variables and functions, templates       | `#define`                                          |
| `enum class`                                         | plain `enum`                                       |
| `using Name = Type;`                                 | `typedef`                                          |
| `nullptr`                                            | `NULL`, `0`                                        |
| `std::optional`, `std::expected`                     | special values (`-1`, `nullptr`), error codes      |
| `std::uint32_t` and other fixed-width types          | `unsigned int`, `long` when the size matters       |
| lambdas, `std::function`, templates                  | function pointers with `void*` data                |
| `std::variant`                                       | `union`                                            |
| range-based `for`, `<algorithm>`, `<ranges>`         | index loops when the index is not needed           |
| `std::sqrt`, `std::size_t` from `import std;`        | `sqrt`, `size_t` from the global namespace         |

- C arrays are allowed only where the language needs them, for example for data from
  `#embed`.
- `const_cast` and `reinterpret_cast` always come with a comment that explains why they
  are needed.

## Classes and functions

- `struct` is for plain data with public fields and no invariants: components, events,
  settings. Keep them aggregates (no constructors, no private members), so reflection can
  save, load and inspect them.
- `class` is for everything with private state; its data members are private.
- Every variable is initialized where it is declared, every data member has a default
  value. Prefer braces: `Vec2 position{};`.
- Prefer the rule of zero: members like `std::unique_ptr` and containers manage resources,
  so no destructor, copy or move has to be written. If one of them is needed, define or
  `= delete` all five.
- Constructors with one parameter are `explicit`. Overridden functions are marked
  `override`.
- Member functions that don't change the object are `const`.
- Functions whose result must not be ignored are `[[nodiscard]]`, in particular all that
  return `std::expected` or `std::optional`.
- Cheap types (numbers, `Vec2`, `std::string_view`, `std::span`) are passed by value,
  everything else by `const&`. Results are returned, not written into reference
  parameters.
- Raw pointers and references never own anything. Use a reference when the object always
  exists and a pointer when it may be missing.
- Use `auto` when the type is visible on the same line or too long to write (iterators,
  lambdas). Otherwise write the type.

## Errors

- Failures that can happen in a correct program, such as a missing asset or a broken
  scene file, are returned as `std::expected<T, Error>`. If the reason does not matter,
  return `std::optional<T>`.
- Engine code does not throw exceptions. Exceptions from a library are caught where the
  library is called and turned into an error value.
- Errors are never ignored silently: handle them, return them, or log them.

## Documentation

The public API in module interfaces is documented with Doxygen comments. `.cpp` files have no
Doxygen comments, only regular comments where they are needed.

- Doc comments start with `///` and stand on the lines above what they document.
  Tags start with `@`.
- Everything public gets a doc comment: classes, structs, enums and their values,
  functions, data members.
- The first sentence is a short summary and ends with a period. Add more sentences only
  when the user of the API needs them. Function descriptions start with a verb:
  "Loads ...", "Returns ...".
- Functions describe each parameter with `@param`, each template parameter with `@tparam`
  and the result with `@return`. Getters and setters only need the summary.
- `@note` marks something a user must not miss: lifetime, thread safety, performance.
  No other tags are used.
- Describe what the code does and how to use it, not how it is implemented.

```cpp
export module ember.assets:asset_manager;

import ember.common;
import std;

export namespace ember::assets
{

/// Reasons why an asset cannot be loaded.
enum class AssetError
{
    /// No asset with this name is registered in the catalog.
    NotFound,
    /// The asset data is corrupt or has an unsupported format.
    InvalidData
};

/// Loads assets embedded into the executable and caches them by name.
class AssetManager
{
public:
    /// Loads a texture, or returns the cached one if it was loaded before.
    /// @param name Name under which the texture is registered in the catalog.
    /// @return Handle to the texture, or the reason why it cannot be loaded.
    [[nodiscard]] std::expected<Handle<Texture>, AssetError> load_texture(std::string_view name);

    /// Returns the number of loaded textures.
    std::size_t texture_count() const;

private:
    // Cached by name, so loading the same asset twice does not decode it again.
    std::unordered_map<std::string, Handle<Texture>> m_textures;
};

} // namespace ember::assets
```

## Comments

- A comment explains why the code is written this way, or the idea behind code that is
  hard to follow. It never repeats what the code already says.
- Comments describe the code as it is now. No history and no work process: no "fixed",
  "changed", "new version", "added for ...", no names or dates. That belongs in commit
  messages.
- No commented-out code. Delete it; git keeps the old version.
- `// TODO: what is missing` marks known unfinished work. Remove it when the work is done.
- Code, comments, documentation and commit messages are written in English.

## Tests

- Tests use Catch2 and live in the `tests/` folder of their module. A test file is named
  after the file it tests: `event_bus_test.cpp` tests `event_bus.cppm`.
- Test case names describe the expected behavior:
  `TEST_CASE("events are delivered in publishing order")`.
- When you fix a bug, add a test that reproduces it if possible.

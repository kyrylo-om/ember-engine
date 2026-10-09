# Workflow conventions

## Working on main

- Everyone works on `main`. There are no other branches.
- Get the latest changes before you start working and before every push: `git pull`.
  With `pull.rebase` set, your local commits are moved on top of the new ones instead of
  creating a merge commit. On a conflict: fix the files, `git add` them, then
  `git rebase --continue` (or `git rebase --abort` to go back).
- Push small finished pieces of work often. The longer work stays local, the harder the
  conflicts get.
- Every pushed commit builds without warnings and passes the tests. Everyone pulls `main`,
  so a broken commit blocks the whole team.
- Never force-push and never change commits that are already pushed (`commit --amend`,
  `rebase`, `reset`). To undo a pushed commit, use `git revert <commit>`.
- Code that other modules depend on (`engine/common`, public interfaces of a module) is
  changed only after agreeing with the people who use it.
- Tell the person working on a module before you change it.

## Commits

- One commit is one logical change in one module. Files that belong to the change go
  together, unrelated changes go into separate commits.
- A change that spans several modules is split into one commit per module, in an order
  where every commit builds: first `common`, then the modules that use it. If that is
  impossible, for example when renaming a type used everywhere, make one commit with the
  scope `engine`.
- Formatting-only changes, renames and file moves are separate commits.
- `git add -p` stages only some of the changes in a file. Check what you are about to
  commit with `git diff --staged`.

## Commit messages

```text
type(scope): subject

- change
- change
```

- Subject: what the commit does, in imperative mood, lowercase, without a period, at most
  72 characters including the tags. `add box-circle collision test`, not
  `Added collision tests.`
- Body: required. A short list of the changes and their reasons, written like the subject.
  The diff shows how; the body says what and why.
- Lines are at most 72 characters. Nothing else goes into the message: no trailers,
  signatures or generated text.

```text
feat(physics): add box-circle collision test

- return contact normal and penetration depth
- treat a circle center inside the box as the deepest contact
```

More subjects:

```text
fix(ecs): detect stale entities after an index is reused
refactor(core): move stage lists into the scheduler
test(physics): cover resting contact between boxes
style(render): apply clang-format
build: require CMake 3.30
docs: describe the asset pipeline
chore: ignore clangd cache
```

### Types

| Type       | Use for                                                          |
|------------|------------------------------------------------------------------|
| `feat`     | New functionality                                                |
| `fix`      | Bug fix                                                          |
| `refactor` | Code change that keeps the behavior the same                     |
| `perf`     | Performance improvement                                          |
| `test`     | Adding or changing tests only                                    |
| `docs`     | Documentation only: README files, `docs/`, doc comments          |
| `style`    | Formatting only, the code itself does not change                 |
| `build`    | CMake, dependencies, compiler flags, CI                          |
| `chore`    | Repository maintenance: `.gitignore`, tool configs, moving files |

### Scopes

The scope is the one part of the repository that the commit changes:

- An engine module, named like its folder: `common`, `ecs`, `core`, `platform`,
  `assets`, `debug`, `physics`, `particles`, `render`. The `services` and `systems`
  groups are not scopes.
- `engine`: several engine modules at once, or `engine/` itself.
- `examples`: programs in `examples/`.
- No scope: repository-wide files, such as root files, `cmake/` and `docs/`.

When a module is added or renamed, update this list.

## Files that are never committed

- Build output, editor settings and system files are covered by `.gitignore`.
- Personal files: any file or folder whose name starts with `_` is ignored everywhere,
  for example `_notes.md`, `_simd_try.cpp`, `_old/`. Use it for experiments and drafts.
- `docs/personal/`: notes, drafts and context for AI assistants. Everything in it except
  its README is ignored.
- Files are stored with LF line endings. `.gitattributes` converts them automatically, so
  a commit never changes line endings only.

## AI assistants

Instructions for AI assistants (`AGENTS.md`, `CLAUDE.md` in the repository root) are
personal and ignored by git. You are responsible for every line you commit, whoever wrote
it: read and understand it first.

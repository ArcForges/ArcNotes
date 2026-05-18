# ArcNotes

Qt Widgets Markdown note manager, forked from QOwnNotes and currently being
rebuilt toward a Store-driven MVVM architecture. C++20 / Qt 5&6 / CMake /
SQLite.

---

## Current Architecture

ArcNotes is a single-process Qt Widgets application. The current codebase has a
layered structure, but it still contains legacy UI and bootstrap code from the
original QOwnNotes codebase.

Core write path:

```
View / MainWindow -> ViewModel / Coordinator -> CommandBus -> CommandHandler -> Service / Repository
```

Core read path:

```
Repository / Service -> AppCoordinator refresh -> State / ViewModel / Model -> View
```

The intended direction for new work is:

- Keep business logic out of widgets, dialogs, and views.
- Route new mutations through typed commands.
- Prefer `QAbstractItemModel` based views for primary UI lists and trees.
- Keep repository and SQL access in the core side of the application.

This is an incremental migration. Do not assume every existing file already
follows the rules below.

---

## Main Layers

| Layer | Current Role |
|-------|--------------|
| `core/data/` | Plain value structs and command structs |
| `core/repositories/` | SQLite/settings persistence and query logic |
| `core/services/` | Business operations built on repositories |
| `core/commands/` | Command handlers and `CommandBus` |
| `core/state/` | Observable state containers |
| `coordinator/` | Dependency wiring, command registration, model refresh flow |
| `viewmodels/` | Presentation state and user intent handlers |
| `models/` | `QAbstractItemModel` implementations |
| `views/` | Thin Qt item views bound to view models/models |
| `widgets/`, `dialogs/` | Qt Widgets UI, including some legacy item-widget code |
| `mainwindow*.cpp` | Application shell, actions, menus, dock layout, host adapters |

---

## Commands

New write operations should use command structs in `core/data/commands.h`.

Command rules:

1. Add a command struct with `static constexpr const char* Type`.
2. Implement handling in the relevant `core/commands/*commandhandlers.cpp`.
3. Register the handler in `AppCoordinator::registerCommandHandlers()`.
4. Keep business logic in handlers/services/repositories, not in views.

`main.cpp` and migration/bootstrap code may directly use repositories when the
application is not fully initialized yet. Treat that as a narrow exception.

---

## State And Models

State objects in `core/state/` are observable containers. They should emit Qt
signals when values change and should not perform database or file operations.

Primary list/tree UI should prefer:

- `QListView` / `QTreeView` / `QTableView`
- `QAbstractListModel` / `QAbstractItemModel` / `QAbstractTableModel`
- incremental updates with `beginInsertRows`, `beginRemoveRows`, and
  `dataChanged` when practical

Legacy dialogs/settings widgets still use `QListWidget`, `QTreeWidget`, and
`QTableWidget`. Do not expand that pattern into primary application surfaces.

---

## Dependency Rules

Preferred dependency direction:

```
core/data          -> Qt value/container types only
core/repositories  -> core/data, services/databaseservice
core/services      -> core/data, core/repositories
core/commands      -> core/data, core/services, core/repositories
core/state         -> core/data
coordinator        -> core/*, viewmodels
viewmodels         -> core/data, core/state, core/commands
models             -> core/data, core/state
views              -> viewmodels, models, widgets
dialogs/widgets    -> viewmodels, models, coordinator as needed
mainwindow         -> views, dialogs, coordinator
```

Hard boundaries for new code:

- `core/*` must not include `views/`, `widgets/`, `dialogs/`, `viewmodels/`, or
  `mainwindow`.
- `core/state` must not access repositories, services, SQL, or files.
- ViewModels must not hold QWidget pointers.
- Views/widgets/dialogs must not write SQL.

Current exceptions:

- Some settings ViewModels still read through repositories. Prefer moving new
  read APIs behind services/coordinator when touching that area.
- Some dialogs and settings widgets still own item widgets. Avoid broad
  rewrites unless the surrounding feature is already being migrated.

---

## Editor Rules

The editor path is sensitive because it touches user data.

- Do not call `setPlainText()` casually on the main editor.
- Only replace editor text on note switch, explicit reload, or controlled
  preview/export helper documents.
- User edits should mark editor state dirty and save through debounced
  `SaveNoteCommand`.
- `SaveNoteCommand` carries `baseChecksum` for conflict detection.
- External changes to a dirty note should set conflict state instead of
  overwriting local text.

Search and autosave should use `Debouncer` rather than dispatching commands on
every keystroke.

---

## Threading

Qt UI objects and `QAbstractItemModel` updates belong on the GUI thread.

Background work must deliver results back to the GUI thread with queued Qt
signals/slots or `Qt::QueuedConnection` before touching widgets or models.

---

## Build

- On Windows, run `vcvars64.bat` before building with MSVC / clang-cl.
- `ARC_QT6_BUILD` defaults to `OFF` in CMake, but Windows presets set it to
  `true`.
- `AUTOMOC` and `AUTORCC` are enabled.
- `AUTOUIC` is disabled for first-party code.
- First-party C++ sources are collected from `src/` with `libraries/` filtered
  out.
- Some bundled third-party `.ui` files are still wrapped explicitly.

---

## Practical Guidance

When changing code:

1. Follow nearby patterns unless they violate a hard boundary.
2. Keep refactors scoped to the feature or bug being fixed.
3. Do not move legacy widgets to model/view as drive-by cleanup.
4. Add new business behavior to commands/services/repositories.
5. Keep UI code focused on layout, binding, input handling, and presentation.
6. Run focused checks for touched files, and use the CMake-generated
   `build/compile_commands.json` when running clang-tidy.

<p align="center">
  <img src="src/images/icons/256x256/apps/ArcNotes.png" alt="ArcNotes" width="128">
</p>

<h1 align="center">ArcNotes</h1>

<p align="center">
  A desktop Markdown note manager built on modern Qt/C++ architecture,<br>
  forked from <a href="https://github.com/pbek/QOwnNotes">QOwnNotes</a> and
  evolving toward ArcChat AI-driven workflows.
</p>

<p align="center">
  <img src="https://img.shields.io/badge/C%2B%2B-20-blue?logo=cplusplus" alt="C++20">
  <img src="https://img.shields.io/badge/Qt-5%20%7C%206-41cd52?logo=qt" alt="Qt 5 | 6">
  <img src="https://img.shields.io/badge/license-GPL--2.0-orange" alt="GPL-2.0">
  <img src="https://img.shields.io/badge/platform-Windows%20%7C%20Linux%20%7C%20macOS-lightgrey" alt="Platform">
</p>

---

## What is ArcNotes?

ArcNotes takes the battle-tested QOwnNotes Markdown editor and rebuilds its
internals around a clean, layered MVVM architecture. Your notes stay as plain
Markdown files on disk -- no lock-in, no proprietary format. The codebase is
being restructured to separate business logic from UI, making it ready for
future integration with ArcChat, a local AI assistant that can search, summarize,
and organize your notes through natural language.

## Features

- **Plain Markdown files** -- notes live in folders you control, no database
  export needed
- **Syntax-highlighted editor** with live preview, Vim mode (FakeVim), and
  in-editor search
- **Hierarchical tags** with color coding and drag-drop organization
- **Sub-folder support** with recursive note browsing
- **Full-text search** across all notes
- **Note history and local trash** -- soft delete with restore
- **Multiple note folders** -- switch between separate note collections
- **Markdown preview** via litehtml rendering engine
- **Backlink and navigation panels** -- heading outline, wiki-link resolution
- **Customizable layouts** -- minimal, full, preview-only, vertical, single
  column, or build your own
- **Global hotkeys** for quick note capture
- **Command palette** for keyboard-driven workflows
- **Dark mode** with automatic system detection
- **PDF and HTML export**
- **Attachment and image management** with drag-drop insertion
- **Cross-platform** -- Windows, Linux, macOS

## Building

### Prerequisites

- CMake 4.2+
- Qt 5.15+ or Qt 6.2+
- C++20 compiler (MSVC 2022, GCC 11+, Clang 14+)

### Windows (MSVC / clang-cl)

```powershell
# Initialize the Visual Studio environment first
& "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"

cmake -B build -G Ninja -DARC_QT6_BUILD=ON
cmake --build build
```

### Linux / macOS

```bash
cmake -B build -DARC_QT6_BUILD=ON
cmake --build build
```

The built executable lands in the `build/` directory.

### CMake Options

| Option | Default | Description |
|--------|---------|-------------|
| `ARC_QT6_BUILD` | `OFF` | Build against Qt 6 instead of Qt 5 |
| `USE_QLITEHTML` | `ON` | Enable litehtml-based Markdown preview (Qt 6 only) |
| `DEV_MODE` | `OFF` | Enable stricter compiler warnings |
| `ENABLE_ASAN` | `OFF` | Enable address sanitizer |
| `ENABLE_CLANG_TIDY` | `OFF` | Run clang-tidy during build |

## Architecture

ArcNotes separates non-UI logic (`core/`) from presentation (`views/`,
`widgets/`, `dialogs/`). Mutations flow through a typed `CommandBus`, and UI
subscribes to observable state objects via Qt signals.

See [AGENTS.md](AGENTS.md) for the full architecture guide, layer dependency
rules, and contribution constraints.

## Roadmap

ArcNotes is one component of the **ArcForges** productivity suite. The
architecture is being designed so the core note engine can eventually run inside
ArcChat Mini as a shared service, with multiple UI surfaces (Qt Widgets, QML,
Avalonia) connecting through a local protocol. Current work focuses on
completing the MVVM migration and stabilizing the command/state layer.

## License

[GNU General Public License v2.0](LICENSE)

Based on [QOwnNotes](https://github.com/pbek/QOwnNotes) by Patrizio Bekerle.

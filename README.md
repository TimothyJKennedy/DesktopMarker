# DesktopMarker

A lightweight, always-on-top desktop drawing utility for Windows. Draw annotations, highlights, and marks directly on your screen with a compact, unobtrusive toolbar.

## How It Works

A small marker icon sits on the edge of your screen. Hover near it to open the toolbar, pick a color and tool, then click **DRAW** to start annotating. Everything you draw floats above your desktop and other windows. Press **Esc** or click **STOP** when you're done.

## Features

- **Screen-edge grip** — A tiny marker icon docked to any screen edge. Drag it to reposition; it snaps to the nearest edge on release.
- **Compact flyout panel** — Hover the grip to reveal drawing controls. Three configurable panel sizes (Compact, Standard, Full) via Settings.
- **12 preset colors + custom** — Black, white, and 10 vibrant colors. Click **+** to pick any color with the system color dialog.
- **Pen & Highlighter** — Pen draws solid strokes; Highlighter draws semi-transparent wide strokes.
- **Brush size slider** — Adjustable from 1px to 80px with a live preview circle.
- **Undo / Clear** — Left-click **DEL** to undo the last stroke, right-click to clear all (configurable in Settings).
- **Multi-monitor support** — The drawing overlay spans all connected monitors.
- **Persistent settings** — Color, tool, brush size, panel size, dock position, and delete mode are saved between sessions.
- **Always accessible** — The toolbar opens even while drawing mode is active, so you can change tools or colors mid-stroke.

## Keyboard Shortcuts

| Shortcut | Action |
|---|---|
| `Ctrl + Shift + D` | Toggle drawing mode on/off (global hotkey — works from any app) |
| `Esc` | Exit drawing mode |

## Settings

Click the gear icon (⚙) in the bottom-right of the toolbar to open Settings:

- **Delete Button Behavior** — Swap left-click/right-click between Undo and Clear All.
- **Panel Size** — Choose Compact (colors + buttons only), Standard (default, includes tools and slider), or Full (larger swatches and buttons).

## Download

Grab the latest `DesktopMarker.zip` from the releases. Unzip anywhere and run `DesktopMarker.exe`. No installation required.

**Requirements:** Windows 10 or later (64-bit).

## Building from Source

**Prerequisites:**
- Qt 6.7+ (Widgets module)
- CMake 3.25+
- A C++20 compiler (MinGW 13+ or MSVC 2022+)

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="C:/Qt/6.8.2/mingw_64"
cmake --build build --config Release
```

To create a distributable package:

```bash
mkdir dist
cp build/DesktopMarker.exe dist/
windeployqt --release --no-translations --no-opengl-sw dist/DesktopMarker.exe
```

## Tech Stack

- **C++20** / **Qt 6 Widgets**
- Custom `QPainterPath` vector strokes with quadratic Bezier interpolation
- Win32 API for flicker-free click-through (`WS_EX_TRANSPARENT`) and global hotkeys (`RegisterHotKey`)
- Frameless, translucent overlay windows spanning all monitors

## License

MIT

---
mkskill:
  pos: 30
  in: "*"
---
## How the receiver works

Everything below is what `source/ot4xb_log.cpp` actually does; there is no other
source of truth.

### The command line

```
ot4xb_log.exe [--class <name>] [--title <text>] [--log <name>] [--icon <file.ico>]
```

| Option | Meaning | Default |
|---|---|---|
| `--class` | Class name of the main window. It is what separates one viewer from another - see *Single instance*. | `11CBDBE2_0AF0_4713_B463_269FA6E2654B` |
| `--title` | Window caption and tray tooltip. | `ot4xb - log` |
| `--log` | Base name of the log file. Only the name is used (path and extension are stripped); the file is always written **next to the executable** as `<name>.log`. | the exe's own name, `ot4xb_log.log` |
| `--icon` | An `.ico` file read from disk, for the window and the tray. | the icon built into the executable |

Options are case insensitive and may appear in any order. Values are parsed by
the shell (`CommandLineToArgvW`), so quoting works as everywhere else in
Windows: `--title "My application"`. An unknown option, an option with no
value, and an empty value are all ignored - that field simply keeps its
default. The same goes for an icon that cannot be loaded: a missing file, or a
file that is not an icon, silently leaves the built-in one in place.

### Single instance

Before creating anything the exe does `FindWindow(<window-class>, NULL)`. If a
window with that class already exists it posts `WM_APP+1` (activate: show,
restore, bring to front) to it and **exits immediately**. That is the whole
single-instance mechanism: one viewer per window class, and a distinct class
name yields a distinct, independent viewer with its own log file.

### Receiving a line: `WM_COPYDATA`

The sender delivers each trace with `SendMessage(hWnd, WM_COPYDATA, ...)` where
`COPYDATASTRUCT.lpData` / `cbData` hold the raw text (`dwData` is ignored, no
terminating NUL required). On receipt the exe:

1. copies the buffer, appends `CR LF`, and calls `ReplyMessage(1)` so the sender
   is released before any disk or UI work happens;
2. appends `text + CR LF + NUL` to the `.log` file (`bWriteLogLine`);
3. renders the text in the RichEdit control (`ShowLogLine`).

### Log file format

The `.log` file is a sequence of **NUL-terminated records**: each record is the
text as sent plus `\r\n`, followed by a single `0x00` byte. That NUL is what
*Load History* uses to split the file back into entries; a file with a
non-NUL tail stops the reload at that point. The file is opened with
`OPEN_ALWAYS` and appended to; it is never truncated by the program - delete it
by hand to start over.

### Display

- The first line of a record (up to the first `\r\n`) is printed in **red**;
  the remaining text in black; then a long line of underscores in **blue** as a
  separator. The sender is expected to put the header (`ProcName`, line number)
  on that first line and the message after it.
- Font: Verdana 8 pt on a light grey (`0xF0F0F0`) background; read-only, vertical
  scroll, auto-scrolls to the bottom after each entry.
- The window is a `WS_EX_TOOLWINDOW` (no taskbar button), docked at the top-left
  of the primary monitor's work area, full height, max 540 px wide. It is created
  **hidden**; the tray icon is the way in.
- Closing the window (`WM_CLOSE`) only **hides** it; the process keeps running.

### Tray icon and context menu

Left click on the tray icon toggles the window. Right click - on the icon or
inside the text area - opens the menu:

| Item | Action |
|---|---|
| Popup on Event | Toggle: when checked, every incoming line shows the window (without stealing focus) if it is hidden. Off by default. |
| Show / Hide Window | Toggle visibility. |
| Load History | Clears the view and reloads the whole `.log` file (no popups while reloading). |
| Clear Log Window | Clears the **view only**; the file is untouched. |
| Exit <title> | Removes the tray icon and terminates the process. |

### Messages (for another process talking to the viewer)

| Message | Meaning |
|---|---|
| `WM_COPYDATA` | Deliver one log entry (see above). |
| `WM_APP + 1` | Activate: show, restore if minimized, bring to front. |
| `WM_APP + 2` | Internal: tray icon callback. |
| `WM_APP + 3` | Internal: exit request; guarded by `wp == lp == this`, so it cannot be triggered from outside. |

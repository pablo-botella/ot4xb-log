---
name: ot4xb-log
description: "The application that shows and logs the lines an Xbase++ program sends with the ot4xb.dll logging functions (ot4xb_lSendLogStrFL, ot4xb_lSendLogStr): a tiny Win32 tray exe that receives them via WM_COPYDATA, appends them to a .log file and displays them in a RichEdit window. Use when working with ot4xb-log or ot4xb_log.exe."
---

# ot4xb-log

**ot4xb-log** is the application that shows the log lines an Xbase++ program
sends with the logging functions of **ot4xb.dll**, and writes them to disk. It
is a minimal Win32 program that sits in the notification area (system tray);
for every line it receives it:

1. appends it to a `.log` file next to the executable (persistent history), and
2. prints it in a read-only RichEdit window, optionally popping the window up
   on every event.

It is a single C++ source file (`source/ot4xb_log.cpp`), a ~110 KB executable
with no installation and no runtime dependencies beyond
`user32`/`shlwapi`/`Riched20.dll`; it builds as a **Win32 (x86)** executable
with Visual Studio (toolset v143).

Repository: https://github.com/pablo-botella/ot4xb-log

## Sending lines from Xbase++

The sender lives in **ot4xb.dll**, the author's Xbase++ library. Among many
other things it exports two functions that push a string to `ot4xb_log.exe`:

```xbase
ot4xb_lSendLogStrFL( <cFunc>, <nLine>, <cFormat> [, <params, ...> ] )
ot4xb_lSendLogStr( <cFormat> [, <params, ...> ] )
```

`...FL` is the *func/line* flavour: it takes the calling function and the source
line as the first two arguments, so the entry arrives already tagged with where
it came from. Both format the remaining arguments and deliver a single string,
and both return `.T.` when the message reached its destination - which is also
the way to ask, from the program, whether anybody is listening.

What they do with it, as far as the viewer is concerned:

- they look for the viewer window by its class UUID
  (`11CBDBE2_0AF0_4713_B463_269FA6E2654B`) and, if found, deliver the text with
  `WM_COPYDATA`;
- if the viewer is **not running they do nothing, silently**: they just return
  `.F.` - no error, no file, no attempt to launch anything. Nothing ever
  launches `ot4xb_log.exe` automatically - you start it by hand when you want
  to look.

### An example: a pair of trace commands

Those two functions are the raw material; how each application calls them is
its own business. As an example, this is a pair of `#xcommand` definitions the
author uses in his own programs - copy them, or write whatever fits your code:

```xbase
#ifdef TRACE_ENABLED
#xcommand TRACEX <p1> [,<pN>] => ot4xb_lSendLogStrFL( ProcName() , __LINE__  ,"%s", "" + Var2Char(<p1>) [ + Var2Char(<pN>)] )
#xcommand TRACE  <p1> [,<pN>] => ot4xb_lSendLogStrFL( ProcName() , __LINE__  , cPrintf(, <p1> [ , <pN>] ) )
#else
#xcommand TRACEX <p1> [,<pN>] =>
#xcommand TRACE  <p1> [,<pN>] =>
#endif
```

- `TRACE cFormat [, args...]` - `cPrintf`-style: `TRACE "n=%d name=%s", n, cName`.
- `TRACEX expr [, exprN...]` - each argument goes through `Var2Char()` and the
  results are concatenated: `TRACEX "value: ", nValue, " / ", oObj`.
- Both pass `ProcName()` and `__LINE__`, so every entry carries the calling
  function and source line.
- With `TRACE_ENABLED` undefined the commands expand to **nothing**: the
  production build carries no calls at all, without touching a single line of
  code.

### An instance of your own

By default every application that uses `ot4xb_log.exe` writes to the **same
output device**: one viewer, one `.log`, everybody's lines mixed together. For
occasional messages and for debugging that is perfect - you launch one viewer
and see whatever happens. But to monitor one application in detail it becomes
unusable: your lines drown among everyone else's.

For that case ot4xb has a second group of functions:

```xbase
register_user_log_uuid( <cUserWndClassName> )
lSendLogStr( <cFormat> [, <params, ...> ] )
lSendLogStrFl( <cFile>, <nLine>, <cFormat> [, <params, ...> ] )
```

The prefix tells the two groups apart, and the difference is which identifier
they aim at:

- the `ot4xb_`-prefixed functions **always** use ot4xb's own UUID - the shared
  device described above;
- the ones without the prefix use **the UUID assigned at application level**,
  the one you set with `register_user_log_uuid`.

Register the class name once and from then on `lSendLogStr` / `lSendLogStrFl`
go only there. Then start a second instance of the viewer with that same class
name:

```
ot4xb_log.exe --class MYAPP_LOG_CLASS --title "My application" --log myapp --icon myapp.ico
```

That instance receives only your application's lines and keeps them in its own
`myapp.log`, while the default viewer, if it is running, carries on with
everybody else's. Its own title and icon make it recognisable in the tray when
several are open at once. The class name is what separates the instances (see
*Single instance*), so pick something unlikely to collide - a UUID is the
obvious choice.

### Two ways to switch logging on

1. **Compile time** - define `TRACE_ENABLED` (or whatever your own commands
   test) while developing and testing, and leave it undefined for the
   production build, as above.
2. **Run time** - keep the calls compiled but route them through a small
   function of your own that only calls `ot4xb_lSendLogStr...` when a program
   flag (command-line switch, ini entry, variable...) is set. This is the
   recommended shape for production: logging can be enabled and disabled
   without recompiling. Leaving the calls always active is also possible
   (without the viewer they make no noise), but that is each application's
   decision.

### The typical scenario

While developing you keep `ot4xb_log.exe` open and watch the lines arrive.
On a customer's machine the application runs as usual and the viewer is not
there. When something goes wrong and you need extra information, you launch
`ot4xb_log.exe` (a ~110 KB single file, no installation, no dependencies):
the DLL finds the window by its class UUID and from that moment every line
shows up in the viewer and is kept in the `.log`, with no recompile and no
restart of the application. Close the viewer and silence returns.

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

## Rules for agents working on this repo

- **Single source file.** All behaviour is in `source/ot4xb_log.cpp`; describe
  and change it from what the code does, not from assumptions about the ot4xb
  sender, whose source is not in this repository.
- **Wire protocol is frozen.** The `WM_APP+1` activate message, the
  `WM_COPYDATA` payload and the NUL-separated `.log` format are consumed by
  the ot4xb logging functions in deployed builds. Do not change them without
  an explicit request. The command line is not part of that contract - nothing
  launches the viewer automatically - but keep its options as documented.
- **x86 only.** Do not add an x64 configuration or "fix" the `DWORD` pointer
  casts unless asked; the tool is shipped as a 32-bit exe on purpose.
- **Docs are generated.** Edit `_mkskill/src/*.md`, then run `mkskill build`.
  `README.md`, `AGENTS.md` and `.claude/skills/ot4xb-log/SKILL.md` are outputs.
- **The version has one source.** `_mkskill/mkskill.config.xml` generates
  `source/ot4xb_log_version.h` and `source/ot4xb_log_version.props`; the `.rc`
  and the `.vcxproj` only consume them. Bump with `mkskill -vinc-*`, never by
  editing a number in the resource, the project file or the generated files.
- **Repository language is English.**
- **Git is the owner's job**: never commit, tag or push; leave the working tree
  ready and report.

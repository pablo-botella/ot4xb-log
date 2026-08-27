---
mkskill:
  pos: 10
  in: "*"
  replace-macros: true
---
# <$$$msk.name$$$>

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

Repository: <$$$msk.repo$$$>

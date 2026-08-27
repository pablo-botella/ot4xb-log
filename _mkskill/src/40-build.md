---
mkskill:
  pos: 40
  in: readme
---
## Building

- Open `ot4xb_log.sln` with Visual Studio 2022 (platform toolset **v143**,
  character set MultiByte). Configurations: `Debug|Win32` and `Release|Win32`.
- Output: `Release\ot4xb_log.exe` (and `Debug\ot4xb_log.exe` + `.pdb`).
- Links against `shlwapi.lib`; loads `Riched20.dll` at runtime.
- Version numbers come from two generated files, `source/ot4xb_log_version.h`
  (used by the `.rc`) and `source/ot4xb_log_version.props` (an MSBuild property
  sheet the project imports, which sets the linker's `/VERSION`). Both are
  written by mkskill from `_mkskill/mkskill.config.xml` - never edit them, and
  never hand-edit a version in the `.rc` or the `.vcxproj`.
- The code is **x86 only**: pointer arithmetic is done through `DWORD` casts
  (`_mk_ptr_`, the command-line parser), so an x64 configuration would need
  those rewritten to `UINT_PTR` first.

The build directories (`Debug/`, `Release/`, `.vs/`) are ignored by git; the
executable is distributed as a GitHub release asset, not committed.

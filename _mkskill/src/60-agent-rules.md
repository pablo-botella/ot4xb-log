---
mkskill:
  pos: 60
  in: ai*
---
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

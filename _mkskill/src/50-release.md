---
mkskill:
  pos: 50
  in: readme
---
## Releasing

The release number lives in `_mkskill/mkskill.config.xml` (`<version-spec>`)
and is the only place it is declared. From it mkskill generates two files that
the build consumes through standard mechanisms, so the number is never typed by
hand anywhere:

- `source/ot4xb_log_version.h` - included by `ot4xb_log.rc` for its
  `VS_VERSION_INFO` block (what Explorer shows: file and product version).
- `source/ot4xb_log_version.props` - an MSBuild property sheet imported by
  `ot4xb_log.vcxproj` (both configurations, through the standard
  `PropertySheets` import group). It sets `Link/Version`, i.e. the linker's
  `/VERSION:major.minor`, which stamps the PE header's image version. The
  linker only takes major and minor; the build component lives in the
  resource.

Both generated files are committed, so the project builds without mkskill
installed. Docs (this README, `AGENTS.md`, the Claude
skill) are composed from `_mkskill/src/*.md` by
[mkskill](https://github.com/pablo-botella/mkskill); never edit the generated
files directly.

```sh
# 1. bump the number, stamp the title: regenerates ot4xb_log_version.h and __publish.bat
mkskill -vinc-build -label:title "what shipped"
mkskill build            # the docs
# 2. build Release|Win32 in Visual Studio (picks up the new version header)
# 3. commit, then tag + push + GitHub release with the exe attached
__publish.bat
```

`__publish.bat` is generated from the config, git-ignored, and runs
`git tag`, `git push --tags` and `gh release create <tag> Release\ot4xb_log.exe`.

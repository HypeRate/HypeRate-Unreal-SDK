# Releasing HypeRate for Unreal Engine

This repository is the canonical source for both GitHub and Fab. The repository
root is also the plugin root, which lets users clone a tag directly into
`<Project>/Plugins/HypeRate`.

## Versioning

Keep these values synchronized:

- `VersionName` and integer `Version` in `HypeRate.uplugin`
- the current heading in `CHANGELOG.md`
- README badges and example tag

## Release checks

1. Run `./ReleaseTools/validate-plugin.sh`.
2. Compile against every advertised Unreal Engine version.
3. Run Unreal Automation tests matching `HypeRate.*`.
4. Test Blueprint and C++ consumption in a clean project.
5. Test Editor settings, connect, disconnect, reconnect, simulation, and widget.
6. Package Development and Shipping targets on every advertised platform.
7. Search the complete Git diff and packaged output for credentials.
8. Review current Fab technical requirements before submission.

An example command-line test run is:

```bash
UnrealEditor-Cmd YourProject.uproject \
  -unattended -nop4 -NullRHI \
  -ExecCmds="Automation RunTests HypeRate; Quit" \
  -TestExit="Automation Test Queue Empty"
```

## GitHub release

1. Merge the release branch and create an annotated tag such as `v1.0.0`.
2. Push the tag.
3. Run the release builder with the intended Engine installation:

```bash
./ReleaseTools/build-distributions.sh \
  /path/to/UE_5.8/Engine/Build/BatchFiles/RunUAT.sh
```

4. Attach the source zip and each version/platform-specific packaged zip to the
   GitHub Release.

The builder reads `Engine/Build/Build.version` and writes each binary package
below `Builds/<plugin-version>/UE_<major.minor>_<host-platform>`, preventing packages for
different Engine lines from being confused or overwritten.

Consumers can clone the source tag directly. Clearly label binary packages with
their Unreal Engine version and target platforms.

## Fab package

Fab Code Plugins require a `.uplugin`, at least one code module, and `Source`,
`Content`, `Config`, and `Resources` directories. The build script uses Epic's
`BuildPlugin` automation with strict includes, PCH disabled, and unity builds
disabled so missing includes cannot be hidden by neighboring source files. It
also disables UBA only for the release process to avoid the macOS `/var` versus
`/private/var` temporary-path alias issue. The output is filtered like a
distributable plugin.

For each supported Unreal Engine version:

1. Run the build with that Engine's `RunUAT`.
2. Inspect the package for `Binaries`, generated `Intermediate` metadata needed
   by the packaged plugin, source, content, config, resources, and descriptor.
3. Verify the resulting package in a clean Blueprint-only project.
4. Zip exactly one top-level `HypeRate` plugin folder.
5. Upload through Fab using a download link that does not require sign-in.
6. Complete version notes, supported platforms, third-party service disclosure,
   and the technical details from `Docs/fab-listing.md`.

Do not upload repository tooling, local settings, IDE output, tokens, sample
secrets, or an entire host project as the Code Plugin product.


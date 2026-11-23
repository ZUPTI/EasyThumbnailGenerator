# Release Checklist

Use this checklist for every Easy Thumbnail Generator release.

- [ ] Update `Version` and `VersionName` in `EasyThumbnailGenerator.uplugin`.
- [ ] Add the release to `CHANGELOG.md`.
- [ ] Update `README.md` when user-facing behavior, UI, installation, compatibility, or supported features changed.
- [ ] Confirm `LICENSE` and copyright information are still correct.
- [ ] Build the plugin against the maintained Unreal Engine version.
- [ ] Verify compilation with **Unity Build enabled** and **non-Unity compilation**, where practical (including material-helper symbol collisions).
- [ ] Test at least one Static Mesh and one Skeletal Mesh.
- [ ] Verify Actor Blueprint menu entry appears for supported Actor Blueprints and not unrelated Blueprint types.
- [ ] Verify Actor Blueprint previews and PNG/Texture output with multiple Static Mesh and Skeletal Mesh components, offsets, rotations and material overrides.
- [ ] Confirm Actor bounds exclude invisible/unassigned mesh components and empty Actors fail with a clear message.
- [ ] Verify reference pose, selected Animation Asset and nonzero animation time on Skeletal Mesh assets.
- [ ] Verify the same pose in live preview vs Generate Current and Generate All; check incompatible skeleton handling.
- [ ] Verify pose settings persist through saved presets and Last Used, including selecting presets from the dropdown.
- [ ] Verify the preview Actor is destroyed on asset changes and no Actor is spawned into the editor gameplay level.
- [ ] Test `Generate Current` and transparent alpha output.
- [ ] Verify **Fixed EV100** changes match between the live preview and exported PNG.
- [ ] Verify skylight intensity changes match between the live preview and exported PNG.
- [ ] Test opaque, masked, translucent and additive material output.
- [ ] Test at least one Nanite-enabled Static Mesh and one non-Nanite Static Mesh.
- [ ] Verify **Fit to Frame** keeps the full asset visible in Front/Back/Left/Right/Top/Bottom and 3/4 views, especially in a wide preview window.
- [ ] Verify **Visible Content** produces a tighter, visually centered fit on an asymmetric/curved mesh, and compare it with Bounds Center / Pivot / Custom Offset.
- [ ] Verify geometry-aware framing works on both Static Mesh and Skeletal Mesh assets and falls back safely when CPU geometry data is unavailable.
- [ ] Test multi-selection / `Generate All` when batch behavior changed.
- [ ] Test saved presets and last-used settings when configuration behavior changed.
- [ ] Verify legacy PNG defaults under `Saved/Thumbnails`.
- [ ] Verify `SM_Rock` + source prefix `SM_` + output prefix `T_` produces `T_Rock` in PNG, Texture Asset, and Both modes.
- [ ] Verify filename tokens `{AssetName}`, `{BaseName}`, `{Prefix}` and reject invalid template characters.
- [ ] Verify custom absolute and project-relative PNG directories and /Game Texture Asset directory.
- [ ] Verify Auto Increment, Overwrite and Skip Existing for PNG, Texture Asset and Both; confirm other asset types cannot be overwritten.
- [ ] Verify output destinations, prefixes, templates, policy, and mode persist through Save/Load Preset and Last Used.
- [ ] Verify Content Browser discovers the generated Texture2D, transparency is preserved, and overwrite updates the existing asset.
- [ ] Verify Generate Current and Generate All with PNG / Texture Asset / Both and failed/skipped file reporting.
- [ ] Create the release ZIP from the clean source tree without `Binaries`, `Intermediate`, `Saved`, or IDE-generated files.
- [ ] Use the matching `CHANGELOG.md` section as the basis for GitHub Release notes.

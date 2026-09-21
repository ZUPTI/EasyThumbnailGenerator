# Release Checklist

Use this checklist for every Easy Thumbnail Generator release.

- [ ] Update `Version` and `VersionName` in `EasyThumbnailGenerator.uplugin`.
- [ ] Add the release to `CHANGELOG.md`.
- [ ] Update `README.md` when user-facing behavior, UI, installation, compatibility, or supported features changed.
- [ ] Confirm `LICENSE` and copyright information are still correct.
- [ ] Build the plugin against the maintained Unreal Engine version.
- [ ] Test at least one Static Mesh and one Skeletal Mesh.
- [ ] Test `Generate Current` and transparent alpha output.
- [ ] Verify **Fixed EV100** changes match between the live preview and exported PNG.
- [ ] Verify skylight intensity changes match between the live preview and exported PNG.
- [ ] Test opaque, masked, translucent and additive material output.
- [ ] Test at least one Nanite-enabled Static Mesh and one non-Nanite Static Mesh.
- [ ] Verify **Fit to Frame** keeps the full asset visible in Front/Back/Left/Right/Top/Bottom and 3/4 views, especially in a wide preview window.
- [ ] Test multi-selection / `Generate All` when batch behavior changed.
- [ ] Test saved presets and last-used settings when configuration behavior changed.
- [ ] Verify output under `Saved/Thumbnails`.
- [ ] Create the release ZIP from the clean source tree without `Binaries`, `Intermediate`, `Saved`, or IDE-generated files.
- [ ] Use the matching `CHANGELOG.md` section as the basis for GitHub Release notes.

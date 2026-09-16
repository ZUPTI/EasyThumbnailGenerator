# Changelog

All notable changes to **Easy Thumbnail Generator** are documented here.

The project follows semantic versioning where practical:

- **Patch** releases fix bugs and compatibility issues.
- **Minor** releases add backwards-compatible features.
- **Major** releases may introduce breaking workflow or API changes.

## [1.3.0] - 2026-09-16

### Added

- Named user presets with Save, Load, Rename and Delete controls.
- Per-user/per-project persistence for saved presets.
- Automatic restoration of the last-used thumbnail settings.
- Camera shortcut buttons for Front, Back, Left, Right, Top, Bottom, 3/4 Left and 3/4 Right views.
- Previous/next navigation when multiple meshes are selected.
- Separate **Generate Current** and **Generate All** actions.
- Exact live viewport camera transfer for **Generate Current**, including camera location, rotation and orthographic zoom.

### Changed

- Batch generation now fits every selected asset independently instead of reusing the first asset's absolute camera distance.
- Lighting/exposure changes no longer unnecessarily refit the camera.
- README expanded to document the live-preview, preset and batch workflows.
- `CHANGELOG.md` is now the canonical release-notes document and should be updated for every release.

## [1.2.2] - 2026-09-16

### Fixed

- Added the RenderCore module dependency required by `FlushRenderingCommands`.
- Updated viewport realtime handling to the UE 5.8 realtime-override API.

## [1.2.1] - 2026-09-16

### Fixed

- Removed the invalid `SEditorViewport::MakeViewportToolbar` override after the UE 5.8 API made it final.
- Corrected Slate box-panel includes for UE 5.8.

## [1.2.0] - 2026-09-16

### Added

- Live thumbnail preview window.
- In-window camera, lighting, exposure and framing controls.
- Built-in Weapon Side, Weapon 3/4 and Asset Default presets.
- Generate button from the preview workflow.

### Changed

- Removed the separate Project Settings workflow in favor of per-session preview controls.

## [1.1.1] - 2026-09-16

### Fixed

- Explicitly applied Static Mesh and Skeletal Mesh material slots to preview components.
- Improved material/texture readiness before capture.
- Enabled material, lighting, tonemapper and reflection-related capture flags.

## [1.1.0] - 2026-09-16

### Added

- Configurable camera, projection, lighting and exposure settings.
- Perspective rendering option.

### Changed

- Improved editor-like lighting compared with the initial minimal preview scene.

## [1.0.0] - 2026-09-16

### Added

- Initial UE 5.8 editor-only C++ plugin.
- Static Mesh and Skeletal Mesh Content Browser context-menu integration.
- Transparent PNG generation to `Saved/Thumbnails/<AssetName>.png`.
- 1024x1024 default output.
- Automatic fit-to-frame.

# Easy Thumbnail Generator

Native Unreal Engine editor plugin by **ZUPTI** for generating transparent PNG thumbnails from **Static Mesh** and **Skeletal Mesh** assets.

> **Target:** Unreal Engine 5.8  
> **Current version:** 1.3.0  
> **Type:** Editor-only C++ plugin  
> **License:** MIT

## Overview

Easy Thumbnail Generator adds **Create PNG Thumbnail...** to the Content Browser context menu for Static Mesh and Skeletal Mesh assets.

The action opens a live editor preview where you can frame the asset, switch camera angles, tune lighting/exposure, save reusable presets, and generate transparent PNG thumbnails without leaving Unreal Editor.

Output is written to:

```text
<Project>/Saved/Thumbnails/<AssetName>.png
```

## Features

- Unreal Engine 5.8 native C++ editor plugin
- Static Mesh and Skeletal Mesh support
- Content Browser right-click integration
- Live 3D preview before export
- Transparent PNG output
- Material-aware rendering
- Perspective and orthographic projection
- Automatic fit-to-frame
- Mouse orbit/pan/zoom in the preview viewport
- Camera shortcuts:
  - Front
  - Back
  - Left
  - Right
  - Top
  - Bottom
  - 3/4 Left
  - 3/4 Right
- Built-in starting presets:
  - **Weapon Side**
  - **Weapon 3/4**
  - **Asset Default**
  - **Custom**
- Named user presets with **Save / Load / Rename / Delete**
- Last-used settings are restored automatically per user/per project
- Multi-selection navigation with previous/next asset controls
- **Generate Current** preserves the current live viewport camera framing
- **Generate All** fits each selected asset independently while reusing the current camera direction and render settings
- Configurable:
  - output resolution
  - frame padding
  - projection mode
  - FOV
  - camera yaw/pitch
  - directional light
  - skylight
  - manual exposure

## Installation

Clone or copy this repository into the project's Plugins directory:

```text
<Project>/Plugins/EasyThumbnailGenerator/
```

Expected structure:

```text
<Project>/
└── Plugins/
    └── EasyThumbnailGenerator/
        ├── EasyThumbnailGenerator.uplugin
        ├── README.md
        ├── CHANGELOG.md
        ├── LICENSE
        ├── RELEASE_CHECKLIST.md
        └── Source/
```

Regenerate project files if needed, then build your Editor target with Unreal Engine 5.8.

## Usage

1. Open the Content Browser.
2. Select one or more **Static Mesh** or **Skeletal Mesh** assets.
3. Right-click and choose **Create PNG Thumbnail...**.
4. Adjust the live preview.
5. Optionally use a built-in preset, camera shortcut, or saved user preset.
6. Click **Fit to Frame** when you want automatic framing.
7. Use:
   - **Generate Current** for the currently previewed asset using the live viewport camera framing.
   - **Generate All** for batch generation across the current selection.
8. Find the PNG files in:

```text
Saved/Thumbnails/
```

## Preview Workflow

The preview window intentionally owns the working settings. There is no separate plugin page under Project Settings.

### Built-in presets

**Weapon Side** is a side-profile starting point intended for weapon/catalog thumbnails.

**Weapon 3/4** uses a three-quarter view to expose more depth and silhouette.

**Asset Default** is a general-purpose starting point for arbitrary meshes.

Changing an individual setting switches the session to **Custom**.

### Saved presets

The **Saved Presets** panel lets you give the current setup a name and save it for later.

Saved presets and the last-used settings are stored as editor per-user/per-project configuration. They are not asset files and do not need to be committed to source control.

### Camera shortcuts

Camera shortcut buttons immediately change the view and refit the current asset. They are useful for quickly exploring a consistent silhouette before fine-tuning with the mouse.

## Current vs Batch Generation

### Generate Current

`Generate Current` transfers the current preview camera location, rotation, FOV/orthographic zoom, lighting and output settings to the PNG renderer. This is intended to keep the generated framing as close as possible to what you composed in the live viewport.

### Generate All

`Generate All` uses the current camera direction, projection, lighting and output settings, but automatically fits each selected mesh to frame. This avoids applying the first asset's absolute camera distance to differently sized meshes.

## Transparent Background

The live preview uses an editor preview scene for useful lighting and material evaluation. The final PNG is rendered separately with an alpha-aware capture path, so the exported image keeps a transparent background rather than capturing the editor window itself.

## Multi-Selection

When multiple supported meshes are selected, use the `<` and `>` buttons to inspect them individually.

Example output:

```text
Saved/Thumbnails/SM_Example.png
Saved/Thumbnails/SK_Example.png
```

## Source Layout

```text
EasyThumbnailGenerator/
├── EasyThumbnailGenerator.uplugin
├── README.md
├── CHANGELOG.md
├── LICENSE
├── RELEASE_CHECKLIST.md
└── Source/
    └── EasyThumbnailGenerator/
        ├── EasyThumbnailGenerator.Build.cs
        ├── Public/
        │   ├── EasyThumbnailGeneratorModule.h
        │   ├── EasyThumbnailGeneratorSessionSettings.h
        │   └── EasyThumbnailGeneratorUserSettings.h
        └── Private/
            ├── EasyThumbnailGeneratorModule.cpp
            ├── EasyThumbnailGeneratorRenderer.cpp
            ├── EasyThumbnailGeneratorSessionSettings.cpp
            ├── EasyThumbnailGeneratorUserSettings.cpp
            ├── SEasyThumbnailGeneratorWindow.cpp
            └── SEasyThumbnailGeneratorWindow.h
```

The plugin is editor-only and does not add runtime dependencies to packaged games.

## Compatibility

| Unreal Engine | Status |
| --- | --- |
| 5.8 | Target / maintained |
| Earlier versions | Not currently maintained |

## Release Notes

See [`CHANGELOG.md`](CHANGELOG.md) for version-by-version release notes. Maintainers can use [`RELEASE_CHECKLIST.md`](RELEASE_CHECKLIST.md) to keep version metadata, release notes, documentation and validation in sync.

## Roadmap

Potential future improvements include:

- Checkerboard/transparent preview visualization
- Advanced preview-environment profiles
- Better preview/output shading parity for complex Static Mesh materials
- Output folder and filename templates
- Overwrite policy controls
- Asset-type-specific controls such as Static Mesh LOD/Nanite and Skeletal Mesh pose options

## Contributing

Issues and pull requests are welcome. For rendering or build issues, please include:

- Unreal Engine version
- Plugin version
- Asset type (Static Mesh or Skeletal Mesh)
- Relevant build/editor log
- Preview and generated-output screenshots when the issue is visual

## License

Easy Thumbnail Generator is released under the **MIT License**. See [`LICENSE`](LICENSE).

Unreal Engine and its associated trademarks are the property of Epic Games, Inc. This project is independent and is not affiliated with or endorsed by Epic Games.

## About

Developed by **ZUPTI**.

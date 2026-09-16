# Easy Thumbnail Generator

Native Unreal Engine editor plugin by **ZUPTI** for generating transparent PNG thumbnails from **Static Mesh** and **Skeletal Mesh** assets.

> **Target:** Unreal Engine 5.8  
> **Current version:** 1.2.1  
> **Type:** Editor-only C++ plugin  
> **License:** MIT

## Overview

Easy Thumbnail Generator adds a **Create PNG Thumbnail...** action to the Content Browser context menu for Static Mesh and Skeletal Mesh assets.

Instead of immediately exporting an image, the plugin opens a live editor preview where you can frame the asset, choose a camera preset, tune lighting and exposure, and then generate the final PNG.

The exported image keeps the mesh materials and uses a **transparent background**.

## Features

- Native C++ editor plugin for Unreal Engine 5.8
- Content Browser context-menu integration
- Supports Static Mesh and Skeletal Mesh assets
- Live 3D preview before export
- Perspective and orthographic projection modes
- Camera controls with automatic fit-to-frame
- Configurable output resolution and frame padding
- Directional light, skylight and exposure controls
- Built-in presets:
  - **Weapon Side**
  - **Weapon 3/4**
  - **Asset Default**
  - **Custom**
- Material-aware rendering
- Transparent PNG output
- Multi-selection support: preview the first selected asset and generate thumbnails for all selected supported assets
- Output written to:

```text
<Project>/Saved/Thumbnails/<AssetName>.png
```

## Installation

Clone or copy the repository into your Unreal project Plugins directory:

```text
<Project>/Plugins/EasyThumbnailGenerator/
```

The resulting structure should look like:

```text
<Project>/
└── Plugins/
    └── EasyThumbnailGenerator/
        ├── EasyThumbnailGenerator.uplugin
        └── Source/
```

Regenerate project files if needed, then build your Editor target with Unreal Engine 5.8.

## Usage

1. Open the Content Browser.
2. Right-click a **Static Mesh** or **Skeletal Mesh** asset.
3. Choose **Create PNG Thumbnail...**.
4. Adjust the live preview using a preset or custom camera/light settings.
5. Use **Fit to Frame** when needed.
6. Click **Generate PNG**.
7. Find the result in:

```text
Saved/Thumbnails/
```

## Preview Controls

The preview window exposes the settings needed for thumbnail creation directly in the workflow, so there is no separate Project Settings page to maintain.

### Output

- Output Resolution
- Frame Padding

### Camera

- Projection Mode
- Yaw
- Pitch
- Perspective FOV

### Lighting

- Directional Light Intensity
- Directional Light Yaw
- Directional Light Pitch
- Sky Light Intensity

### Post Process

- Manual Exposure
- Exposure Compensation

Changing a preset value manually switches the session to **Custom**.

## Presets

### Weapon Side

Designed for clean side-profile weapon thumbnails and catalog-style presentation.

### Weapon 3/4

Uses a three-quarter camera angle to expose more of the asset volume and silhouette.

### Asset Default

A general-purpose starting point for meshes that are not weapon-shaped.

## Transparent Background

The preview scene can use lighting and shading needed to evaluate the asset, while the final PNG is generated with a transparent background. The export path does not rely on simply capturing the editor window.

## Multi-Selection

When multiple supported meshes are selected, the preview window displays the first selection. **Generate PNG** applies the current render settings to every selected supported asset.

Each asset is written using its Unreal asset name:

```text
Saved/Thumbnails/SM_Example.png
Saved/Thumbnails/SK_Example.png
```

## Source Layout

```text
EasyThumbnailGenerator/
├── EasyThumbnailGenerator.uplugin
└── Source/
    └── EasyThumbnailGenerator/
        ├── EasyThumbnailGenerator.Build.cs
        ├── Public/
        └── Private/
```

The plugin is intentionally editor-only and does not add runtime dependencies to packaged games.

## Compatibility

| Unreal Engine | Status |
| --- | --- |
| 5.8 | Target version |
| Earlier versions | Not currently maintained |

## Roadmap

Possible future additions include:

- Saved user presets
- Persisting last-used session settings
- Additional asset-specific camera presets
- Checkerboard/background preview options
- Expanded preview environment controls

## Contributing

Issues and pull requests are welcome. When reporting a rendering or build issue, please include:

- Unreal Engine version
- Asset type (Static Mesh or Skeletal Mesh)
- Plugin version
- Relevant build or editor log
- A screenshot of the preview/output when the issue is visual

## License

Easy Thumbnail Generator is released under the **MIT License**. See [`LICENSE`](LICENSE) for the full license text.

Unreal Engine and its associated trademarks are the property of Epic Games, Inc. This project is an independent plugin and is not affiliated with or endorsed by Epic Games.

## About

Developed by **ZUPTI**.

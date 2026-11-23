#pragma once

#include "CoreMinimal.h"

class AActor;
class UObject;
class UWorld;
class USkeletalMeshComponent;
struct FEasyThumbnailGeneratorRenderSettings;

namespace EasyThumbnailGenerator
{
    // Actor Blueprints are instantiated only inside the transient thumbnail preview world.
    bool IsSupportedActorBlueprint(const UObject* Asset);
    AActor* SpawnPreviewActor(UObject* Asset, UWorld* PreviewWorld);

    // Return mesh-component bounds in actor-pivot space (independent of its world offset).
    bool GetActorMeshBounds(const AActor* Actor, FBoxSphereBounds& OutBounds);

    // Reference pose is used when no animation is selected; the preview never plays.
    void ApplyPoseToSkeletalMesh(
        USkeletalMeshComponent* MeshComponent,
        const FEasyThumbnailGeneratorRenderSettings& Settings);

    void ApplyPoseToActor(
        AActor* Actor,
        const FEasyThumbnailGeneratorRenderSettings& Settings);
}

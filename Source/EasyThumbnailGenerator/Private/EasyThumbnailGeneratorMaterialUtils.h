#pragma once

class UStaticMesh;
class USkeletalMesh;
class UStaticMeshComponent;
class USkeletalMeshComponent;

namespace EasyThumbnailGenerator
{
    // Apply an asset's assigned materials to its temporary preview/capture component.
    // Shared definitions live in one .cpp so Unity and non-Unity builds behave alike.
    void ApplyStaticMeshMaterials(UStaticMeshComponent* MeshComponent, const UStaticMesh* StaticMesh);
    void ApplySkeletalMeshMaterials(USkeletalMeshComponent* MeshComponent, const USkeletalMesh* SkeletalMesh);
}

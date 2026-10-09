#include "EasyThumbnailGeneratorMaterialUtils.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

namespace EasyThumbnailGenerator
{
    void ApplyStaticMeshMaterials(UStaticMeshComponent* MeshComponent, const UStaticMesh* StaticMesh)
    {
        if (!MeshComponent || !StaticMesh)
        {
            return;
        }

        const TArray<FStaticMaterial>& Materials = StaticMesh->GetStaticMaterials();
        for (int32 MaterialIndex = 0; MaterialIndex < Materials.Num(); ++MaterialIndex)
        {
            if (UMaterialInterface* Material = Materials[MaterialIndex].MaterialInterface)
            {
                MeshComponent->SetMaterial(MaterialIndex, Material);
            }
        }
    }

    void ApplySkeletalMeshMaterials(USkeletalMeshComponent* MeshComponent, const USkeletalMesh* SkeletalMesh)
    {
        if (!MeshComponent || !SkeletalMesh)
        {
            return;
        }

        const TArray<FSkeletalMaterial>& Materials = SkeletalMesh->GetMaterials();
        for (int32 MaterialIndex = 0; MaterialIndex < Materials.Num(); ++MaterialIndex)
        {
            if (UMaterialInterface* Material = Materials[MaterialIndex].MaterialInterface)
            {
                MeshComponent->SetMaterial(MaterialIndex, Material);
            }
        }
    }
}

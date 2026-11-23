#include "EasyThumbnailGeneratorActorUtils.h"

#include "Animation/AnimationAsset.h"
#include "Components/ActorComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EasyThumbnailGeneratorSessionSettings.h"
#include "Engine/Blueprint.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

namespace EasyThumbnailGenerator
{
    bool IsSupportedActorBlueprint(const UObject* Asset)
    {
        const UBlueprint* Blueprint = Cast<UBlueprint>(Asset);
        return Blueprint && Blueprint->GeneratedClass &&
            Blueprint->GeneratedClass->IsChildOf(AActor::StaticClass()) &&
            !Blueprint->GeneratedClass->HasAnyClassFlags(CLASS_Abstract);
    }

    AActor* SpawnPreviewActor(UObject* Asset, UWorld* PreviewWorld)
    {
        if (!PreviewWorld || !IsSupportedActorBlueprint(Asset))
        {
            return nullptr;
        }

        const UBlueprint* Blueprint = CastChecked<UBlueprint>(Asset);

        FActorSpawnParameters Parameters;
        Parameters.SpawnCollisionHandlingOverride =
            ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        Parameters.ObjectFlags |= RF_Transient;
        Parameters.bTemporaryEditorActor = true;

        AActor* Actor = PreviewWorld->SpawnActor<AActor>(
            Blueprint->GeneratedClass,
            FVector::ZeroVector,
            FRotator::ZeroRotator,
            Parameters);

        if (!Actor)
        {
            return nullptr;
        }

        // Capture a static construction-script result, not live gameplay or animation.
        Actor->SetActorTickEnabled(false);
        TArray<UActorComponent*> Components;
        Actor->GetComponents(Components);
        for (UActorComponent* Component : Components)
        {
            if (!Component)
            {
                continue;
            }

            Component->SetComponentTickEnabled(false);

            if (UStaticMeshComponent* StaticMesh = Cast<UStaticMeshComponent>(Component))
            {
                StaticMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                StaticMesh->SetCastShadow(true);
                StaticMesh->SetTextureForceResidentFlag(true);
            }
            else if (USkeletalMeshComponent* SkeletalMesh = Cast<USkeletalMeshComponent>(Component))
            {
                SkeletalMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
                SkeletalMesh->SetCastShadow(true);
                SkeletalMesh->SetTextureForceResidentFlag(true);
            }
            // Other actor components retain their authored visibility; only the
            // Static/Skeletal Mesh components participate in framing bounds.
        }

        return Actor;
    }

    bool GetActorMeshBounds(const AActor* Actor, FBoxSphereBounds& OutBounds)
    {
        if (!IsValid(Actor))
        {
            return false;
        }

        TArray<UActorComponent*> Components;
        Actor->GetComponents(Components);

        FBox UnionBounds(EForceInit::ForceInit);
        bool bHasMesh = false;
        for (UActorComponent* Component : Components)
        {
            UPrimitiveComponent* Mesh = nullptr;
            if (UStaticMeshComponent* StaticMesh = Cast<UStaticMeshComponent>(Component))
            {
                if (StaticMesh->GetStaticMesh())
                {
                    Mesh = StaticMesh;
                }
            }
            else if (USkeletalMeshComponent* SkeletalMesh = Cast<USkeletalMeshComponent>(Component))
            {
                if (SkeletalMesh->GetSkeletalMeshAsset())
                {
                    Mesh = SkeletalMesh;
                }
            }

            if (!Mesh || !Mesh->IsVisible())
            {
                continue;
            }

            Mesh->UpdateBounds();
            UnionBounds += Mesh->Bounds.GetBox();
            bHasMesh = true;
        }

        if (!bHasMesh || !UnionBounds.IsValid)
        {
            return false;
        }

        // Actor pivot may be offset when preview framing is updated.
        OutBounds = FBoxSphereBounds(UnionBounds.ShiftBy(-Actor->GetActorLocation()));
        return true;
    }

    void ApplyPoseToSkeletalMesh(
        USkeletalMeshComponent* MeshComponent,
        const FEasyThumbnailGeneratorRenderSettings& Settings)
    {
        if (!MeshComponent || !MeshComponent->GetSkeletalMeshAsset())
        {
            return;
        }

        UAnimationAsset* Animation = Settings.AnimationAsset.LoadSynchronous();
        if (!Animation)
        {
            MeshComponent->bForceRefpose = true;
        }
        else if (Animation->GetSkeleton() == MeshComponent->GetSkeletalMeshAsset()->GetSkeleton())
        {
            MeshComponent->bForceRefpose = false;
            MeshComponent->SetAnimationMode(EAnimationMode::AnimationSingleNode);
            MeshComponent->SetAnimation(Animation);
            MeshComponent->SetPosition(
                FMath::Clamp(Settings.AnimationTimeSeconds, 0.0f, Animation->GetPlayLength()),
                false);
            MeshComponent->TickAnimation(0.0f, false);
        }
        else
        {
            // A pose from an incompatible skeleton must never deform this mesh.
            MeshComponent->bForceRefpose = true;
        }

        MeshComponent->RefreshBoneTransforms();
        MeshComponent->UpdateComponentToWorld();
        MeshComponent->UpdateBounds();
        MeshComponent->MarkRenderStateDirty();
    }

    void ApplyPoseToActor(
        AActor* Actor,
        const FEasyThumbnailGeneratorRenderSettings& Settings)
    {
        if (!IsValid(Actor))
        {
            return;
        }

        TArray<UActorComponent*> Components;
        Actor->GetComponents(Components);
        for (UActorComponent* Component : Components)
        {
            if (USkeletalMeshComponent* Mesh = Cast<USkeletalMeshComponent>(Component))
            {
                ApplyPoseToSkeletalMesh(Mesh, Settings);
            }
        }
    }
}

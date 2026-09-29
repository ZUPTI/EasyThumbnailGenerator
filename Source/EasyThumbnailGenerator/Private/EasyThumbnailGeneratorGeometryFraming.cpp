#include "EasyThumbnailGeneratorGeometryFraming.h"

#include "EasyThumbnailGeneratorFraming.h"

#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Rendering/SkeletalMeshLODRenderData.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "StaticMeshResources.h"

namespace EasyThumbnailGenerator
{
    namespace
    {
        bool GatherStaticMeshPositions(
            const UStaticMesh* StaticMesh,
            TArray<FVector>& OutPositions)
        {
            if (!StaticMesh)
            {
                return false;
            }

            const FStaticMeshRenderData* RenderData = StaticMesh->GetRenderData();
            if (!RenderData || RenderData->LODResources.Num() == 0)
            {
                return false;
            }

            const FStaticMeshLODResources& LODResources = RenderData->LODResources[0];
            const FPositionVertexBuffer& PositionBuffer =
                LODResources.VertexBuffers.PositionVertexBuffer;

            const uint32 VertexCount = LODResources.GetNumVertices();
            if (VertexCount == 0)
            {
                return false;
            }

            OutPositions.Reset();
            OutPositions.Reserve(static_cast<int32>(VertexCount));

            for (uint32 VertexIndex = 0; VertexIndex < VertexCount; ++VertexIndex)
            {
                OutPositions.Add(FVector(PositionBuffer.VertexPosition(VertexIndex)));
            }

            return !OutPositions.IsEmpty();
        }

        bool GatherSkeletalMeshPositions(
            USkeletalMesh* SkeletalMesh,
            USkeletalMeshComponent* SkeletalMeshComponent,
            TArray<FVector>& OutPositions)
        {
            if (!SkeletalMesh || !SkeletalMeshComponent)
            {
                return false;
            }

            FSkeletalMeshRenderData* RenderData = SkeletalMesh->GetResourceForRendering();
            if (!RenderData || RenderData->LODRenderData.Num() == 0)
            {
                return false;
            }

            const FSkeletalMeshLODRenderData& LODData = RenderData->LODRenderData[0];
            const FSkinWeightVertexBuffer* SkinWeightBuffer =
                LODData.GetSkinWeightVertexBuffer();

            const uint32 VertexCount = LODData.GetNumVertices();
            if (!SkinWeightBuffer || VertexCount == 0)
            {
                return false;
            }

            TArray<FMatrix44f> CachedRefToLocals;
            SkeletalMeshComponent->GetCurrentRefToLocalMatrices(
                CachedRefToLocals,
                0,
                nullptr);

            OutPositions.Reset();
            OutPositions.Reserve(static_cast<int32>(VertexCount));

            for (uint32 VertexIndex = 0; VertexIndex < VertexCount; ++VertexIndex)
            {
                const FVector3f SkinnedPosition =
                    USkeletalMeshComponent::GetSkinnedVertexPosition(
                        SkeletalMeshComponent,
                        static_cast<int32>(VertexIndex),
                        LODData,
                        *SkinWeightBuffer,
                        CachedRefToLocals);

                OutPositions.Add(FVector(SkinnedPosition));
            }

            return !OutPositions.IsEmpty();
        }

        bool GatherGeometryPositions(
            UObject* Asset,
            UPrimitiveComponent* MeshComponent,
            TArray<FVector>& OutPositions)
        {
            if (const UStaticMesh* StaticMesh = Cast<UStaticMesh>(Asset))
            {
                return GatherStaticMeshPositions(StaticMesh, OutPositions);
            }

            if (USkeletalMesh* SkeletalMesh = Cast<USkeletalMesh>(Asset))
            {
                return GatherSkeletalMeshPositions(
                    SkeletalMesh,
                    Cast<USkeletalMeshComponent>(MeshComponent),
                    OutPositions);
            }

            return false;
        }
    }

    bool CalculateGeometryAwareFit(
        UObject* Asset,
        UPrimitiveComponent* MeshComponent,
        const FBoxSphereBounds& AssetBounds,
        const FEasyThumbnailGeneratorRenderSettings& Settings,
        const FRotator& CameraRotation,
        float ViewportAspectRatio,
        float PaddingMultiplier,
        float MinimumCameraDistance,
        FGeometryFitResult& OutResult)
    {
        OutResult = FGeometryFitResult();

        TArray<FVector> Positions;
        if (!GatherGeometryPositions(Asset, MeshComponent, Positions))
        {
            return false;
        }

        const float SafeAspectRatio = FMath::Max(ViewportAspectRatio, 0.01f);
        const float SafePadding = FMath::Max(PaddingMultiplier, 1.0f);

        const FVector FrameCenter = GetFramingCenter(AssetBounds, Settings);

        const FRotationMatrix CameraMatrix(CameraRotation);
        const FVector CameraForward = CameraMatrix.GetUnitAxis(EAxis::X);
        const FVector CameraRight = CameraMatrix.GetUnitAxis(EAxis::Y);
        const FVector CameraUp = CameraMatrix.GetUnitAxis(EAxis::Z);

        float MinRight = TNumericLimits<float>::Max();
        float MaxRight = -TNumericLimits<float>::Max();
        float MinUp = TNumericLimits<float>::Max();
        float MaxUp = -TNumericLimits<float>::Max();

        for (const FVector& Position : Positions)
        {
            const FVector RelativePosition = Position - FrameCenter;
            const float Right = FVector::DotProduct(RelativePosition, CameraRight);
            const float Up = FVector::DotProduct(RelativePosition, CameraUp);

            MinRight = FMath::Min(MinRight, Right);
            MaxRight = FMath::Max(MaxRight, Right);
            MinUp = FMath::Min(MinUp, Up);
            MaxUp = FMath::Max(MaxUp, Up);
        }

        FVector ViewTarget = FVector::ZeroVector;

        if (Settings.FramingMode == EEasyThumbnailGeneratorFramingMode::VisibleContent)
        {
            const float CenterRight = (MinRight + MaxRight) * 0.5f;
            const float CenterUp = (MinUp + MaxUp) * 0.5f;
            ViewTarget = (CameraRight * CenterRight) + (CameraUp * CenterUp);
        }

        float MaxAbsRight = 0.0f;
        float MaxAbsUp = 0.0f;
        float MaxAbsDepth = 0.0f;

        const float HorizontalFOVRadians =
            FMath::DegreesToRadians(FMath::Clamp(Settings.PerspectiveFOV, 1.0f, 170.0f));
        const float VerticalFOVRadians = 2.0f * FMath::Atan(
            FMath::Tan(HorizontalFOVRadians * 0.5f) / SafeAspectRatio);

        const float TanHalfHorizontal =
            FMath::Max(FMath::Tan(HorizontalFOVRadians * 0.5f), KINDA_SMALL_NUMBER);
        const float TanHalfVertical =
            FMath::Max(FMath::Tan(VerticalFOVRadians * 0.5f), KINDA_SMALL_NUMBER);

        float RequiredPerspectiveDistance = MinimumCameraDistance;

        for (const FVector& Position : Positions)
        {
            const FVector RelativePosition = Position - FrameCenter - ViewTarget;

            const float Right =
                FVector::DotProduct(RelativePosition, CameraRight);
            const float Up =
                FVector::DotProduct(RelativePosition, CameraUp);
            const float Depth =
                FVector::DotProduct(RelativePosition, CameraForward);

            const float AbsRight = FMath::Abs(Right);
            const float AbsUp = FMath::Abs(Up);

            MaxAbsRight = FMath::Max(MaxAbsRight, AbsRight);
            MaxAbsUp = FMath::Max(MaxAbsUp, AbsUp);
            MaxAbsDepth = FMath::Max(MaxAbsDepth, FMath::Abs(Depth));

            // Camera is ViewTarget - Forward * Distance. Therefore each vertex has
            // camera-space depth (Distance + Depth). Solve the FOV inequalities
            // directly instead of expanding an axis-aligned 3D bounds box.
            const float RequiredFromWidth =
                (AbsRight * SafePadding / TanHalfHorizontal) - Depth;
            const float RequiredFromHeight =
                (AbsUp * SafePadding / TanHalfVertical) - Depth;

            RequiredPerspectiveDistance = FMath::Max(
                RequiredPerspectiveDistance,
                FMath::Max(RequiredFromWidth, RequiredFromHeight));
        }

        const float RequiredOrthoHalfWidth = FMath::Max(
            MaxAbsRight,
            MaxAbsUp * SafeAspectRatio);

        OutResult.bValid = true;
        OutResult.ViewTarget = ViewTarget;
        OutResult.PerspectiveDistance =
            FMath::Max(RequiredPerspectiveDistance, MinimumCameraDistance);
        OutResult.OrthoWidth =
            FMath::Max(RequiredOrthoHalfWidth * 2.0f * SafePadding, 2.0f);
        OutResult.OrthoCameraDistance =
            FMath::Max(
                MinimumCameraDistance,
                (MaxAbsDepth * SafePadding) + 100.0f);

        return true;
    }
}

// Copyright 2019-Present tarnishablec. All Rights Reserved.

#include "PinecoTrace/Trace/PinecoTraceLibrary.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/OverlapResult.h"
#include "GameFramework/Actor.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/SkeletalBodySetup.h"

FHitResult UPinecoTraceLibrary::MakeActorResult(AActor* Actor)
{
    FHitResult Result;
    Result.Item = INDEX_NONE;
    Result.FaceIndex = INDEX_NONE;
    if (IsValid(Actor))
    {
        Result.HitObjectHandle = FActorInstanceHandle(Actor);
    }
    return Result;
}

FHitResult UPinecoTraceLibrary::MakeOverlapResult(const FOverlapResult& Overlap)
{
    FHitResult Result;
    Result.HitObjectHandle = Overlap.OverlapObjectHandle;
    Result.Component = Overlap.Component;
    Result.Item = Overlap.GetItemIndex();
    Result.MyItem = Overlap.MyItemIndex;
    Result.FaceIndex = INDEX_NONE;
    Result.bBlockingHit = Overlap.bBlockingHit;
    Result.PhysicsObject = Overlap.PhysicsObject;
    Result.PhysicsObjectOwner = Overlap.PhysicsObjectOwner;

    // A Body index is local to the queried PA, not a skeleton index or a persistent part ID.
    if (const auto* Mesh = Cast<USkeletalMeshComponent>(Overlap.GetComponent()))
    {
        const auto* PhysicsAsset = Mesh->GetPhysicsAsset();
        if (PhysicsAsset && PhysicsAsset->SkeletalBodySetups.IsValidIndex(Result.Item))
        {
            const auto* BodySetup = PhysicsAsset->SkeletalBodySetups[Result.Item].Get();
            if (BodySetup)
            {
                Result.BoneName = BodySetup->BoneName;
            }
        }
    }
    return Result;
}

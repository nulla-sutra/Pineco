// Copyright 2019-Present tarnishablec. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/HitResult.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PinecoTraceLibrary.generated.h"

class AActor;
struct FOverlapResult;

/** Game-thread adapters. No helper invents missing contact information. */
UCLASS()
class PINECOTRACE_API UPinecoTraceLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    /** Target identity only; no contact point, normal, or path-time information is supplied. */
    UFUNCTION(BlueprintPure, Category="Pineco|Trace")
    static FHitResult MakeActorResult(AActor* Actor);

    /**
     * Native-only because FOverlapResult is not a Blueprint type.
     * Preserves overlap identity and resolves a valid skeletal Body index against the current PA.
     * The result still has no contact point, normal, or path-time information.
     */
    static FHitResult MakeOverlapResult(const FOverlapResult& Overlap);
};

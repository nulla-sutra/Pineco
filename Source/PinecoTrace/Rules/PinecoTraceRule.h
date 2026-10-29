// Copyright 2019-Present tarnishablec. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/HitResult.h"
#include "UObject/Object.h"
#include "PinecoTraceRule.generated.h"

class AActor;
class UWorld;

/** Read-only candidate predicate. Accepted-hit accounting belongs to the execution, not this predicate. */
UCLASS(Abstract, BlueprintType, Blueprintable)
class PINECOTRACE_API UPinecoTraceRule : public UObject
{
    GENERATED_BODY()

public:
    /** False rejects this candidate. Never apply effects or consume a hit allowance here. */
    UFUNCTION(BlueprintNativeEvent, Category="Pineco|Trace")
    bool AcceptHit(UWorld* World, AActor* SourceActor, double EndTimeSeconds,
                   const FHitResult& Candidate) const;

    virtual bool AcceptHit_Implementation(UWorld* World, AActor* SourceActor, double EndTimeSeconds,
                                          const FHitResult& Candidate) const;

    virtual UWorld* GetWorld() const override;
};

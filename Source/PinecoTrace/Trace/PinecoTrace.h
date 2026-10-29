// Copyright 2019-Present tarnishablec. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/HitResult.h"
#include "UObject/Object.h"
#include "PinecoTrace.generated.h"

class AActor;
class UWorld;

/** One result-provider instance per execution. Results need not originate from physics queries. */
UCLASS(Abstract, BlueprintType, Blueprintable)
class PINECOTRACE_API UPinecoTrace : public UObject
{
    GENERATED_BODY()

public:
    /** Discrete skill queries are the default. Opt in only when the provider needs adjacent samples. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Pineco|Trace")
    bool bRequiresContinuousSampling = false;

    /**
     * Synchronously fill the caller's empty, reusable buffer on the game thread.
     * True includes an empty result; false invalidates the entire batch.
     * Discrete queries use EndTimeSeconds as their evaluation time; EvaluateAt supplies [t, t].
     * Only populated identity fields are guaranteed. Contact fields depend on the provider.
     */
    UFUNCTION(BlueprintNativeEvent, Category="Pineco|Trace")
    bool Evaluate(UWorld* World, AActor* SourceActor, double StartTimeSeconds,
                  double EndTimeSeconds, TArray<FHitResult>& OutResults);

    virtual bool Evaluate_Implementation(UWorld* World, AActor* SourceActor, double StartTimeSeconds,
                                         double EndTimeSeconds, TArray<FHitResult>& OutResults);

    virtual UWorld* GetWorld() const override;
};

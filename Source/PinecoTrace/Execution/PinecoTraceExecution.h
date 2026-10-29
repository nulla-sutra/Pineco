// Copyright 2019-Present tarnishablec. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/ActorInstanceHandle.h"
#include "Engine/HitResult.h"
#include "PinecoTrace/Execution/PinecoTraceExecutionTypes.h"
#include "UObject/Object.h"
#include "UObject/ObjectKey.h"
#include "PinecoTraceExecution.generated.h"

class AActor;
class UPinecoTrace;
class UPinecoTraceRule;
class UWorld;

/** Manually driven execution. Keep a strong reference; no automatic Tick, RPC, or effect submission. */
UCLASS(BlueprintType, NotBlueprintable)
class PINECOTRACE_API UPinecoTraceExecution final : public UObject
{
    GENERATED_BODY()

public:
    /** Creates isolated providers and rules without evaluating. Returns null for invalid inputs. */
    UFUNCTION(BlueprintCallable, Category="Pineco|Trace",
        meta=(WorldContext="WorldContextObject", DefaultToSelf="WorldContextObject"))
    static UPinecoTraceExecution* Create(UObject* WorldContextObject, TSubclassOf<UPinecoTrace> TraceClass,
                                         AActor* SourceActor, const FPinecoTraceExecutionSettings& Settings);

    /** Evaluate one skill event at the host's simulation time. Does not schedule another evaluation. */
    UFUNCTION(BlueprintCallable, Category="Pineco|Trace")
    bool EvaluateAt(double TimeSeconds, TArray<FHitResult>& OutResults);

    /**
     * Game-thread synchronous step. Caller owns and may reuse OutResults.
     * Discrete providers allow gaps and equal evaluation times, but never backwards time.
     * Continuous providers require adjacent intervals and strictly advancing end times after the first step.
     * False clears output, except a reentrant/off-thread call is rejected without touching it.
     * Closed executions cannot restart. Failure and terminal state are available through getters.
     */
    UFUNCTION(BlueprintCallable, Category="Pineco|Trace")
    bool Advance(double StartTimeSeconds, double EndTimeSeconds, TArray<FHitResult>& OutResults);

    UFUNCTION(BlueprintCallable, Category="Pineco|Trace")
    void Complete();

    UFUNCTION(BlueprintCallable, Category="Pineco|Trace")
    void Cancel();

    UFUNCTION(BlueprintPure, Category="Pineco|Trace")
    EPinecoTraceExecutionState GetState() const { return State; }

    UFUNCTION(BlueprintPure, Category="Pineco|Trace")
    EPinecoTraceError GetError() const { return Error; }

    UFUNCTION(BlueprintPure, Category="Pineco|Trace")
    bool HasAdvanced() const { return bHasAdvanced; }

    /** Meaningful only after HasAdvanced; uses the host's simulation clock. */
    UFUNCTION(BlueprintPure, Category="Pineco|Trace")
    double GetLastEndTimeSeconds() const { return LastEndTimeSeconds; }

    virtual UWorld* GetWorld() const override;

private:
    struct FHitKey
    {
        FObjectKey Actor;
        FActorInstanceHandle Instance;
        FObjectKey Component;
        FName BoneName;
        int32 Item = INDEX_NONE;

        bool operator==(const FHitKey& Other) const
        {
            return Actor == Other.Actor && Instance == Other.Instance && Component == Other.Component
                && BoneName == Other.BoneName && Item == Other.Item;
        }

        friend uint32 GetTypeHash(const FHitKey& Key)
        {
            // Ordinary actors use native object keys, avoiding actor-path hashing in the hot path.
            auto Hash = Key.Actor != FObjectKey() ? GetTypeHash(Key.Actor) : GetTypeHash(Key.Instance);
            Hash = HashCombineFast(Hash, GetTypeHash(Key.Component));
            Hash = HashCombineFast(Hash, GetTypeHash(Key.BoneName));
            return HashCombineFast(Hash, GetTypeHash(Key.Item));
        }
    };

    bool Fail(EPinecoTraceError InError);
    void ReleaseResources();
    FHitKey MakeHitKey(const FHitResult& Hit) const;

    UPROPERTY(Transient)
    TObjectPtr<UPinecoTrace> Trace;

    UPROPERTY(Transient)
    TArray<TObjectPtr<UPinecoTraceRule>> Rules;

    TWeakObjectPtr<UWorld> ExecutionWorld;
    TWeakObjectPtr<AActor> SourceActor;
    TSet<FHitKey> SeenHits;
    EPinecoTraceHitPolicy HitPolicy = EPinecoTraceHitPolicy::OncePerActor;
    EPinecoTraceExecutionState State = EPinecoTraceExecutionState::Inactive;
    EPinecoTraceError Error = EPinecoTraceError::None;
    double LastEndTimeSeconds = 0.0;
    bool bIgnoreSourceActor = true;
    bool bRequiresSourceActor = false;
    bool bRequiresContinuousSampling = false;
    bool bHasAdvanced = false;
    bool bAdvancing = false;
};

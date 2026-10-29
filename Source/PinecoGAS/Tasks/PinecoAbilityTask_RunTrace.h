// Copyright 2019-Present tarnishablec. All Rights Reserved.

#pragma once

#include "Abilities/Tasks/AbilityTask.h"
#include "Engine/HitResult.h"
#include "PinecoTrace/Execution/PinecoTraceExecutionTypes.h"
#include "PinecoAbilityTask_RunTrace.generated.h"

class AActor;
class UPinecoTrace;
class UPinecoTraceExecution;

UENUM(BlueprintType)
enum class EPinecoTraceTaskResult : uint8
{
    Running,
    Completed,
    Cancelled,
    Failed,
    NotAuthority,
    AbilityEnded
};

// Identical delegate signatures keep the stock AbilityTask Blueprint node's data pins consistent.
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPinecoTraceTaskEvent,
                                               const TArray<FHitResult>&, Results, EPinecoTraceTaskResult, Result,
                                               EPinecoTraceError, Error);

/** Server-authoritative GAS adapter. Forwards execution steps without selecting a trace strategy. */
UCLASS(meta=(ExposedAsyncProxy="Task"))
class PINECOGAS_API UPinecoAbilityTask_RunTrace : public UAbilityTask
{
    GENERATED_BODY()

public:
    /** Factory only configures the task. C++ callers bind delegates, then call ReadyForActivation. */
    UFUNCTION(BlueprintCallable, Category="Pineco|Tasks",
        meta=(HidePin="OwningAbility", DefaultToSelf="OwningAbility", BlueprintInternalUseOnly="true",
            AdvancedDisplay="Settings,TaskInstanceName", DisplayName="Run Pineco Trace"))
    static UPinecoAbilityTask_RunTrace* RunTrace(
        UGameplayAbility* OwningAbility, TSubclassOf<UPinecoTrace> TraceClass,
        const FPinecoTraceExecutionSettings& Settings, FName TaskInstanceName = NAME_None);

    /** Common execution step after OnReady. The provider decides when and how to gather candidates. */
    UFUNCTION(BlueprintCallable, Category="Pineco|Trace")
    bool Advance(double StartTimeSeconds, double EndTimeSeconds);

    UFUNCTION(BlueprintCallable, Category="Pineco|Trace")
    void Complete();

    UFUNCTION(BlueprintCallable, Category="Pineco|Trace")
    void Cancel();

    UFUNCTION(BlueprintPure, Category="Pineco|Trace")
    EPinecoTraceTaskResult GetResult() const { return Result; }

    UFUNCTION(BlueprintPure, Category="Pineco|Trace")
    EPinecoTraceError GetError() const { return Error; }

    /** No query has run yet. Results is empty; Result is Running. */
    UPROPERTY(BlueprintAssignable, Category="Pineco|Trace")
    FPinecoTraceTaskEvent OnReady;

    /** Results are read-only during this notification; copy them if retaining beyond the callback. */
    UPROPERTY(BlueprintAssignable, Category="Pineco|Trace")
    FPinecoTraceTaskEvent OnResults;

    /** Once after task teardown, with empty Results. Suppressed when the owning Ability has ended. */
    UPROPERTY(BlueprintAssignable, Category="Pineco|Trace")
    FPinecoTraceTaskEvent OnFinished;

    virtual void ExternalConfirm(bool bEndTask) override;
    virtual void ExternalCancel() override;
    virtual void OnDestroy(bool bInOwnerFinished) override;

protected:
    virtual void Activate() override;

private:
    bool HasAuthority() const;
    void Finish(EPinecoTraceTaskResult InResult, EPinecoTraceError InError);
    void DispatchFinished();

    UPROPERTY(Transient)
    TSubclassOf<UPinecoTrace> TraceClass;

    UPROPERTY(Transient)
    FPinecoTraceExecutionSettings Settings;

    UPROPERTY(Transient)
    TObjectPtr<UPinecoTraceExecution> Execution;

    TArray<FHitResult> Results;
    TWeakObjectPtr<AActor> SourceActor;
    TWeakObjectPtr<UGameplayAbility> CompletionAbility;
    EPinecoTraceTaskResult Result = EPinecoTraceTaskResult::Running;
    EPinecoTraceError Error = EPinecoTraceError::None;
    bool bReady = false;
    bool bFinished = false;
    bool bAdvancing = false;
    bool bFinishNotificationPending = false;
};

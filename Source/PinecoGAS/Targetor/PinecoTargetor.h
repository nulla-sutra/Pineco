// Copyright 2019-Present tarnishablec. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbilityTargetActor.h"
#include "PinecoTargetor.generated.h"

UCLASS(Abstract)
class PINECOGAS_API APinecoTargetor : public AGameplayAbilityTargetActor
{
    GENERATED_BODY()

public:
    // Sets default values for this actor's properties
    APinecoTargetor();

    virtual void StartTargeting(UGameplayAbility* Ability) override;
    virtual void ConfirmTargetingAndContinue() override;

    UPROPERTY(BlueprintReadOnly, Category="Pineco")
    TArray<FHitResult> CurrentHitResults;

    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Pineco")
    void SetShouldDestroyOnConfirmation(bool bShould);

    UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category="Pineco")
    void SubmitHitResults();
};

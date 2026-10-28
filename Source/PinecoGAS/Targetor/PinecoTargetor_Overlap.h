// Copyright 2019-Present tarnishablec. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "PinecoGAS/Targetor/PinecoTargetor.h"
#include "PinecoTargetor_Overlap.generated.h"

UCLASS(Abstract)
class PINECOGAS_API APinecoTargetor_Overlap : public APinecoTargetor
{
    GENERATED_BODY()

public:
    // Sets default values for this actor's properties
    APinecoTargetor_Overlap();

    virtual void ConfirmTargetingAndContinue() override;
    virtual void StartTargeting(UGameplayAbility* Ability) override;

    // Called every frame
    virtual void Tick(float DeltaTime) override;

    UFUNCTION(BlueprintNativeEvent, Category="Pineco")
    void OverlappingComponents(TArray<UPrimitiveComponent*>& Components);
};

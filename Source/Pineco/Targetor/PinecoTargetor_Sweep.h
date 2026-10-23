// Copyright 2019-Present tarnishablec. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Pineco/Targetor/PinecoTargetor.h"
#include "PinecoTargetor_Sweep.generated.h"

UCLASS(Abstract)
class PINECO_API APinecoTargetor_Sweep : public APinecoTargetor
{
    GENERATED_BODY()

public:
    // Sets default values for this actor's properties
    APinecoTargetor_Sweep();

    UFUNCTION(BlueprintNativeEvent)
    void PerformSweep(TArray<FHitResult>& HitResults);

    virtual void ConfirmTargetingAndContinue() override;
};

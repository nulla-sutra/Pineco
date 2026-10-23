// Copyright 2019-Present tarnishablec. All Rights Reserved.


#include "Pineco/Targetor/PinecoTargetor_Sweep.h"


// Sets default values
APinecoTargetor_Sweep::APinecoTargetor_Sweep()
{
    // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = false;
}

void APinecoTargetor_Sweep::ConfirmTargetingAndContinue()
{
    check(ShouldProduceTargetData());
    if (SourceActor && IsConfirmTargetingAllowed())
    {
        PerformSweep(CurrentHitResults);
        SubmitHitResults();
    }
}


void APinecoTargetor_Sweep::PerformSweep_Implementation(TArray<FHitResult>& HitResults)
{
}

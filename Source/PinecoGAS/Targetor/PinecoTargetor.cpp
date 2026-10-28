// Copyright 2019-Present tarnishablec. All Rights Reserved.


#include "PinecoGAS/Targetor/PinecoTargetor.h"

#include "Abilities/GameplayAbility.h"


// Sets default values
APinecoTargetor::APinecoTargetor()
{
    // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
    PrimaryActorTick.bCanEverTick = false;
    this->ShouldProduceTargetDataOnServer = true;
    bReplicates = false;
}

void APinecoTargetor::StartTargeting(UGameplayAbility* Ability)
{
    Super::StartTargeting(Ability);
    SourceActor = Ability->GetCurrentActorInfo()->AvatarActor.Get();
}

void APinecoTargetor::ConfirmTargetingAndContinue()
{
    unimplemented();
}

void APinecoTargetor::SetShouldDestroyOnConfirmation(const bool bShould)
{
    bDestroyOnConfirmation = bShould;
}

void APinecoTargetor::SubmitHitResults_Implementation()
{
    if (CurrentHitResults.Num() > 0)
    {
        auto&& Handle = StartLocation.MakeTargetDataHandleFromHitResults(OwningAbility, CurrentHitResults);
        TargetDataReadyDelegate.Broadcast(Handle);
    }
}

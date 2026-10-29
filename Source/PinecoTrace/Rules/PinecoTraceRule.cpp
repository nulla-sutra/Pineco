// Copyright 2019-Present tarnishablec. All Rights Reserved.

#include "PinecoTrace/Rules/PinecoTraceRule.h"

bool UPinecoTraceRule::AcceptHit_Implementation(UWorld*, AActor*, double, const FHitResult&) const
{
    return false;
}

UWorld* UPinecoTraceRule::GetWorld() const
{
    const auto* Outer = GetOuter();
    return !HasAnyFlags(RF_ClassDefaultObject) && Outer ? Outer->GetWorld() : nullptr;
}

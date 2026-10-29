// Copyright 2019-Present tarnishablec. All Rights Reserved.

#include "PinecoTrace/Trace/PinecoTrace.h"

bool UPinecoTrace::Evaluate_Implementation(UWorld*, AActor*, double, double, TArray<FHitResult>&)
{
    // An unimplemented provider is an execution error, not a successful miss.
    return false;
}

UWorld* UPinecoTrace::GetWorld() const
{
    const auto* Outer = GetOuter();
    return !HasAnyFlags(RF_ClassDefaultObject) && Outer ? Outer->GetWorld() : nullptr;
}

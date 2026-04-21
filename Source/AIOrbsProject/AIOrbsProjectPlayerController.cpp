// Copyright Epic Games, Inc. All Rights Reserved.

#include "AIOrbsProjectPlayerController.h"

#include "AIOrbsProjectCameraManager.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"

// -----------------------------------------------------------------------------
// Constructor
// -----------------------------------------------------------------------------

AAIOrbsProjectPlayerController::AAIOrbsProjectPlayerController()
{
    // Override the default camera manager with our custom version, which
    // enforces first-person pitch limits (ViewPitchMin / ViewPitchMax).
    PlayerCameraManagerClass = AAIOrbsProjectCameraManager::StaticClass();
}

// -----------------------------------------------------------------------------
// Input Setup
// -----------------------------------------------------------------------------

void AAIOrbsProjectPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    // Register all mapping contexts with the Enhanced Input subsystem.
    // GetSubsystem<> returns null if the local player is not yet available
    // (e.g. during listen server setup), so the null check is mandatory.
    UEnhancedInputLocalPlayerSubsystem* Subsystem =
        ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());

    if (!Subsystem)
    {
        // This is expected in server-only configurations — not an error.
        return;
    }

    for (UInputMappingContext* CurrentContext : DefaultMappingContexts)
    {
        if (!IsValid(CurrentContext))
        {
            continue; // Skip null entries — Blueprint may leave slots empty.
        }

        // Priority 0 — base gameplay context. UI or ability contexts can
        // be added at higher priorities at runtime to override specific bindings.
        Subsystem->AddMappingContext(CurrentContext, 0);
    }
}

// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Camera/PlayerCameraManager.h"
#include "AIOrbsProjectCameraManager.generated.h"

/**
 * AAIOrbsProjectCameraManager — Custom camera manager for the AI Orbs first-person project.
 *
 * Configures pitch limits to prevent the camera from flipping upside down
 * when the player looks fully up or down. These limits are applied by the
 * engine's camera manager update loop via ViewPitchMin / ViewPitchMax.
 *
 * Architecture notes:
 * - Set as the PlayerCameraManagerClass on AAIOrbsProjectPlayerController.
 * - The pitch range (-70° to +80°) is asymmetric: more downward look than upward,
 *   which matches natural human head movement and first-person game conventions.
 */
UCLASS()
class AAIOrbsProjectCameraManager : public APlayerCameraManager
{
    GENERATED_BODY()

public:

    /** Applies the pitch clamp values to ViewPitchMin and ViewPitchMax. */
    AAIOrbsProjectCameraManager();
};

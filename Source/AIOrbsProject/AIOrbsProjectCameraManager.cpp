// Copyright Epic Games, Inc. All Rights Reserved.

#include "AIOrbsProjectCameraManager.h"

AAIOrbsProjectCameraManager::AAIOrbsProjectCameraManager()
{
    // Pitch range for first-person look.
    // -70° allows a steep downward look (crouching inspection, descending stairs).
    // +80° allows a near-vertical upward look without reaching 90° where gimbal lock
    // effects can cause visual artifacts in some camera setups.
    ViewPitchMin = -70.0f;
    ViewPitchMax =  80.0f;
}

// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "AIOrbsProjectPlayerController.generated.h"

// Forward declaration
class UInputMappingContext;

/**
 * AAIOrbsProjectPlayerController — Base player controller for the AI Orbs project.
 *
 * Responsibilities:
 * - Overrides the PlayerCameraManager class to AAIOrbsProjectCameraManager
 *   (which enforces pitch limits for first-person look).
 * - Registers one or more InputMappingContexts with the Enhanced Input subsystem
 *   during SetupInputComponent(), applying them at priority 0.
 *
 * Architecture notes:
 * - DefaultMappingContexts is an array so multiple contexts can be stacked
 *   (e.g. a base gameplay context + a UI context) without modifying this class.
 * - Marked abstract — subclass in Blueprint to assign the mapping context assets.
 */
UCLASS(abstract)
class AIORBSPROJECT_API AAIOrbsProjectPlayerController : public APlayerController
{
    GENERATED_BODY()

public:

    /** Sets the PlayerCameraManagerClass to AAIOrbsProjectCameraManager. */
    AAIOrbsProjectPlayerController();

protected:

    /**
     * Input Mapping Contexts to register with the Enhanced Input subsystem.
     * Assigned in the Blueprint subclass. Applied in order (index 0 = lowest priority).
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input", meta = (AllowPrivateAccess = "true"))
    TArray<TObjectPtr<UInputMappingContext>> DefaultMappingContexts;

    /**
     * Registers all DefaultMappingContexts with the local player's Enhanced Input subsystem.
     * Called automatically by the engine after the input component is created.
     */
    virtual void SetupInputComponent() override;
};

// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "Logging/LogMacros.h"
#include "AIOrbsProjectCharacter.generated.h"

// Forward declarations
class UInputComponent;
class USkeletalMeshComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;

/** Custom log category for the player character system. */
DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 * AAIOrbsProjectCharacter — Base class for the first-person player character.
 *
 * Handles:
 * - First-person mesh and camera setup (separate from the full-body third-person mesh).
 * - Enhanced Input bindings for movement, looking, and jumping.
 * - Virtual DoMove / DoAim / DoJump methods for Blueprint overrides or UI-driven input.
 *
 * Architecture notes:
 * - Marked abstract — must be subclassed in Blueprint to set input action references
 *   and assign the first-person skeletal mesh.
 * - The first-person mesh is owner-only (SetOnlyOwnerSee) and uses FirstPerson primitive type.
 *   The full-body mesh (GetMesh()) is hidden from the owner but visible to others.
 * - PlayerTag is a GameplayTag available for faction / role identification.
 */
UCLASS(abstract)
class AAIOrbsProjectCharacter : public ACharacter
{
    GENERATED_BODY()

    // -------------------------------------------------------------------------
    // Components (private with AllowPrivateAccess for Blueprint read)
    // -------------------------------------------------------------------------

    /** First-person arms mesh — visible only to the owning player. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<USkeletalMeshComponent> FirstPersonMesh;

    /** First-person camera, attached to the head socket of FirstPersonMesh. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components", meta = (AllowPrivateAccess = "true"))
    TObjectPtr<UCameraComponent> FirstPersonCameraComponent;

protected:

    // -------------------------------------------------------------------------
    // Input Actions — set in Blueprint subclass
    // -------------------------------------------------------------------------

    /** Input action for jumping. Bound to DoJumpStart (Started) and DoJumpEnd (Completed). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> JumpAction;

    /** Input action for WASD / gamepad stick movement. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> MoveAction;

    /** Input action for gamepad look (right stick). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> LookAction;

    /** Input action for mouse look. Separate from LookAction for sensitivity tuning. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
    TObjectPtr<UInputAction> MouseLookAction;

    // -------------------------------------------------------------------------
    // Settings
    // -------------------------------------------------------------------------

    /**
     * GameplayTag identifying this character's role or faction.
     * Expected value: OrbType.Player (defined in DefaultGameplayTags.ini).
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    FGameplayTag PlayerTag;

public:

    /** Sets up components, movement parameters, and first-person camera. */
    AAIOrbsProjectCharacter();

protected:

    // -------------------------------------------------------------------------
    // Input Handlers (raw — called from EnhancedInput bindings)
    // -------------------------------------------------------------------------

    /**
     * Called by the Move input action. Extracts the 2D axis and forwards to DoMove().
     *
     * @param Value The input action value containing a FVector2D (X=Right, Y=Forward).
     */
    void MoveInput(const FInputActionValue& Value);

    /**
     * Called by the Look / MouseLook input actions. Extracts the 2D axis and forwards to DoAim().
     *
     * @param Value The input action value containing a FVector2D (X=Yaw, Y=Pitch).
     */
    void LookInput(const FInputActionValue& Value);

    // -------------------------------------------------------------------------
    // Virtual Input Methods (overridable in Blueprint or subclasses)
    // -------------------------------------------------------------------------

    /**
     * Applies yaw and pitch rotation to the controller.
     * Virtual so Blueprint subclasses or UI systems can redirect aim input.
     *
     * @param Yaw   Horizontal rotation delta (degrees/frame).
     * @param Pitch Vertical rotation delta (degrees/frame).
     */
    UFUNCTION(BlueprintCallable, Category = "Input")
    virtual void DoAim(float Yaw, float Pitch);

    /**
     * Applies movement input along the character's right and forward vectors.
     * Virtual so Blueprint subclasses can add sprinting, strafing modifiers, etc.
     *
     * @param Right   Lateral movement axis (-1 left, +1 right).
     * @param Forward Longitudinal movement axis (-1 back, +1 forward).
     */
    UFUNCTION(BlueprintCallable, Category = "Input")
    virtual void DoMove(float Right, float Forward);

    /**
     * Triggers ACharacter::Jump(). Virtual for Blueprint override.
     */
    UFUNCTION(BlueprintCallable, Category = "Input")
    virtual void DoJumpStart();

    /**
     * Triggers ACharacter::StopJumping(). Virtual for Blueprint override.
     */
    UFUNCTION(BlueprintCallable, Category = "Input")
    virtual void DoJumpEnd();

    /** Binds Enhanced Input actions to their handler functions. */
    virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;

public:

    // -------------------------------------------------------------------------
    // Accessors
    // -------------------------------------------------------------------------

    /** Returns the first-person skeletal mesh component. */
    USkeletalMeshComponent* GetFirstPersonMesh() const { return FirstPersonMesh; }

    /** Returns the first-person camera component. */
    UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }
};

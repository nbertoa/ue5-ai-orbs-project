// Copyright Epic Games, Inc. All Rights Reserved.

#include "AIOrbsProjectCharacter.h"

#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EnhancedInputComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputActionValue.h"

DEFINE_LOG_CATEGORY(LogTemplateCharacter);

// -----------------------------------------------------------------------------
// Constructor
// -----------------------------------------------------------------------------

AAIOrbsProjectCharacter::AAIOrbsProjectCharacter()
{
    // Outer capsule size — slightly wider than the default to accommodate
    // the first-person arms mesh which protrudes slightly at shoulder width.
    GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

    // First-person arms mesh — attached to the head socket of the full-body mesh.
    // Visible only to this player (SetOnlyOwnerSee); uses FirstPerson primitive type
    // so it renders in front of world geometry without z-fighting.
    FirstPersonMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("First Person Mesh"));
    FirstPersonMesh->SetupAttachment(GetMesh());
    FirstPersonMesh->SetOnlyOwnerSee(true);
    FirstPersonMesh->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
    FirstPersonMesh->SetCollisionProfileName(FName("NoCollision"));
    FirstPersonMesh->SetHiddenInGame(true); // Unhidden by Blueprint after animation setup

    // Camera attached to the head socket of the first-person mesh.
    // bUsePawnControlRotation ensures look input rotates the camera directly.
    FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("First Person Camera"));
    FirstPersonCameraComponent->SetupAttachment(FirstPersonMesh, FName("head"));
    FirstPersonCameraComponent->SetRelativeLocationAndRotation(
        FVector(-2.8f, 5.89f, 0.0f),
        FRotator(0.0f, 90.0f, -90.0f)
    );
    FirstPersonCameraComponent->bUsePawnControlRotation = true;

    // First-person FOV and scale — narrower FOV reduces perceived speed at normal FPS,
    // and scale 0.6 keeps the arms proportional to the world-space character size.
    FirstPersonCameraComponent->bEnableFirstPersonFieldOfView = true;
    FirstPersonCameraComponent->bEnableFirstPersonScale = true;
    FirstPersonCameraComponent->FirstPersonFieldOfView = 70.0f;
    FirstPersonCameraComponent->FirstPersonScale = 0.6f;

    // Full-body mesh — hidden from the owner but visible to other players (e.g. multiplayer).
    // Uses WorldSpaceRepresentation so it casts correct shadows for the owner.
    GetMesh()->SetOwnerNoSee(true);
    GetMesh()->FirstPersonPrimitiveType = EFirstPersonPrimitiveType::WorldSpaceRepresentation;
    GetMesh()->SetHiddenInGame(true);

    // Narrower capsule for the full body representation.
    GetCapsuleComponent()->SetCapsuleSize(34.0f, 96.0f);

    // Movement tuning — higher braking deceleration stops the character quickly
    // on landing, preventing sliding after a jump.
    GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;
    GetCharacterMovement()->AirControl = 0.5f;
}

// -----------------------------------------------------------------------------
// Input Setup
// -----------------------------------------------------------------------------

void AAIOrbsProjectCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    // Require Enhanced Input — the legacy system is not supported by this template.
    UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent);
    if (!EnhancedInputComponent)
    {
        UE_LOG(LogTemplateCharacter, Error,
            TEXT("%s: Failed to find an Enhanced Input Component. "
                 "This character requires the Enhanced Input system."),
            *GetNameSafe(this));
        return;
    }

    // Jump — Started fires immediately on press; Completed fires on release.
    EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Started,   this, &AAIOrbsProjectCharacter::DoJumpStart);
    EnhancedInputComponent->BindAction(JumpAction, ETriggerEvent::Completed,  this, &AAIOrbsProjectCharacter::DoJumpEnd);

    // Move — Triggered fires every frame while the input is held.
    EnhancedInputComponent->BindAction(MoveAction, ETriggerEvent::Triggered, this, &AAIOrbsProjectCharacter::MoveInput);

    // Look — both gamepad and mouse look route through the same LookInput handler.
    EnhancedInputComponent->BindAction(LookAction,      ETriggerEvent::Triggered, this, &AAIOrbsProjectCharacter::LookInput);
    EnhancedInputComponent->BindAction(MouseLookAction, ETriggerEvent::Triggered, this, &AAIOrbsProjectCharacter::LookInput);
}

// -----------------------------------------------------------------------------
// Input Handlers
// -----------------------------------------------------------------------------

void AAIOrbsProjectCharacter::MoveInput(const FInputActionValue& Value)
{
    // The Move action is a 2D axis: X = strafe right, Y = move forward.
    const FVector2D MovementVector = Value.Get<FVector2D>();
    DoMove(MovementVector.X, MovementVector.Y);
}

void AAIOrbsProjectCharacter::LookInput(const FInputActionValue& Value)
{
    // The Look action is a 2D axis: X = yaw (horizontal), Y = pitch (vertical).
    const FVector2D LookAxisVector = Value.Get<FVector2D>();
    DoAim(LookAxisVector.X, LookAxisVector.Y);
}

// -----------------------------------------------------------------------------
// Virtual Input Methods
// -----------------------------------------------------------------------------

void AAIOrbsProjectCharacter::DoAim(float Yaw, float Pitch)
{
    if (!GetController())
    {
        return;
    }

    AddControllerYawInput(Yaw);
    AddControllerPitchInput(Pitch);
}

void AAIOrbsProjectCharacter::DoMove(float Right, float Forward)
{
    if (!GetController())
    {
        return;
    }

    // Move relative to the actor's own axes so look direction doesn't affect strafe direction.
    AddMovementInput(GetActorRightVector(),   Right);
    AddMovementInput(GetActorForwardVector(), Forward);
}

void AAIOrbsProjectCharacter::DoJumpStart()
{
    Jump();
}

void AAIOrbsProjectCharacter::DoJumpEnd()
{
    StopJumping();
}

// Copyright Epic Games, Inc. All Rights Reserved.

#include "Orb/Orb.h"

#include "AIOrbsProjectCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "Orb/OrbAIController.h"
#include "Sound/SoundBase.h"

DEFINE_LOG_CATEGORY(LogOrb);

// -----------------------------------------------------------------------------
// Constructor
// -----------------------------------------------------------------------------

AOrb::AOrb()
{
    PrimaryActorTick.bCanEverTick = true;

    // Set up the inherited capsule for movement collision.
    // The orb mesh itself has no collision — all interaction goes through
    // CollisionSphere for proximity detection.
    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);
    GetCapsuleComponent()->SetCollisionProfileName(TEXT("Pawn"));

    // Disable collision on the inherited skeletal mesh — orbs are purely visual here.
    GetMesh()->SetCollisionProfileName(TEXT("NoCollision"));
    GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);

    // Sphere mesh that represents the orb visually.
    // Material is set at runtime by ActivateOrb() / DeactivateOrb().
    OrbMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("OrbMesh"));
    OrbMesh->SetupAttachment(GetCapsuleComponent());
    OrbMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    // Note: DefaultMaterial is null at construction time (set from the Blueprint CDO),
    // so we do NOT call SetMaterial here. ActivateOrb/DeactivateOrb handle this at runtime.

    // Sphere trigger for proximity overlap detection (e.g. enemy orb touching player).
    CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
    CollisionSphere->SetupAttachment(GetCapsuleComponent());
    CollisionSphere->InitSphereRadius(100.f);
    CollisionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

    // Use AOrbAIController — starts the BehaviorTree in OnPossess().
    AIControllerClass = AOrbAIController::StaticClass();
    AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

    // Configure CharacterMovementComponent for a floating orb.
    // Flying mode removes gravity and allows full 3D movement via the BehaviorTree.
    UCharacterMovementComponent* MoveComp = GetCharacterMovement();
    if (ensure(MoveComp))
    {
        MoveComp->DefaultLandMovementMode = MOVE_Flying;

        // Deliberately slow — orbs should feel weightless but not frantic.
        MoveComp->MaxWalkSpeed = 250.f;

        // Low acceleration / braking produces a smooth, floaty drift effect.
        MoveComp->MaxAcceleration = 200.f;
        MoveComp->BrakingDecelerationFlying = 150.f;
        MoveComp->BrakingDecelerationWalking = 150.f;

        // Controller-desired rotation with a slow rotation rate gives the orb
        // a gentle turning behavior rather than snapping to face its target instantly.
        MoveComp->bUseControllerDesiredRotation = true;
        MoveComp->RotationRate = FRotator(0.f, 60.f, 0.f);
    }
}

// -----------------------------------------------------------------------------
// BeginPlay
// -----------------------------------------------------------------------------

void AOrb::BeginPlay()
{
    Super::BeginPlay();

    // Bind overlap only after the world is fully initialized.
    // Binding in the constructor would fire before Begin Play on other actors.
    CollisionSphere->OnComponentBeginOverlap.AddDynamic(this, &AOrb::OnOverlapBegin);

    UE_LOG(LogOrb, Log, TEXT("%s: BeginPlay — OrbTypeTag = [%s]"),
        *GetName(), *OrbTypeTag.ToString());
}

// -----------------------------------------------------------------------------
// State — Activate / Deactivate
// -----------------------------------------------------------------------------

void AOrb::ActivateOrb()
{
    bIsActiveOrb = true;

    if (IsValid(ActiveMaterial))
    {
        OrbMesh->SetMaterial(0, ActiveMaterial);
    }
    else
    {
        UE_LOG(LogOrb, Warning, TEXT("%s: ActivateOrb — ActiveMaterial is not set!"), *GetName());
    }

    PlayActivatedSound();

    UE_LOG(LogOrb, Log, TEXT("%s: Activated."), *GetName());
}

void AOrb::DeactivateOrb()
{
    bIsActiveOrb = false;

    if (IsValid(DefaultMaterial))
    {
        OrbMesh->SetMaterial(0, DefaultMaterial);
    }
    else
    {
        UE_LOG(LogOrb, Warning, TEXT("%s: DeactivateOrb — DefaultMaterial is not set!"), *GetName());
    }

    UE_LOG(LogOrb, Log, TEXT("%s: Deactivated."), *GetName());
}

// -----------------------------------------------------------------------------
// Audio
// -----------------------------------------------------------------------------

void AOrb::PlayActivatedSound()
{
    if (!IsValid(ActivatedSound))
    {
        UE_LOG(LogOrb, Warning, TEXT("%s: PlayActivatedSound — ActivatedSound is not set!"), *GetName());
        return;
    }

    UGameplayStatics::PlaySoundAtLocation(this, ActivatedSound, GetActorLocation());
}

// -----------------------------------------------------------------------------
// Consume
// -----------------------------------------------------------------------------

void AOrb::ConsumeOrb()
{
    // Spawn VFX at the orb's current location before destroying it.
    if (IsValid(DestroyVFX))
    {
        UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), DestroyVFX, GetActorLocation());
    }

    // Play destruction sound at the orb's current location before destroying it.
    if (IsValid(DestroySFX))
    {
        UGameplayStatics::PlaySoundAtLocation(this, DestroySFX, GetActorLocation());
    }

    UE_LOG(LogOrb, Log, TEXT("%s: Consumed."), *GetName());

    Destroy();
}

// -----------------------------------------------------------------------------
// Overlap
// -----------------------------------------------------------------------------

void AOrb::OnOverlapBegin(
    UPrimitiveComponent* OverlappedComp,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult
)
{
    // Guard: OtherActor can be null in edge cases (e.g. during level teardown).
    if (!IsValid(OtherActor))
    {
        return;
    }

    // Only enemy orbs damage the player on contact.
    // Ally orbs are collected by AOrbReservoir, not by direct player touch.
    const bool bIsEnemyOrb = OrbTypeTag == FGameplayTag::RequestGameplayTag(FName("OrbType.Enemy"));
    if (!bIsEnemyOrb)
    {
        return;
    }

    // Check if the other actor is the player character.
    if (OtherActor->IsA<AAIOrbsProjectCharacter>())
    {
        UE_LOG(LogOrb, Log, TEXT("%s: Enemy orb touched player — consuming."), *GetName());
        ConsumeOrb();
    }
}

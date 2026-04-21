// Copyright Epic Games, Inc. All Rights Reserved.

#include "Orb/OrbReservoir.h"

#include "Blueprint/UserWidget.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameplayTagContainer.h"
#include "Kismet/GameplayStatics.h"
#include "Orb/Orb.h"

DEFINE_LOG_CATEGORY(LogOrbReservoir);

// -----------------------------------------------------------------------------
// Constructor
// -----------------------------------------------------------------------------

AOrbReservoir::AOrbReservoir()
{
    // Tick is not needed — all logic is event-driven via overlap callbacks.
    PrimaryActorTick.bCanEverTick = false;

    // CollectTrigger is the root so the reservoir's world position equals
    // the trigger center. The visual mesh is a child.
    CollectTrigger = CreateDefaultSubobject<USphereComponent>(TEXT("CollectTrigger"));
    SetRootComponent(CollectTrigger);
    CollectTrigger->InitSphereRadius(300.f);
    CollectTrigger->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
    CollectTrigger->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
    CollectTrigger->SetGenerateOverlapEvents(true);

    ReservoirMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ReservoirMesh"));
    ReservoirMesh->SetupAttachment(GetRootComponent());
    ReservoirMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

// -----------------------------------------------------------------------------
// BeginPlay
// -----------------------------------------------------------------------------

void AOrbReservoir::BeginPlay()
{
    Super::BeginPlay();

    // Bind the overlap delegate after the world is ready.
    CollectTrigger->OnComponentBeginOverlap.AddDynamic(this, &AOrbReservoir::OnCollectTriggerBegin);

    // Create the dynamic material instance (MID) from slot 0 of the reservoir mesh.
    // We need a MID — not the static material — to drive scalar parameters at runtime.
    if (IsValid(ReservoirMesh->GetMaterial(0)))
    {
        ReservoirMID = ReservoirMesh->CreateAndSetMaterialInstanceDynamic(0);
        if (!IsValid(ReservoirMID))
        {
            UE_LOG(LogOrbReservoir, Warning,
                TEXT("%s: BeginPlay — Failed to create MID from ReservoirMesh slot 0."), *GetName());
        }
    }
    else
    {
        UE_LOG(LogOrbReservoir, Warning,
            TEXT("%s: BeginPlay — ReservoirMesh has no material in slot 0. Emissive will not update."),
            *GetName());
    }

    // Apply baseline emissive immediately so the reservoir doesn't start black.
    UpdateReservoirEmissive();

    UE_LOG(LogOrbReservoir, Log,
        TEXT("%s: BeginPlay — MaxStorage = %d, BaseEmissive = %.2f."),
        *GetName(), MaxStorage, BaseEmissive);
}

// -----------------------------------------------------------------------------
// UI
// -----------------------------------------------------------------------------

void AOrbReservoir::CreateHurtScreenWidget()
{
    if (!IsValid(HurtScreenWidgetClass))
    {
        UE_LOG(LogOrbReservoir, Warning,
            TEXT("%s: CreateHurtScreenWidget — HurtScreenWidgetClass is not set!"), *GetName());
        return;
    }

    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (!IsValid(PC))
    {
        UE_LOG(LogOrbReservoir, Warning,
            TEXT("%s: CreateHurtScreenWidget — Could not get PlayerController 0."), *GetName());
        return;
    }

    UUserWidget* Widget = CreateWidget<UUserWidget>(PC, HurtScreenWidgetClass);
    if (IsValid(Widget))
    {
        Widget->AddToViewport();
    }
    else
    {
        UE_LOG(LogOrbReservoir, Warning,
            TEXT("%s: CreateHurtScreenWidget — Widget creation failed."), *GetName());
    }
}

// -----------------------------------------------------------------------------
// Overlap
// -----------------------------------------------------------------------------

void AOrbReservoir::OnCollectTriggerBegin(
    UPrimitiveComponent* OverlappedComp,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult
)
{
    // Guard: only react to AOrb actors — ignore everything else silently.
    AOrb* Orb = Cast<AOrb>(OtherActor);
    if (!IsValid(Orb))
    {
        return;
    }

    const FGameplayTag AllyTag  = FGameplayTag::RequestGameplayTag(FName("OrbType.Ally"));
    const FGameplayTag EnemyTag = FGameplayTag::RequestGameplayTag(FName("OrbType.Enemy"));
    const FGameplayTag OrbTag   = Orb->GetOrbTypeTag();

    if (OrbTag == AllyTag)
    {
        // Only collect ally orbs that have been activated.
        // Inactive ally orbs are not yet "ready" — they should be ignored.
        if (!Orb->IsActiveOrb())
        {
            UE_LOG(LogOrbReservoir, Verbose,
                TEXT("%s: Ally orb [%s] entered trigger but is not active — skipping."),
                *GetName(), *Orb->GetName());
            return;
        }

        Orb->ConsumeOrb();

        StoredCount++;
        UE_LOG(LogOrbReservoir, Log,
            TEXT("%s: Collected ALLY orb. StoredCount = %d / %d."),
            *GetName(), StoredCount, MaxStorage);

        UpdateReservoirEmissive();

        // Broadcast the win condition exactly once when the target is reached.
        if (StoredCount == MaxStorage)
        {
            UE_LOG(LogOrbReservoir, Log, TEXT("%s: All orbs collected! Broadcasting delegate."), *GetName());
            OnAllOrbsCollected.Broadcast();
        }
    }
    else if (OrbTag == EnemyTag)
    {
        Orb->ConsumeOrb();

        // Show the hurt screen to give the player damage feedback.
        CreateHurtScreenWidget();

        // Penalize 3 orbs, but never go below zero.
        StoredCount = FMath::Clamp(StoredCount - 3, 0, MaxStorage);
        UE_LOG(LogOrbReservoir, Log,
            TEXT("%s: Hit by ENEMY orb. StoredCount = %d / %d."),
            *GetName(), StoredCount, MaxStorage);

        UpdateReservoirEmissive();
    }
    else
    {
        // Player orbs or unrecognized tags — no action.
        UE_LOG(LogOrbReservoir, Verbose,
            TEXT("%s: Orb [%s] with unhandled tag [%s] entered trigger — ignoring."),
            *GetName(), *Orb->GetName(), *OrbTag.ToString());
    }
}

// -----------------------------------------------------------------------------
// Emissive Update
// -----------------------------------------------------------------------------

void AOrbReservoir::UpdateReservoirEmissive() const
{
    // Guard: MID may be null if material setup failed in BeginPlay.
    if (!IsValid(ReservoirMID))
    {
        return;
    }

    // Scale emissive linearly with StoredCount, clamped to MaxEmissive.
    const float Target = FMath::Clamp(
        BaseEmissive + EmissivePerOrb * static_cast<float>(StoredCount),
        0.f,
        MaxEmissive
    );

    ReservoirMID->SetScalarParameterValue(EmissiveParamName, Target);
}

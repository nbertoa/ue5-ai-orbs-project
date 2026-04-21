#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "OrbReservoir.generated.h"

// Forward declarations
class UStaticMeshComponent;
class USphereComponent;
class UMaterialInstanceDynamic;
class UUserWidget;

/** Custom log category for the OrbReservoir system. */
DECLARE_LOG_CATEGORY_EXTERN(LogOrbReservoir, Log, All);

/**
 * Delegate broadcast when the reservoir reaches its maximum storage capacity.
 * Bind to this in Blueprint or C++ to trigger win-condition logic.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAllOrbsCollected);

/**
 * AOrbReservoir — The central collection point for ally orbs.
 *
 * The reservoir has a spherical trigger. When an AOrb enters it:
 *   - OrbType.Ally + active: consumed, StoredCount incremented, emissive updated.
 *     If StoredCount reaches MaxStorage, OnAllOrbsCollected is broadcast.
 *   - OrbType.Enemy: consumed, StoredCount decremented by 3 (clamped to 0),
 *     and a hurt-screen widget is shown to the player.
 *
 * The reservoir mesh's emissive intensity scales with the number of stored orbs,
 * giving visual feedback of progress toward the win condition.
 *
 * Architecture notes:
 * - The emissive parameter is driven through a UMaterialInstanceDynamic created at BeginPlay.
 *   The parameter name must match the material's scalar parameter exactly (EmissiveParamName).
 * - The hurt screen widget is created and added to viewport each time an enemy orb hits.
 *   Blueprint should implement fade-out behavior on the widget itself.
 */
UCLASS()
class AIORBSPROJECT_API AOrbReservoir : public AActor
{
    GENERATED_BODY()

public:

    /** Sets up CollectTrigger and ReservoirMesh components. Tick is disabled. */
    AOrbReservoir();

    /**
     * Returns the current number of ally orbs stored in the reservoir.
     *
     * @return Current stored orb count (0 to MaxStorage).
     */
    UFUNCTION(BlueprintCallable, Category = "Orbs")
    int32 CurrentStoredOrbs() const { return StoredCount; }

    /**
     * Returns the maximum number of ally orbs this reservoir can hold.
     *
     * @return The MaxStorage value configured in the editor.
     */
    UFUNCTION(BlueprintCallable, Category = "Orbs")
    int32 TotalStoredOrbs() const { return MaxStorage; }

    /**
     * Delegate broadcast when StoredCount reaches MaxStorage.
     * Bind in Blueprint or C++ to trigger win-condition events.
     */
    UPROPERTY(BlueprintAssignable, Category = "Orbs")
    FOnAllOrbsCollected OnAllOrbsCollected;

protected:

    /** Binds the overlap delegate and creates the dynamic material instance. */
    virtual void BeginPlay() override;

    // -------------------------------------------------------------------------
    // Components
    // -------------------------------------------------------------------------

    /**
     * Sphere trigger that detects incoming AOrb actors.
     * Set as root component — the reservoir exists at this sphere's center.
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USphereComponent> CollectTrigger;

    /** Visual mesh representing the reservoir. No collision. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> ReservoirMesh;

    /**
     * Dynamic material instance created from ReservoirMesh's material slot 0 at BeginPlay.
     * Used to drive the EmissiveParamName scalar parameter at runtime.
     * Marked Transient — not serialized, recreated each session.
     */
    UPROPERTY(Transient)
    TObjectPtr<UMaterialInstanceDynamic> ReservoirMID;

    // -------------------------------------------------------------------------
    // State
    // -------------------------------------------------------------------------

    /** Number of ally orbs currently stored. Drives the emissive intensity. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "State")
    int32 StoredCount = 0;

    // -------------------------------------------------------------------------
    // Settings
    // -------------------------------------------------------------------------

    /** Maximum number of ally orbs needed to trigger OnAllOrbsCollected. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
    int32 MaxStorage = 50;

    /**
     * Name of the scalar parameter in the reservoir material that controls emissive brightness.
     * Must exactly match the parameter name in the assigned Material Instance.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
    FName EmissiveParamName = "EmissiveStrength";

    /** Base emissive value when the reservoir is empty (StoredCount == 0). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
    float BaseEmissive = 1.0f;

    /** Emissive units added per stored orb. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
    float EmissivePerOrb = 0.75f;

    /** Maximum emissive value, regardless of orb count. Prevents material blowout. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
    float MaxEmissive = 30.0f;

    /** Widget class to instantiate and show when an enemy orb hits the reservoir. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|UI")
    TSubclassOf<UUserWidget> HurtScreenWidgetClass;

    // -------------------------------------------------------------------------
    // Internal
    // -------------------------------------------------------------------------

    /**
     * Creates and adds HurtScreenWidgetClass to the player's viewport.
     * Called each time an enemy orb reaches the reservoir.
     * Safe to call if HurtScreenWidgetClass is null — logs a warning and returns early.
     */
    UFUNCTION(BlueprintCallable, Category = "UI")
    void CreateHurtScreenWidget();

    /**
     * Overlap callback bound to CollectTrigger in BeginPlay.
     * Filters for AOrb actors and routes to ally or enemy handling logic.
     */
    UFUNCTION()
    void OnCollectTriggerBegin(
        UPrimitiveComponent* OverlappedComp,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult
    );

    /**
     * Recomputes and applies the emissive scalar parameter on ReservoirMID.
     * Called after every StoredCount change. No-ops if ReservoirMID is null.
     */
    void UpdateReservoirEmissive() const;
};

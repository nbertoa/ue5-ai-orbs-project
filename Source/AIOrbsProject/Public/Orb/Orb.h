#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "Orb.generated.h"

// Forward declarations — avoid full includes in the header
class UStaticMeshComponent;
class USphereComponent;
class USoundBase;
class UMaterialInterface;
class UNiagaraSystem;
class UBehaviorTree;

/** Custom log category for the Orb system. */
DECLARE_LOG_CATEGORY_EXTERN(LogOrb, Log, All);

/**
 * AOrb — A flying AI-driven character that represents an orb in the game world.
 *
 * Orbs use GameplayTags (OrbType.Ally / OrbType.Enemy) to define their faction.
 * Each orb runs a BehaviorTree via AOrbAIController. They can be activated or
 * deactivated visually, and consumed (destroyed with VFX/SFX feedback) when collected
 * by an AOrbReservoir or when they collide with the player.
 *
 * Architecture notes:
 * - AOrb derives from ACharacter to benefit from CharacterMovementComponent (flying mode).
 * - The collision capsule is used for movement; OrbMesh and CollisionSphere are children.
 * - AI possession and BehaviorTree startup are handled by AOrbAIController::OnPossess().
 */
UCLASS()
class AIORBSPROJECT_API AOrb : public ACharacter
{
    GENERATED_BODY()

public:

    /** Sets up components, movement parameters, and AI controller class. */
    AOrb();

    /**
     * Returns the BehaviorTree asset assigned to this orb.
     * Called by AOrbAIController::OnPossess() to start the AI tree.
     *
     * @return The assigned UBehaviorTree asset, or nullptr if none is set.
     */
    UBehaviorTree* GetBehaviorTreeAsset() const { return BehaviorTreeAsset; }

    /**
     * Deactivates the orb: switches to the default (inactive) material.
     * Safe to call multiple times — idempotent.
     */
    UFUNCTION(BlueprintCallable, Category = "Orb|State")
    void DeactivateOrb();

    /**
     * Activates the orb: switches to the active material and plays the activation sound.
     * Only ally orbs that are active will be collected by AOrbReservoir.
     */
    UFUNCTION(BlueprintCallable, Category = "Orb|State")
    void ActivateOrb();

    /**
     * Returns whether this orb is currently in the active state.
     *
     * @return True if the orb has been activated and not yet deactivated or consumed.
     */
    UFUNCTION(BlueprintCallable, Category = "Orb|State")
    bool IsActiveOrb() const { return bIsActiveOrb; }

    /**
     * Returns the GameplayTag that identifies this orb's faction/type.
     * Expected values: OrbType.Ally, OrbType.Enemy, OrbType.Player.
     *
     * @return The orb type tag set in the editor.
     */
    UFUNCTION(BlueprintCallable, Category = "Orb|State")
    FGameplayTag GetOrbTypeTag() const { return OrbTypeTag; }

    /**
     * Destroys this orb, spawning VFX and playing SFX at its location first.
     * Called by AOrbReservoir when an orb enters its collection trigger.
     */
    void ConsumeOrb();

protected:

    /** Registers the overlap delegate on CollisionSphere. */
    virtual void BeginPlay() override;

    /** Plays ActivatedSound at this orb's world location, if the asset is assigned. */
    void PlayActivatedSound();

    // -------------------------------------------------------------------------
    // Components
    // -------------------------------------------------------------------------

    /** Visual sphere mesh. No collision — all collision is handled by CollisionSphere. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<UStaticMeshComponent> OrbMesh;

    /**
     * Overlap sphere used to detect proximity to the player.
     * Enemy orbs use this to trigger ConsumeOrb() when touching the player character.
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
    TObjectPtr<USphereComponent> CollisionSphere;

    // -------------------------------------------------------------------------
    // Settings — Audio
    // -------------------------------------------------------------------------

    /** Sound played when ActivateOrb() is called. Optional. */
    UPROPERTY(EditAnywhere, Category = "Settings|SFX")
    TObjectPtr<USoundBase> ActivatedSound;

    /** Sound played when ConsumeOrb() is called. Optional. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|SFX")
    TObjectPtr<USoundBase> DestroySFX;

    // -------------------------------------------------------------------------
    // Settings — Visuals
    // -------------------------------------------------------------------------

    /** Material applied to OrbMesh when the orb is inactive. */
    UPROPERTY(EditAnywhere, Category = "Settings|Visuals")
    TObjectPtr<UMaterialInterface> DefaultMaterial;

    /** Material applied to OrbMesh when the orb is active. */
    UPROPERTY(EditAnywhere, Category = "Settings|Visuals")
    TObjectPtr<UMaterialInterface> ActiveMaterial;

    /** Niagara system spawned at the orb's location when ConsumeOrb() is called. Optional. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|VFX")
    TObjectPtr<UNiagaraSystem> DestroyVFX;

    // -------------------------------------------------------------------------
    // Settings — Gameplay
    // -------------------------------------------------------------------------

    /**
     * Gameplay tag that defines the faction of this orb.
     * Must match one of the tags defined in DefaultGameplayTags.ini:
     *   OrbType.Ally, OrbType.Enemy, OrbType.Player
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
    FGameplayTag OrbTypeTag;

    // -------------------------------------------------------------------------
    // Settings — AI
    // -------------------------------------------------------------------------

    /**
     * BehaviorTree asset to run on this orb when possessed by AOrbAIController.
     * Must include a BlackboardAsset — InitializeBlackboard() will fail otherwise.
     */
    UPROPERTY(EditAnywhere, Category = "Settings|AI")
    TObjectPtr<UBehaviorTree> BehaviorTreeAsset;

    // -------------------------------------------------------------------------
    // Internal
    // -------------------------------------------------------------------------

    /**
     * Overlap callback bound to CollisionSphere in BeginPlay().
     * Enemy orbs consume themselves when they touch the player character.
     */
    UFUNCTION()
    void OnOverlapBegin(
        UPrimitiveComponent* OverlappedComp,
        AActor* OtherActor,
        UPrimitiveComponent* OtherComp,
        int32 OtherBodyIndex,
        bool bFromSweep,
        const FHitResult& SweepResult
    );

    /** Whether this orb is currently in the active state. Drives material selection. */
    bool bIsActiveOrb = false;
};

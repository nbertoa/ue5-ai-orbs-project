#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "OrbAIController.generated.h"

// Forward declarations
class UBlackboardComponent;
class UBehaviorTreeComponent;

/** Custom log category for the OrbAIController system. */
DECLARE_LOG_CATEGORY_EXTERN(LogOrbAIController, Log, All);

/**
 * AOrbAIController — AI Controller responsible for running the BehaviorTree on an AOrb.
 *
 * When this controller possesses an AOrb, it reads the orb's assigned BehaviorTree asset,
 * initializes the Blackboard from that asset, and starts running the tree.
 *
 * Architecture notes:
 * - This controller creates its own UBlackboardComponent and UBehaviorTreeComponent
 *   in the constructor rather than relying on the base class defaults, giving us
 *   explicit named references for debugging.
 * - If the possessed pawn is not an AOrb, or if the orb has no BehaviorTree assigned,
 *   a warning is logged and the AI remains idle.
 */
UCLASS()
class AIORBSPROJECT_API AOrbAIController : public AAIController
{
    GENERATED_BODY()

public:

    /** Initializes BlackboardComp and BehaviorComp as subobjects. */
    AOrbAIController();

protected:

    /**
     * Called when this controller takes possession of a pawn.
     * Attempts to cast InPawn to AOrb and start its BehaviorTree.
     *
     * @param InPawn The pawn being possessed. Expected to be an AOrb instance.
     */
    virtual void OnPossess(APawn* InPawn) override;

    // -------------------------------------------------------------------------
    // Components
    // -------------------------------------------------------------------------

    /**
     * Blackboard component initialized from the BehaviorTree's BlackboardAsset.
     * Keys are set by BehaviorTree tasks and decorators at runtime.
     */
    UPROPERTY(VisibleAnywhere, Category = "AI")
    TObjectPtr<UBlackboardComponent> BlackboardComp;

    /**
     * BehaviorTree component that drives the orb's AI logic.
     * Started in OnPossess() once the blackboard is initialized.
     */
    UPROPERTY(VisibleAnywhere, Category = "AI")
    TObjectPtr<UBehaviorTreeComponent> BehaviorComp;
};

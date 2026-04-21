// Copyright Epic Games, Inc. All Rights Reserved.

#include "Orb/OrbAIController.h"

#include "BehaviorTree/BehaviorTree.h"
#include "BehaviorTree/BehaviorTreeComponent.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "Orb/Orb.h"

DEFINE_LOG_CATEGORY(LogOrbAIController);

// -----------------------------------------------------------------------------
// Constructor
// -----------------------------------------------------------------------------

AOrbAIController::AOrbAIController()
{
    // Create named components so they appear in the details panel and are
    // easily identifiable in the debugger / AI debugger overlay.
    BlackboardComp = CreateDefaultSubobject<UBlackboardComponent>(TEXT("BlackboardComp"));
    BehaviorComp   = CreateDefaultSubobject<UBehaviorTreeComponent>(TEXT("BehaviorComp"));
}

// -----------------------------------------------------------------------------
// OnPossess
// -----------------------------------------------------------------------------

void AOrbAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);

    // Guard: we only know how to drive AOrb pawns.
    AOrb* OrbPawn = Cast<AOrb>(InPawn);
    if (!OrbPawn)
    {
        UE_LOG(LogOrbAIController, Warning,
            TEXT("%s: OnPossess — possessed pawn [%s] is not an AOrb. AI will not start."),
            *GetName(), InPawn ? *InPawn->GetName() : TEXT("nullptr"));
        return;
    }

    // Guard: the orb must have a BehaviorTree assigned in its Blueprint.
    UBehaviorTree* BTAsset = OrbPawn->GetBehaviorTreeAsset();
    if (!IsValid(BTAsset))
    {
        UE_LOG(LogOrbAIController, Warning,
            TEXT("%s: OnPossess — AOrb [%s] has no BehaviorTreeAsset assigned. AI will not start."),
            *GetName(), *OrbPawn->GetName());
        return;
    }

    // Guard: the BehaviorTree must reference a BlackboardAsset or InitializeBlackboard will crash.
    if (!IsValid(BTAsset->BlackboardAsset))
    {
        UE_LOG(LogOrbAIController, Warning,
            TEXT("%s: OnPossess — BehaviorTree [%s] has no BlackboardAsset. AI will not start."),
            *GetName(), *BTAsset->GetName());
        return;
    }

    // Initialize the blackboard from the tree's asset.
    // This populates the blackboard with the key schema defined in the asset.
    BlackboardComp->InitializeBlackboard(*BTAsset->BlackboardAsset);

    // Start running the behavior tree. From this point, the BT tasks and
    // decorators drive the orb's movement and decisions each frame.
    BehaviorComp->StartTree(*BTAsset);

    UE_LOG(LogOrbAIController, Log,
        TEXT("%s: OnPossess — BehaviorTree [%s] started on orb [%s]."),
        *GetName(), *BTAsset->GetName(), *OrbPawn->GetName());
}

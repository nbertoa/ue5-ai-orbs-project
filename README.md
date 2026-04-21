# UE5 – AI Orbs Project

A first-person game prototype built in **Unreal Engine 5.6** with **C++ and Blueprints**, focused on AI systems: Behavior Trees, Blackboards, Tasks, Services, and Decorators.

📝 [Blog post](https://nbertoa.wordpress.com/2025/09/10/unreal-5-6-ai-orbs-project/) · 🎮 [Video demo](https://www.youtube.com/watch?v=YdxnxSYz0ss)

---

## Gameplay

The player must collect **friendly (ally) orbs** and guide them to a central **reservoir** until it is completely filled to win. Meanwhile, **enemy orbs** move autonomously toward the reservoir — if they reach it before being destroyed, they subtract from the reservoir's stored count. The reservoir mesh glows progressively brighter as it fills, providing constant visual feedback of progress.

---

## Architecture

### C++ Classes

| Class | Base | Responsibility |
|---|---|---|
| `AOrb` | `ACharacter` | Flying orb actor — faction tag, active/inactive material state, VFX/SFX on consume, overlap detection |
| `AOrbAIController` | `AAIController` | Possesses `AOrb`; initializes the Blackboard and starts the correct Behavior Tree per orb type |
| `AOrbReservoir` | `AActor` | Central collection point; scores ally orbs, penalizes enemy orbs, drives emissive material, broadcasts win event |
| `AAIOrbsProjectCharacter` | `ACharacter` | First-person player character with Enhanced Input bindings |
| `AAIOrbsProjectPlayerController` | `APlayerController` | Registers Input Mapping Contexts; assigns the custom camera manager |
| `AAIOrbsProjectCameraManager` | `APlayerCameraManager` | Clamps vertical look pitch for the first-person camera |

### Orb Faction System

Each orb carries a **Gameplay Tag** (`OrbTypeTag`) that identifies its faction:

- `OrbType.Ally` — wanders by default; follows the player on proximity; heads to the reservoir when close enough.
- `OrbType.Enemy` — moves directly to the reservoir.
- `OrbType.Player` — reserved for the player character tag used by AI perception.

### Behavior Trees

Ally and enemy orbs run **separate Behavior Tree assets** but share the same `AOrbAIController`. The controller reads `GetBehaviorTreeAsset()` from the possessed `AOrb` and starts the appropriate tree — no controller subclassing required per orb type.

**Ally orb BT logic (simplified):**
```
Selector
├── Sequence [close to reservoir AND active]
│   └── Task: MoveToReservoir
├── Sequence [player in range]
│   └── Task: FollowPlayer
└── Task: Wander
```

**Enemy orb BT logic (simplified):**
```
Sequence
└── Task: MoveToReservoir
```

### OrbReservoir Scoring

| Event | Effect |
|---|---|
| Active ally orb enters trigger | `StoredCount++` · update emissive · broadcast win if full |
| Enemy orb enters trigger | `StoredCount -= EnemyOrbPenalty` (clamped to 0) · show hurt screen widget |

---

## Key Technical Decisions

**`CharacterMovementComponent` in Flying mode for orbs** — Using `ACharacter` as the orb base gives access to `UCharacterMovementComponent`, which handles NavMesh pathfinding integration, smooth rotation toward targets (`bUseControllerDesiredRotation`), and configurable acceleration/braking — far less code than a custom movement solution.

**`TObjectPtr<T>` for all UPROPERTY pointers** — Enables UE5's access tracking and lazy asset loading. All component and asset references in this project use `TObjectPtr` instead of raw pointers.

**Gameplay Tags for faction identification** — Tags decouple faction logic from class hierarchy. The same `AOrbAIController` and `AOrbReservoir` can handle any number of orb types by checking `OrbTypeTag` rather than casting to specific subclasses.

**Event-driven design** — `AOrbReservoir` has `PrimaryActorTick.bCanEverTick = false`. All scoring logic runs from `OnComponentBeginOverlap` delegates. No polling.

**Dynamic Material Instance for emissive feedback** — `ReservoirMID` is created at `BeginPlay` from mesh slot 0. `UpdateReservoirEmissive()` drives the `EmissiveStrength` scalar parameter each time `StoredCount` changes, making the fill level visible without any UI widget on the reservoir itself.

---

## Project Structure

```
AIOrbsProject/
├── Config/
│   ├── DefaultEngine.ini       # Collision profiles, rendering settings
│   ├── DefaultGame.ini
│   ├── DefaultGameplayTags.ini # OrbType tag definitions
│   └── DefaultInput.ini        # Enhanced Input axis config
├── Source/
│   └── AIOrbsProject/
│       ├── AIOrbsProject.h/.cpp            # Module entry point
│       ├── AIOrbsProject.Build.cs          # Module dependencies
│       ├── AIOrbsProjectCharacter.h/.cpp   # FP player character
│       ├── AIOrbsProjectPlayerController.h/.cpp
│       ├── AIOrbsProjectCameraManager.h/.cpp
│       ├── AIOrbsProjectGameMode.h/.cpp
│       └── Orb/
│           ├── Orb.h/.cpp                  # Orb actor
│           ├── OrbAIController.h/.cpp      # AI controller
│           └── OrbReservoir.h/.cpp         # Collection point
└── AIOrbsProject.uproject
```

---

## Dependencies

- Unreal Engine 5.6
- Modules: `AIModule`, `GameplayTags`, `NavigationSystem`, `Niagara`, `EnhancedInput`, `UMG`, `StateTreeModule`

---

## Learning Source

[Udemy – UE5 AI Crash Course](https://www.udemy.com/course/ue5-ai-crash-course/)

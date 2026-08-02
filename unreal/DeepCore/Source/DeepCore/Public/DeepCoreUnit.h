// DeepCoreUnit.h : an articulated brick figure.
//
// WHAT MAKES A CHARACTER LOOK ALIVE
// ---------------------------------
// Not polygon count. The three things that actually read, in order of how much they matter:
//
//  1. Feet that do not slide. The gait phase here advances by DISTANCE TRAVELLED, not by
//     elapsed time, which makes foot-slide impossible by construction rather than by tuning.
//     A character whose feet skate looks wrong to everyone and fixable to no one.
//  2. Secondary motion. Nothing stops dead: the torso lags the hips through a spring, the
//     helmet lags the torso, and both settle rather than snap. This is most of the difference
//     between "posed" and "animated".
//  3. Anticipation. A swing winds back before it strikes. AttackCurve goes deliberately
//     negative before it goes positive for exactly this reason.
//
// All of that maths lives in Anim.hpp, shared verbatim with the OpenGL build and covered by
// the harness. This file is only the skeleton it drives.
//
// ON THE DESIGN
// These figures are NOT minifigures -- that silhouette is a registered trade mark. They have
// tapered bodies, articulated knees, and rounded helmets, which is both legally the point and
// visually better here: real knees are what let the walk cycle read at all.
//

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Anim.hpp"
#include "DeepCoreUnit.generated.h"

class UProceduralMeshComponent;
class USpotLightComponent;
class ADeepCoreTerrain;

UENUM()
enum class EDeepCoreUnitKind : uint8
{
	Miner,
	Monster
};

UCLASS()
class ADeepCoreUnit : public AActor
{
	GENERATED_BODY()

public:
	ADeepCoreUnit();

	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;

	/** Assemble the body. Must be called before the unit is first ticked. */
	void Build(EDeepCoreUnitKind InKind, const FLinearColor& Accent, float InScale);

	/** Walk toward a world position; arrival is reported by HasArrived(). */
	void OrderMoveTo(const FVector& World) { Destination = World; bHasOrder = true; }

	bool HasArrived() const { return !bHasOrder; }

	EDeepCoreUnitKind GetKind() const { return Kind; }

	/** Play the wind-up-and-strike swing. Drives arms and torso through AttackCurve. */
	void Strike() { AttackTime = 0.0f; bAttacking = true; }

	/** Creatures roam on their own; crew wait to be told. */
	void SetWanders(bool bIn) { bWanders = bIn; }

	UPROPERTY() TObjectPtr<ADeepCoreTerrain> Terrain;

private:
	void BuildMiner(const FLinearColor& Accent);
	void BuildMonster(const FLinearColor& Accent);

	/** One rigid brick part hung off a pivot. */
	UProceduralMeshComponent* MakePart(USceneComponent* Parent, const TCHAR* Name);

	/**
	 * The actor root, at FOOT level.
	 *
	 * The hips deliberately are NOT the root. Walk bob and breathing move the hips relative to
	 * the feet, and if the hips were the root then writing that offset would rewrite the
	 * actor's own world position -- zeroing X and Y along with it and teleporting the unit to
	 * the map corner every frame. Root at the feet also means placing a unit is just "put it
	 * on the floor" rather than "put it on the floor plus a leg length".
	 */
	UPROPERTY() TObjectPtr<USceneComponent>          Root;
	UPROPERTY() TObjectPtr<USceneComponent>          Pivot;      ///< hips
	UPROPERTY() TObjectPtr<USceneComponent>          TorsoPivot;
	UPROPERTY() TObjectPtr<USceneComponent>          HeadPivot;
	UPROPERTY() TObjectPtr<USceneComponent>          HipL;
	UPROPERTY() TObjectPtr<USceneComponent>          HipR;
	UPROPERTY() TObjectPtr<USceneComponent>          KneeL;
	UPROPERTY() TObjectPtr<USceneComponent>          KneeR;
	UPROPERTY() TObjectPtr<USceneComponent>          ShoulderL;
	UPROPERTY() TObjectPtr<USceneComponent>          ShoulderR;

	UPROPERTY() TArray<TObjectPtr<UProceduralMeshComponent>> Parts;

	/**
	 * The cap lamp. With no sun underground this is the game's key light, not a decoration:
	 * five crew members carrying five of these ARE the lighting rig, and the pools they throw
	 * are the only reason any of the rock is visible at all.
	 */
	UPROPERTY() TObjectPtr<USpotLightComponent> Lamp;

	EDeepCoreUnitKind Kind  = EDeepCoreUnitKind::Miner;
	float   Scale           = 1.0f;
	float   HipHeight       = 62.0f;  ///< hips above the feet, in local units
	float   Gait            = 0.0f;   ///< 0..1 phase, advanced by distance
	float   Speed           = 260.0f; ///< cm/s
	float   Yaw             = 0.0f;
	float   AttackTime      = 0.0f;
	bool    bAttacking      = false;
	bool    bHasOrder       = false;
	bool    bBuilt          = false;
	bool    bWanders        = false;
	float   WanderPause     = 0.0f;   ///< creatures stand and look around between moves
	uint32  WanderState     = 0u;
	FVector Destination     = FVector::ZeroVector;

	/** Choose somewhere reachable to roam to. Returns false if the map offers nowhere. */
	bool PickWanderTarget();

	/** Secondary motion. The torso and head chase the hips instead of being welded to them. */
	Anim::Spring TorsoLag;
	Anim::Spring HeadLag;
};

// DeepCoreGame.h : the game mode and the camera that looks at it.
//
// The startup map is /Engine/Maps/Entry -- an empty engine map. Everything visible, including
// the lights, the fog and the post-processing, is spawned here in C++. That is what lets this
// project contain no content assets at all: there is no level to save, because the level is
// code.
//

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/Pawn.h"
#include "DeepCoreGame.generated.h"

class ADeepCoreTerrain;
class ADeepCoreUnit;
class USpringArmComponent;
class UCameraComponent;

/**
 * An overhead camera on a spring arm.
 *
 * Keys are polled rather than bound through an input mapping, because a mapping lives in a
 * config file and this project's whole premise is that behaviour lives in reviewable code.
 */
UCLASS()
class ADeepCorePawn : public APawn
{
	GENERATED_BODY()

public:
	ADeepCorePawn();

	virtual void Tick(float DeltaSeconds) override;
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* Input) override;

	UPROPERTY() TObjectPtr<USpringArmComponent> Arm;
	UPROPERTY() TObjectPtr<UCameraComponent>    Cam;

private:
	void ZoomIn();
	void ZoomOut();
	void OnClick();

	/**
	 * Default view distance.
	 *
	 * With the 38-degree lens this frames roughly fifteen tiles across -- enough cavern to
	 * plan a route through, while a 147cm crew member is still tall enough on screen to read
	 * their walk cycle. Widening the lens instead of pulling back would show more map, but it
	 * is what flattened the brickwork in the first place.
	 */
	float Boom = 2200.0f;
};

UCLASS()
class ADeepCoreGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	ADeepCoreGameMode();

	virtual void BeginPlay() override;

	/** Order the nearest idle crew member to a world point. */
	void OrderNearestTo(const FVector& World);

	UPROPERTY() TObjectPtr<ADeepCoreTerrain>          Terrain;
	UPROPERTY() TArray<TObjectPtr<ADeepCoreUnit>>     Crew;
	UPROPERTY() TArray<TObjectPtr<ADeepCoreUnit>>     Creatures;

private:
	/** Directional key light, sky fill, fog and a fixed-exposure post volume. */
	void BuildLighting();

	/** Find an open tile, preferring ones near the middle of a chamber. */
	bool FindOpenTile(int32& OutX, int32& OutY, int32 Attempt) const;
};

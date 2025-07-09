// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "CubePawn.h"
#include "ServerP1GameMode.generated.h"

UCLASS(minimalapi)
class AServerP1GameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	AServerP1GameMode();

	virtual void Tick(float DeltaTime) override;
	
public:
	void SpawnCubePawn();
	void SpawnCubes();

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	int32 frameCountPerSecond = 60;

	float fElapsed = 0.f;
	float fFrameTime = 1.f;

	int32 maxCol = 5;
	int32 maxRow = 6;

	UPROPERTY(EditAnywhere)
	TSubclassOf<ACubePawn> PlayerClass;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AActor> CubeClass;

	// World에 Spawn된 오브젝트
public:
	TArray<AActor*> spawnedCubes;
	ACubePawn* playerCube;

	void InsertInput();
	void ApplyInput();

	TQueue<FMyInput> qInput;

public:
	void HandleLockstep(FLockstepPacket& pkt);
	void HandleSync(FSyncPacket& pkt);
	void HandleSnapshot(FSnapshotPacket& pkt);

};




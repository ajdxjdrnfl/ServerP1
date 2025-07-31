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
	AServerP1GameMode(const FObjectInitializer& ObjectInitializer);
	
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	
public:
	void SpawnCubePawn();
	void SpawnCubes();

public:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	int32 frameCountPerSecond = 60;

	float fElapsed = 0.f;
	float fFrameTime = 1.f;

	int32 maxCol = 3;
	int32 maxRow = 3;

	UPROPERTY(EditAnywhere)
	TSubclassOf<ACubePawn> PlayerClass;

	UPROPERTY(EditDefaultsOnly)
	TSubclassOf<AActor> CubeClass;

	UPROPERTY(EditAnywhere)
	FVector Origin {
		1000.f, 1000.f, 1000.f
	};
	// World에 Spawn된 오브젝트
public:
	TArray<AActor*> spawnedCubes;
	ACubePawn* playerCube;
	
// Input 관련
	void ApplyInput(FMyInput& input, float deltaTime);
	void InsertInput(int32 nSeq, FMyInput& item);
	bool PopInput(FMyInput& item);
	void SetSimulatePhysics(float DeltaTime, bool bSimulate);

// State 관련
	void InsertState();

// 자료구조
	// Delayed Out Buffer용
	TQueue<TPair<float, int32>> qCurSeq;
	TQueue<FMyInput> qInput;
	
	int32 nCurSeq;

	// State 저장용
	TMap<AActor*, FMyState> prevStates;

public:
	// 패킷 처리 관련
	void HandleLockstep(FLockstepPacket& pkt);
	void HandleSync(FSyncPacket& pkt);
	void HandleSnapshot(FSnapshotPacket& pkt);
	void HandleAck(FAckPacket& pkt);

private:
	// 로직 관련
	void HandleTick(float DeltaTime);
	void HandleTick_Svr(float DeltaTime);
	void HandleTick_Cli(float DeltaTime);
	void HandleTick_Impl(float DeltaTime);
	void TickLockstep(float DeltaTime);
	void TickSync(float DeltaTime);
	void TickSnapshot(float DeltaTime);
	void TickAck(float DeltaTime); 

	int32 GetCurrentSeq();
};




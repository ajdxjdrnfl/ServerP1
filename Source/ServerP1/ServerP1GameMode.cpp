// Copyright Epic Games, Inc. All Rights Reserved.

#include "ServerP1GameMode.h"
#include "ServerP1Character.h"
#include "UObject/ConstructorHelpers.h"

AServerP1GameMode::AServerP1GameMode()
{
	// set default pawn class to our Blueprinted character
	/*
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnBPClass(TEXT("/Game/ThirdPerson/Blueprints/BP_ThirdPersonCharacter"));
	if (PlayerPawnBPClass.Class != NULL)
	{
		DefaultPawnClass = PlayerPawnBPClass.Class;
	}
	*/

	fFrameTime = 1.f / float(frameCountPerSecond);
}

void AServerP1GameMode::SpawnCubePawn()
{
	FVector Location = FVector(0.f, 0.f, 0.f);
	FRotator Rotation = FRotator::ZeroRotator;

	playerCube = GetWorld()->SpawnActor<ACubePawn>(PlayerClass, Location, Rotation);
}

void AServerP1GameMode::SpawnCubes()
{
	FRotator Rotation = FRotator::ZeroRotator;
	int width = 100;
	int height = 100;
	for (int i = 0; i < maxRow; i++)
	{
		for (int j = 0; j < maxCol; j++)
		{
			int x = i - (maxRow / 2);
			int y = j - (maxCol / 2);
			FVector Location = FVector(x * width, y * height, 0.f);
			AActor* actor = GetWorld()->SpawnActor<AActor>(CubeClass, Location, Rotation);
			spawnedCubes.Push(actor);
		}
	}
}

void AServerP1GameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	fElapsed += DeltaTime;

	float dt = DeltaTime;

	if (fElapsed >= fFrameTime)
	{
		fElapsed -= fFrameTime;
	}
}

void AServerP1GameMode::HandleLockstep(FLockstepPacket& pkt)
{
	
}

void AServerP1GameMode::HandleSync(FSyncPacket& pkt)
{

}

void AServerP1GameMode::HandleSnapshot(FSnapshotPacket& pkt)
{

}
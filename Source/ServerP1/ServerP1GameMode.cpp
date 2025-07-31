// Copyright Epic Games, Inc. All Rights Reserved.

#include "ServerP1GameMode.h"
#include "ServerP1Character.h"
#include "MyGameInstance.h"
#include "UObject/ConstructorHelpers.h"
#include "Physics/Experimental/PhysScene_Chaos.h"
#include "Kismet/GameplayStatics.h"

AServerP1GameMode::AServerP1GameMode(const FObjectInitializer& ObjectInitializer)
	:Super(ObjectInitializer)
{
	fFrameTime = 1.f / float(frameCountPerSecond);
	PrimaryActorTick.bCanEverTick = true;
	nCurSeq = 0;
}

void AServerP1GameMode::SpawnCubePawn()
{
	FRotator Rotation = FRotator::ZeroRotator;

	playerCube = GetWorld()->SpawnActor<ACubePawn>(PlayerClass, Origin, Rotation);
	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);

	if (PC && playerCube)
	{
		PC->Possess(playerCube);
	}
}

void AServerP1GameMode::SpawnCubes()
{
	FRotator Rotation = FRotator::ZeroRotator;
	int width = 400;
	int height = 400;
	for (int i = 0; i < maxRow; i++)
	{
		for (int j = 0; j < maxCol; j++)
		{
			int x = i - (maxRow / 2);
			int y = j - (maxCol / 2);
			FVector Location = FVector(x * width, y * height, 0.f);
			AActor* actor = GetWorld()->SpawnActor<AActor>(CubeClass, Origin + Location, Rotation);
			spawnedCubes.Push(actor);
		}
	}
}

void AServerP1GameMode::BeginPlay()
{
	Super::BeginPlay();
	//PrimaryActorTick.bCanEverTick = true;
	//PrimaryActorTick.SetTickFunctionEnable(true);
}

void AServerP1GameMode::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	fElapsed += DeltaTime;
	
	// 물리에 관한 Tick 처리
	while (fElapsed >= fFrameTime)
	{
		float dt = fFrameTime;

		HandleTick(dt);
		fElapsed -= fFrameTime;
	}
}

void AServerP1GameMode::ApplyInput(FMyInput& input, float deltaTime)
{
	float scale = 100.f;
	float speed = scale * deltaTime;
	if (input.up)
		playerCube->MoveVertical(speed);
	if (input.down)
		playerCube->MoveVertical(-speed);
	if (input.left)
		playerCube->MoveHorizontal(speed);
	if (input.right)
		playerCube->MoveHorizontal(-speed);
	if (input.jump)
		playerCube->Jump();
}

void AServerP1GameMode::InsertInput(int32 nSeq, FMyInput& item)
{
	//vInput.Push(nSeq, item);
	qInput.Enqueue(item);
}

bool AServerP1GameMode::PopInput(FMyInput& item)
{
	/*
	if(!vInput.Pop(item))
		return false;
		
	return true;
	*/

	if (!qInput.Dequeue(item))
		return false;

	return true;
}

void AServerP1GameMode::SetSimulatePhysics(float DeltaTime, bool bSimulate)
{
	TArray<UPrimitiveComponent*> PrimitiveComponents;

	if (playerCube)
		playerCube->GetComponents<UPrimitiveComponent>(PrimitiveComponents);

	for (auto* cube : spawnedCubes)
	{
		if (cube)
			cube->GetComponents<UPrimitiveComponent>(PrimitiveComponents);
	}

	for (UPrimitiveComponent* Comp : PrimitiveComponents)
	{
		if (Comp->IsSimulatingPhysics())
		{
			AActor* owner = Comp->GetOwner();
			Comp->SetSimulatePhysics(bSimulate);
			owner->SetActorTickEnabled(bSimulate);
			// 물리 엔진 멈춰
			if (!bSimulate)
			{
				Comp->SetPhysicsLinearVelocity(FVector::ZeroVector);
				Comp->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
				
			}
			// 다시 재개해
			else
			{
				Comp->SetPhysicsLinearVelocity(prevStates[owner].velocity);
				Comp->SetPhysicsAngularVelocityInDegrees(prevStates[owner].angularVelocity);
			}
		}
	}
	
}

void AServerP1GameMode::InsertState()
{
	
}

void AServerP1GameMode::HandleLockstep(FLockstepPacket& pkt)
{
	InsertInput(pkt.nSeq, pkt.input);
}

void AServerP1GameMode::HandleSync(FSyncPacket& pkt)
{
	for(int i=0; i<pkt.nInputSize; i++)
		InsertInput(pkt.nSeq, pkt.inputs[i]);
	
}

void AServerP1GameMode::HandleSnapshot(FSnapshotPacket& pkt)
{

}

void AServerP1GameMode::HandleAck(FAckPacket& pkt)
{

}

//// TICK

void AServerP1GameMode::HandleTick(float DeltaTime)
{
	UMyGameInstance* gameInstance = GET_GAMEINSTANCE();
	bool bServer = gameInstance->IsServer();

	if (bServer)
	{
		// 우선 Lockstep으로 테스트
		HandleTick_Svr(DeltaTime);
	}
	else
	{
		HandleTick_Cli(DeltaTime);
	}

	HandleTick_Impl(DeltaTime);
}

void AServerP1GameMode::HandleTick_Svr(float DeltaTime)
{
	// 우선 락스텝만 구현
	UMyGameInstance* gameInstance = GET_GAMEINSTANCE();

	// 입력 Insert
	if (gameInstance->IsServer() && playerCube)
	{
		FMyInput curInput = playerCube->GetCurInput();
		InsertInput(nCurSeq++, curInput);
		playerCube->ClearInput();
		
		{
			FLockstepPacket pkt;
			pkt.nSeq = nCurSeq;
			pkt.input = curInput;
			SendBufferRef sendBuffer = PacketSession::MakeSendBuffer(pkt, EPacketType::Lockstep, gameInstance->GetCurrentSeq());
			SEND_PACKET(sendBuffer);
		}
		
	}

	// 스테이트 Insert

	TickAck(DeltaTime);
}

void AServerP1GameMode::HandleTick_Cli(float DeltaTime)
{
	// 우선 락스텝만 구현
	TickLockstep(DeltaTime);
}

void AServerP1GameMode::HandleTick_Impl(float DeltaTime)
{
	// 우선 락스텝만 구현
	// Input을 뽑아서 쓴다
	bool bSimulate = false;
	FMyInput item;
	if (PopInput(item))
	{
		ApplyInput(item, DeltaTime);
		bSimulate = true;
	}

	SetSimulatePhysics(DeltaTime, bSimulate);
}

// 굳이 뭐 안해도 될듯?

void AServerP1GameMode::TickLockstep(float DeltaTime)
{
	// 이전 상태 저장 

	TArray<UPrimitiveComponent*> PrimitiveComponents;

	if (playerCube)
		playerCube->GetComponents<UPrimitiveComponent>(PrimitiveComponents);

	for (auto* cube : spawnedCubes)
	{
		if (cube)
			cube->GetComponents<UPrimitiveComponent>(PrimitiveComponents);
	}

	// 왜 이름이 Clear가 아니라 Empty임?
	prevStates.Empty();

	for (UPrimitiveComponent* Comp : PrimitiveComponents)
	{
		if (Comp->IsSimulatingPhysics())
		{
			AActor* owner = Comp->GetOwner();

			FVector velocity = Comp->GetPhysicsLinearVelocity();
			FVector angularVelocity = Comp->GetPhysicsAngularVelocityInDegrees();
			FVector position = owner->GetActorLocation();
			FQuat rotation = owner->GetActorQuat();

			prevStates[owner] = FMyState{ position, rotation, velocity, angularVelocity };
		}
	}
}

void AServerP1GameMode::TickSync(float DeltaTime)
{
//	__noop;
}

void AServerP1GameMode::TickSnapshot(float DeltaTime)
{
//	__noop;
}

void AServerP1GameMode::TickAck(float DeltaTime)
{
//	__noop;
}

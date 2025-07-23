// Copyright Epic Games, Inc. All Rights Reserved.

#include "ServerP1GameMode.h"
#include "ServerP1Character.h"
#include "MyGameInstance.h"
#include "UObject/ConstructorHelpers.h"
#include "Physics/Experimental/PhysScene_Chaos.h"

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

	nCurSeq = 0;
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
	
	// ������ ���� Tick ó��
	while (fElapsed >= fFrameTime)
	{
		float dt = FMath::Min(fFrameTime, fElapsed);

		HandleTick(dt);
		fElapsed -= fFrameTime;
	}
}

void AServerP1GameMode::ApplyInput(FMyInput& input, float deltaTime)
{
	if (input.up)
		playerCube->MoveVertical(deltaTime);
	if (input.down)
		playerCube->MoveVertical(-deltaTime);
	if (input.left)
		playerCube->MoveHorizontal(deltaTime);
	if (input.right)
		playerCube->MoveHorizontal(-deltaTime);
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
			// ���� ���� ����
			if (!bSimulate)
			{
				Comp->SetPhysicsLinearVelocity(FVector::ZeroVector);
				Comp->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
				
			}
			// �ٽ� �簳��
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
		// �켱 Lockstep���� �׽�Ʈ
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
	// �켱 �����ܸ� ����
	UMyGameInstance* gameInstance = GET_GAMEINSTANCE();

	// �Է� Insert
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

	// ������Ʈ Insert

	TickAck(DeltaTime);
}

void AServerP1GameMode::HandleTick_Cli(float DeltaTime)
{
	// �켱 �����ܸ� ����
	TickLockstep(DeltaTime);
}

void AServerP1GameMode::HandleTick_Impl(float DeltaTime)
{
	// �켱 �����ܸ� ����
	// Input�� �̾Ƽ� ����
	bool bSimulate = false;
	FMyInput item;
	if (PopInput(item))
	{
		ApplyInput(item, DeltaTime);
		bSimulate = true;
	}

	SetSimulatePhysics(DeltaTime, bSimulate);
}

// ���� �� ���ص� �ɵ�?

void AServerP1GameMode::TickLockstep(float DeltaTime)
{
	// ���� ���� ���� 

	TArray<UPrimitiveComponent*> PrimitiveComponents;

	if (playerCube)
		playerCube->GetComponents<UPrimitiveComponent>(PrimitiveComponents);

	for (auto* cube : spawnedCubes)
	{
		if (cube)
			cube->GetComponents<UPrimitiveComponent>(PrimitiveComponents);
	}

	// �� �̸��� Clear�� �ƴ϶� Empty��?
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
	__noop;
}

void AServerP1GameMode::TickSnapshot(float DeltaTime)
{
	__noop;
}

void AServerP1GameMode::TickAck(float DeltaTime)
{
	__noop;
}
// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "ServerP1.h"
#include "CubePawn.h"
#include "MyGameInstance.generated.h"

/**
 * 
 */

class FSocket;

UCLASS()
class SERVERP1_API UMyGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	void InitSocket(bool bServer = true);

	UFUNCTION(BlueprintCallable)
	void HandleRecvPacket();

	void SendPacket(SendBufferRef buffer);

	// HandlePacket
public:
	void HandleLockstep();
	void HandleSnapshot();
	void HandleSync();

public:
	FSocket* Socket;

	FString IpAddress = TEXT("127.0.0.1");
	int16 serverPort = 7777;
	int16 clientPort = 7778;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	int32 maxSize = 2 * 1024 * 1024;
	
	TSharedPtr<PacketSession> mySession;
	void InsertInput();
	void ApplyInput();

	TQueue<FMyInput> qInput;
};

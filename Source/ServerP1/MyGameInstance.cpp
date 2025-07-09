// Fill out your copyright notice in the Description page of Project Settings.


#include "MyGameInstance.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "ServerP1GameMode.h"
#include "Common/UdpSocketBuilder.h"

void UMyGameInstance::InitSocket(bool bServer)
{
	Socket = FUdpSocketBuilder(TEXT("UDPSocket"))
		.AsNonBlocking()
		.AsReusable()
		.WithBroadcast()
		.WithReceiveBufferSize(maxSize); // 2MB

	FIPv4Address Ip;
	FIPv4Address::Parse(IpAddress, Ip);

	TSharedRef<FInternetAddr> InternetAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
	InternetAddr->SetIp(Ip.Value);
	
	int16 Port = serverPort;
	if (!bServer)
		Port = clientPort;

	InternetAddr->SetPort(Port);

	{
		mySession = MakeShared<PacketSession>(Socket, bServer);
		mySession->Run();

		AServerP1GameMode* gameMode = Cast<AServerP1GameMode>(GetWorld()->GetAuthGameMode());
		gameMode->SpawnCubePawn();
		gameMode->SpawnCubes();
	}

}

void UMyGameInstance::SendPacket(SendBufferRef buffer)
{
	if (Socket == nullptr)
		return;

	
}

void UMyGameInstance::HandleRecvPacket()
{

}

void UMyGameInstance::HandleLockstep()
{
	AServerP1GameMode* gameMode = Cast<AServerP1GameMode>(GetWorld()->GetAuthGameMode());
	gameMode->HandleLockstep();
}

void UMyGameInstance::HandleSnapshot()
{
	AServerP1GameMode* gameMode = Cast<AServerP1GameMode>(GetWorld()->GetAuthGameMode());
	gameMode->HandleSnapshot();
}

void UMyGameInstance::HandleSync()
{
	AServerP1GameMode* gameMode = Cast<AServerP1GameMode>(GetWorld()->GetAuthGameMode());
	gameMode->HandleSync();
}
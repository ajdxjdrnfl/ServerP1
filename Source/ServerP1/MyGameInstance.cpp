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
	
	// 귀찮으니 에디터 번호로 서버 판단
	// 0 - 서버
	// 1 - 클라
	int32 pieID = GetWorld()->GetPackage()->PIEInstanceID;
	if (pieID == 0)
		bIsServer = true;
	else bIsServer = false;

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

	if (mySession == nullptr)
		return;

	mySession->SendPacket(buffer);
}

void UMyGameInstance::HandleRecvPacket()
{
	__noop;
}

void UMyGameInstance::HandleLockstep(FLockstepPacket* pkt)
{
	AServerP1GameMode* gameMode = Cast<AServerP1GameMode>(GetWorld()->GetAuthGameMode());
	gameMode->HandleLockstep(*pkt);
}

void UMyGameInstance::HandleSnapshot(FSnapshotPacket* pkt)
{
	AServerP1GameMode* gameMode = Cast<AServerP1GameMode>(GetWorld()->GetAuthGameMode());
	gameMode->HandleSnapshot(*pkt);
}

void UMyGameInstance::HandleSync(FSyncPacket* pkt)
{
	AServerP1GameMode* gameMode = Cast<AServerP1GameMode>(GetWorld()->GetAuthGameMode());
	gameMode->HandleSync(*pkt);
}

void UMyGameInstance::HandleAck(FAckPacket* pkt)
{
	AServerP1GameMode* gameMode = Cast<AServerP1GameMode>(GetWorld()->GetAuthGameMode());
	gameMode->HandleAck(*pkt);
}

int32 UMyGameInstance::GetCurrentSeq()
{
	return mySession->nCurSeq++;
}
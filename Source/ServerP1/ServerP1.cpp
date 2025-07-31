// Copyright Epic Games, Inc. All Rights Reserved.

#include "ServerP1.h"
#include "Modules/ModuleManager.h"
#include "Serialization/ArrayWriter.h"
#include "Sockets.h"
#include "Kismet/GameplayStatics.h"
#include "MyGameInstance.h"
#include "SocketSubsystem.h"

IMPLEMENT_PRIMARY_GAME_MODULE( FDefaultGameModuleImpl, ServerP1, "ServerP1" );

SendBuffer::SendBuffer(int32 bufferSize)
{
	_buffer.SetNum(bufferSize);
}

SendBuffer::~SendBuffer()
{

}

void SendBuffer::CopyData(void* data, int32 len)
{
	::memcpy(_buffer.GetData(), data, len);
	_writeSize = len;
}

void SendBuffer::Close(uint32 writeSize)
{
	_writeSize = writeSize;
}

// Session

PacketSession::PacketSession(class FSocket* Socket, bool bServer, TSharedRef<FInternetAddr> RemoteAddr) : Socket(Socket), bServer(bServer), RemoteAddr(RemoteAddr)
{

}

PacketSession::~PacketSession()
{
	Destroy();
}

void PacketSession::Run()
{
	RecvWorkerThread = MakeShared<RecvWorker>(Socket, AsShared(), RemoteAddr);
	SendWorkerThread = MakeShared<SendWorker>(Socket, AsShared(), RemoteAddr);
}

void PacketSession::Recv()
{

}

void PacketSession::PushRecvPacket(TArray<uint8>& RecvPacket)
{
	FPacketHeader* header = reinterpret_cast<FPacketHeader*>(RecvPacket.GetData());
	auto pktId = header->PacketID;
	
}

void PacketSession::HandleRecvPackets()
{
	while (true)
	{
		TArray<uint8> Packet;
		if (RecvPacketQueue.Pop(OUT Packet) == false)
			break;

		PacketSessionRef ThisPtr = AsShared();
		HandlePacket(Packet);
	}
}

void PacketSession::HandlePacket(TArray<uint8>& Packet)
{
	if (bServer)
		HandleServerPacket(Packet);

	else
		HandleClientPacket(Packet);
}

void PacketSession::HandleServerPacket(TArray<uint8>& Packet)
{
	if (!bServer)
		return;

	FPacketHeader* header = reinterpret_cast<FPacketHeader*>(Packet.GetData());
	auto pktId = header->PacketID;
  
	UMyGameInstance *gameInstance = Cast<UMyGameInstance>(UGameplayStatics::GetGameInstance(GWorld));
	
	switch (pktId)
	{
	case EPacketType::Ack:
	{
		FAckPacket* pkt = reinterpret_cast<FAckPacket*>(&header[1]);
		gameInstance->HandleAck(pkt);
	}
	}
  
}

void PacketSession::HandleClientPacket(TArray<uint8>& Packet)
{
	if (bServer)
		return;
  
	FPacketHeader* header = reinterpret_cast<FPacketHeader*>(Packet.GetData());
	auto pktId = header->PacketID;
  
	UMyGameInstance *gameInstance = Cast<UMyGameInstance>(UGameplayStatics::GetGameInstance(GWorld));
  
	
	switch(pktId)
  {
    case EPacketType::Lockstep:
    {
		FLockstepPacket* pkt = reinterpret_cast<FLockstepPacket*>(&header[1]);
		gameInstance->HandleLockstep(pkt);
    }
      break;

    case EPacketType::Snapshot:
    {
		FSnapshotPacket* pkt = reinterpret_cast<FSnapshotPacket*>(&header[1]);
		gameInstance->HandleSnapshot(pkt);
    }
      break;

    case EPacketType::Sync:
    {
		FSyncPacket* pkt = reinterpret_cast<FSyncPacket*>(&header[1]);
		gameInstance->HandleSync(pkt);
    }
      break;
      
    case EPacketType::Ack:
    {
		FAckPacket* pkt = reinterpret_cast<FAckPacket*>(&header[1]);
		gameInstance->HandleAck(pkt);
    }
      break;
  }
	
}

void PacketSession::SendPacket(SendBufferRef SendBuffer)
{
	// 우선은 모아서 보내는거로,,
	// UDP니까 바로 보내는거로 나중에 수정
	SendPacketQueue.Enqueue(SendBuffer);
}

void PacketSession::Destroy()
{
	if (RecvWorkerThread)
	{
		RecvWorkerThread->Destroy();
		RecvWorkerThread = nullptr;
	}

	if (SendWorkerThread)
	{
		SendWorkerThread->Destroy();
		SendWorkerThread = nullptr;
	}
}

// Worker

RecvWorker::RecvWorker(FSocket* Socket, TSharedPtr<class PacketSession> Session, TSharedRef<FInternetAddr> RemoteAddr) : Socket(Socket), SessionRef(Session), RemoteAddr(RemoteAddr), SenderAddr(ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr())
{
	Thread = FRunnableThread::Create(this, TEXT("RecvWorkerThread"));
}

RecvWorker::~RecvWorker()
{

}

bool RecvWorker::Init()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Recv Worker Init")));
	return true;
}

uint32 RecvWorker::Run()
{
	while (Running)
	{
		TArray<uint8> Packet;

		if (ReceivePacket(OUT Packet))
		{
			if (TSharedPtr<PacketSession> Session = SessionRef.Pin())
			{
			FPacketHeader* header = reinterpret_cast<FPacketHeader*>(Packet.GetData());
			IPacket* packet = reinterpret_cast<IPacket*>(&header[1]);
        
				Session->RecvPacketQueue.Push(packet->nSeq, Packet);
			}
		}
	}

	return 0;
}

void RecvWorker::Exit()
{

}

void RecvWorker::Destroy()
{
	Running = false;
}

bool RecvWorker::ReceivePacket(TArray<uint8>& OutPacket)
{
	const int32 HeaderSize = sizeof(FPacketHeader);
	TArray<uint8> HeaderBuffer;
	HeaderBuffer.AddZeroed(HeaderSize);

	if (ReceiveDesiredBytes(HeaderBuffer.GetData(), HeaderSize) == false)
		return false;

	FPacketHeader Header;
	{
		FMemoryReader Reader(HeaderBuffer);
		Reader << Header;
		UE_LOG(LogTemp, Log, TEXT("Recv PacketID : %d, PacketSize : %d"), Header.PacketID, Header.PacketSize);
	}

	OutPacket = HeaderBuffer;

	TArray<uint8> PayloadBuffer;
	const int32 PayloadSize = Header.PacketSize - HeaderSize;

	if (PayloadSize == 0)
		return true;

	OutPacket.AddZeroed(PayloadSize);

	if (ReceiveDesiredBytes(&OutPacket[HeaderSize], PayloadSize))
		return true;

	return false;
}

bool RecvWorker::ReceiveDesiredBytes(uint8* Results, int32 Size)
{
	uint32 PendingDataSize;
	if (Socket->HasPendingData(PendingDataSize) == false || PendingDataSize <= 0)
		return false;

	int32 offset = 0;

	while (Size > 0)
	{
		int32 NumRead = 0;
		Socket->RecvFrom(Results + offset, Size, OUT NumRead, *SenderAddr);
		check(NumRead <= Size);

		if (NumRead <= 0)
			return false;

		offset += NumRead;
		Size -= NumRead;
	}

	return true;
}


SendWorker::SendWorker(FSocket* Socket, TSharedPtr<class PacketSession> Session, TSharedRef<FInternetAddr> RemoteAddr) : Socket(Socket), SessionRef(Session), RemoteAddr(RemoteAddr)
{
	Thread = FRunnableThread::Create(this, TEXT("SendWorkerThread"));
}

SendWorker::~SendWorker()
{

}


bool SendWorker::Init()
{
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Send Worker Init")));
	return true;
}

uint32 SendWorker::Run()
{
	while (Running)
	{
		SendBufferRef SendBuffer;

		if (TSharedPtr<PacketSession> Session = SessionRef.Pin())
		{
			if (Session->SendPacketQueue.Dequeue(OUT SendBuffer))
			{
				SendPacket(SendBuffer);
			}
		}
	}

	return 0;
}

void SendWorker::Exit()
{

}

bool SendWorker::SendPacket(SendBufferRef SendBuffer)
{
	if (SendDesiredBytes(SendBuffer->Buffer(), SendBuffer->WriteSize()) == false)
	{
		return false;
	}
	return true;
}

void SendWorker::Destroy()
{
	Running = false;
}

bool SendWorker::SendDesiredBytes(const uint8* Buffer, int32 Size)
{
	while (Size > 0)
	{
		int32 BytesSent = 0;
		if (Socket->SendTo(Buffer, Size, BytesSent, RemoteAddr.Get()) == false)
			return false;

		Size -= BytesSent;
		Buffer += BytesSent;
	}

	return true;

}

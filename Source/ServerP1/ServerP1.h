// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Network/Packet.h"

#define USING_SHARED_PTR(name) using name##Ref = TSharedPtr<class name>;

USING_SHARED_PTR(SendBuffer);
USING_SHARED_PTR(Session);
USING_SHARED_PTR(PacketSession);

#define SEND_PACKET(Pkt)														\
	Cast<UMyGameInstance>(GWorld->GetGameInstance())->SendPacket(Pkt);	

#define GET_SESSION()															\
	Cast<UMyGameInstance>(GWorld->GetGameInstance())->mySession;

#define GET_GAMEINSTANCE()															\
	Cast<UMyGameInstance>(GWorld->GetGameInstance());

#define BYTE uint8

#include "Templates/SharedPointer.h"

class FSocket;

class SendBuffer : public TSharedFromThis<SendBuffer>	// 바로바로 보냄
{
public:
	SendBuffer(int32 bufferSize);
	~SendBuffer();

	BYTE* Buffer() { return _buffer.GetData(); }
	int32 WriteSize() { return _writeSize; }
	int32 Capacity() { return static_cast<int32>(_buffer.Num()); }

	void CopyData(void* data, int32 len);
	void Close(uint32 writeSize);

private:
	TArray<BYTE> _buffer;
	int32 _writeSize = 0;
};

// Seq 정보를 담은 벡터
template <typename T>
class TSeqVector
{
	struct TSeqItem
	{
		TSeqItem()
		{

		};

		TSeqItem(T& Item, int nSeq) : Item(Item), nSeq(nSeq)
		{
      bValid = true;
		};

		T Item;
		int32 nSeq = 0;
		bool bValid = false;
	};

public:
  
  TSeqVector()
	{
    bufferSize = 1024;
		left = 0;
		right = 0;
		topSeq = -1;
		v.SetNum(bufferSize);
    v[0].nSeq = 0;
	}

public:
	void Push(int32 nSeq, T& item)
	{
    int32 index = nSeq % bufferSize;
    
    if (nSeq < v[left].nSeq + bufferSize)
    {
      v[index] = TSeqItem{ item, nSeq };
    }
		
    if (nSeq > topSeq && nSeq < v[left].nSeq + bufferSize)
    {
      right = (index + 1) % bufferSize;
      topSeq = nSeq;
    }
	}

	bool Pop(T& item)
	{
		if (IsEmpty())
			return false;

    if (v[left].bValid == false)
      return false;
    
    v[left].bValid = false;
		item = v[left].Item;
    left = (left + 1) % bufferSize;

		return true;
	}
  
	void Clear()
	{
		v.Empty();
	}

	bool IsEmpty()
	{
		return left == right;
	}

private:
	TArray<TSeqItem> v;
	int32 bufferSize;
	int32 left;
	int32 right;
  int32 topSeq;
};

class SERVERP1_API RecvWorker : public FRunnable
{
public:
	RecvWorker(FSocket* Socket, TSharedPtr<class PacketSession> Session, TSharedRef<FInternetAddr> RemoteAddr);
	~RecvWorker();

	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Exit() override;

	void Destroy();

private:
	bool ReceivePacket(TArray<uint8>& OutPacket);
	bool ReceiveDesiredBytes(uint8* Results, int32 Size);

protected:
	FRunnableThread* Thread = nullptr;
	bool Running = true;
	FSocket* Socket;
	TWeakPtr<class PacketSession> SessionRef;
	TSharedRef<FInternetAddr> RemoteAddr;
	TSharedRef<FInternetAddr> SenderAddr;
};

class SERVERP1_API SendWorker : public FRunnable
{
public:
	SendWorker(FSocket* Socket, TSharedPtr<class PacketSession> Session, TSharedRef<FInternetAddr> RemoteAddr);
	~SendWorker();

	virtual bool Init() override;
	virtual uint32 Run() override;
	virtual void Exit() override;

	bool SendPacket(SendBufferRef SendBuffer);
	void Destroy();

private:
	bool SendDesiredBytes(const uint8* Buffer, int32 Size);

protected:
	FRunnableThread* Thread = nullptr;
	bool Running = true;
	FSocket* Socket;
	TWeakPtr<class PacketSession> SessionRef;
	TSharedRef<FInternetAddr> RemoteAddr;
};

class SERVERP1_API PacketSession : public TSharedFromThis<PacketSession>
{
public:
	PacketSession(FSocket* Socket, bool bServer, TSharedRef<FInternetAddr> RemoteAddr);
	~PacketSession();

	void Run();
	void Recv();

	void PushRecvPacket(TArray<uint8>& RecvPacket);

	void HandleRecvPackets();	// Delayed Out Buffer

	void HandlePacket(TArray<uint8>& Packet);
	void HandleServerPacket(TArray<uint8>& Packet);
	void HandleClientPacket(TArray<uint8>& Packet);

	void SendPacket(SendBufferRef SendBuffer);
	void Destroy();

	template<typename T>
	static SendBufferRef MakeSendBuffer(T& pkt, uint16 pktId, int32 nSeq)
	{
		// const uint16 dataSize = static_cast<uint16>(pkt.ByteSize());
		const uint16 dataSize = static_cast<uint16>(sizeof(pkt));
		const uint16 packetSize = dataSize + sizeof(FPacketHeader);

		SendBufferRef sendBuffer = MakeShared<SendBuffer>(packetSize);

		FPacketHeader* header = reinterpret_cast<FPacketHeader*>(sendBuffer->Buffer());
		header->PacketSize = packetSize;
		header->PacketID = pktId;

		switch (pktId)
		{
		case EPacketType::Ack:
		case EPacketType::Lockstep:
		case EPacketType::Snapshot:
		case EPacketType::Sync:
			pkt.nSeq = nSeq;
			break;
		}
		
		pkt.Serialize((uint8*)&header[1]); // 원래는 datasize를 넣어서 오버플로우 방지
		sendBuffer->Close(packetSize);

		return sendBuffer;
	}

public:
	FSocket* Socket;

	TSharedPtr<class RecvWorker> RecvWorkerThread;
	TSharedPtr<class SendWorker> SendWorkerThread;

	//TQueue<TArray<uint8>> RecvPacketQueue;
	TSeqVector<TArray<uint8>> RecvPacketQueue;
	TQueue<SendBufferRef> SendPacketQueue;

	bool bServer;
	int32 nCurSeq = 0;
  TSharedRef<FInternetAddr> RemoteAddr;
};

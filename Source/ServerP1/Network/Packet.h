#pragma once
enum EMySocketType
{
	Server,
	Client,
};
enum EPacketType
{
	Lockstep,
	Snapshot,
	Sync,
	Ack,
};

struct FPacketHeader
{
	FPacketHeader() : PacketSize(0), PacketID(0)
	{

	}

	FPacketHeader(uint16 PacketSize, uint16 PacketID) : PacketSize(PacketSize), PacketID(PacketID)
	{

	}

	friend FArchive& operator<<(FArchive& Ar, FPacketHeader& Header) {
		Ar << Header.PacketSize;
		Ar << Header.PacketID;
		return Ar;
	}

	uint16 PacketSize;
	uint16 PacketID;
};

struct  FMyInput
{
	bool left;
	bool right;
	bool up;
	bool down;
	bool jump;
};


struct  FMyState
{
	FVector position;
	FQuat rotation;
	FVector velocity;
	FVector angularVelocity;
};

struct IPacket
{
	uint16 nSeq;

	virtual void Serialize(uint8* ptr) { }
	virtual void Deserialize(uint8* ptr) { }
	virtual uint16 ByteSize() { return 0; }
};

struct FLockstepPacket : public IPacket
{
	FMyInput input;

	virtual void Serialize(uint8* ptr)
	{
		FLockstepPacket* packet = reinterpret_cast<FLockstepPacket*>(ptr);
		packet->nSeq = nSeq;
		packet->input = input;
	}

	virtual void Deserialize(uint8* ptr)
	{
		FLockstepPacket* packet = reinterpret_cast<FLockstepPacket*>(ptr);
		nSeq = packet->nSeq;
		input = packet->input;
	}

	virtual uint16 ByteSize()
	{
		return sizeof(FLockstepPacket);
	}
};

struct FSyncPacket : public IPacket
{
	uint16 nInputSize;
	TArray<FMyInput> inputs;
	uint16 nStateSize;
	TArray<FMyState> states;

	virtual void Serialize(uint8* ptr)
	{
		uint8* cursor = ptr;

		// Sequence number
		*reinterpret_cast<uint16*>(cursor) = nSeq;
		cursor += sizeof(uint16);

		// Input count
		*reinterpret_cast<uint16*>(cursor) = nInputSize;
		cursor += sizeof(uint16);

		// Inputs
		for (const FMyInput& input : inputs)
		{
			FMemory::Memcpy(cursor, &input, sizeof(FMyInput));
			cursor += sizeof(FMyInput);
		}

		// State count
		*reinterpret_cast<uint16*>(cursor) = nStateSize;
		cursor += sizeof(uint16);

		// States
		for (const FMyState& state : states)
		{
			FMemory::Memcpy(cursor, &state, sizeof(FMyState));
			cursor += sizeof(FMyState);
		}
	}

	// Deserialize a packet from a binary buffer
	virtual void Deserialize(uint8* ptr)
	{
		const uint8* cursor = ptr;

		// Sequence number
		nSeq = *reinterpret_cast<const uint16*>(cursor);
		cursor += sizeof(uint16);

		// Input count
		nInputSize = *reinterpret_cast<const uint16*>(cursor);
		cursor += sizeof(uint16);

		// Inputs
		inputs.SetNum(nInputSize);
		for (int i = 0; i < nInputSize; ++i)
		{
			FMemory::Memcpy(&inputs[i], cursor, sizeof(FMyInput));
			cursor += sizeof(FMyInput);
		}

		// State count
		nStateSize = *reinterpret_cast<const uint16*>(cursor);
		cursor += sizeof(uint16);

		// States
		states.SetNum(nStateSize);
		for (int i = 0; i < nStateSize; ++i)
		{
			FMemory::Memcpy(&states[i], cursor, sizeof(FMyState));
			cursor += sizeof(FMyState);
		}
	}


	virtual uint16 ByteSize()
	{
		return sizeof(FMyInput) + sizeof(nInputSize) +
			sizeof(FMyInput) * nInputSize +
			sizeof(nStateSize) + sizeof(FMyState) * nStateSize
			+ sizeof(nSeq);
	}
};

struct FSnapshotPacket : public IPacket
{
	uint16 nStateSize;
	TArray<FMyState> states;

	virtual void Serialize(uint8* ptr)
	{
		uint8* cursor = ptr;

		// Serialize sequence number
		*reinterpret_cast<uint16*>(cursor) = nSeq;
		cursor += sizeof(uint16);

		// Serialize number of states
		*reinterpret_cast<uint16*>(cursor) = nStateSize;
		cursor += sizeof(uint16);

		// Serialize each state
		for (const FMyState& state : states)
		{
			FMemory::Memcpy(cursor, &state, sizeof(FMyState));
			cursor += sizeof(FMyState);
		}
	}

	virtual void Deserialize(uint8* ptr)
	{
		const uint8* cursor = ptr;

		// Read sequence number
		nSeq = *reinterpret_cast<const uint16*>(cursor);
		cursor += sizeof(uint16);

		// Read number of states
		nStateSize = *reinterpret_cast<const uint16*>(cursor);
		cursor += sizeof(uint16);

		// Read states
		states.SetNum(nStateSize);
		for (int i = 0; i < nStateSize; ++i)
		{
			FMemory::Memcpy(&states[i], cursor, sizeof(FMyState));
			cursor += sizeof(FMyState);
		}
	}


	virtual uint16 ByteSize()
	{
		return sizeof(FMyInput) +
			sizeof(nStateSize) + sizeof(FMyState) * nStateSize
			+ sizeof(nSeq);
	}
};

struct FAckPacket : public IPacket
{
	uint16 nAck;
	// nSeq¶û nAck´Â ¶È°°ÀÌ

	virtual void Serialize(uint8* ptr)
	{
		FAckPacket* packet = reinterpret_cast<FAckPacket*>(ptr);
		packet->nSeq = nSeq;
		packet->nAck = nAck;
	}

	virtual void Deserialize(uint8* ptr)
	{
	}

	virtual uint16 ByteSize()
	{
		return sizeof(nAck)
		+sizeof(nSeq);
	}
};

struct FSessionState
{
	int64 ack;
	int64 seq;
};
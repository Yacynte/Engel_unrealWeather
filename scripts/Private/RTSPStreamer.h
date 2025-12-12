#pragma once

#include "CoreMinimal.h"
#include <stdio.h>
#include "Misc/Paths.h"
#include "ImageUtils.h"
#include "HAL/PlatformProcess.h"
#include "Networking.h"
#include "Sockets.h"
#include "SocketSubsystem.h"



class FRTSPStreamer
{
public:
	FRTSPStreamer();
	~FRTSPStreamer();

	// Start the FFmpeg process
	bool StartStream(int32 InWidth, int32 InHeight, int32 InFPS, FString RTSPURL);

	// Push raw FColor data to FFmpeg
	void SendFrame( const TArray<FColor>& Bitmap);

	// Kill the process
	void StopStream();

	bool IsStreaming() const { return bIsStreaming; }

	bool ConnectToFFmpeg();

	void SendFrameTCP(const TArray<FColor>& Bitmap);
	
	bool StartTCPServer(int32 InWidth, int32 InHeight);
	FSocket* ListenSocket = nullptr;
	FSocket* ClientSocket = nullptr;

	// New members for the metadata channel
	FSocket* MetadataListenSocket; // Listens on port 9001
	FSocket* MetadataClientSocket; // The active connection

	// ... existing variables ...
	float RollValue;
	float PitchValue;

	// New function to start the metadata listener
	bool StartMetadataServer();

	// New function to read metadata
	void ReceiveMetadata();
	

private:
	int32 Width;
	int32 Height;
	bool bIsStreaming ;
	// Member variables needed for the class:
	 FProcHandle FFmpegProcessHandle;
	 void* PipeWriteChild;
	 void* PipeReadChild;
	 FSocket* Socket = nullptr;
	 FString ip_address = TEXT("10.116.88.38");
	 bool IsConnected = false;
	 int32 rstpPort = 9000;
	 int32 dataPort = 9001;
};


class FAcceptTask : public FNonAbandonableTask
{
public:
	FRTSPStreamer* Streamer;

	FAcceptTask(FRTSPStreamer* InStreamer) : Streamer(InStreamer) {}

	void DoWork()
	{
		bool Pending;
		while (true)
		{
			if (Streamer->ListenSocket->HasPendingConnection(Pending) && Pending)
			{
				Streamer->ClientSocket = Streamer->ListenSocket->Accept(TEXT("FFmpegClient"));
				if (Streamer->ClientSocket)
				{
					UE_LOG(LogTemp, Log, TEXT("FFmpeg connected!"));
				}
				return;
			}
			FPlatformProcess::Sleep(0.01f);
		}
	}

	FORCEINLINE TStatId GetStatId() const { RETURN_QUICK_DECLARE_CYCLE_STAT(FAcceptTask, STATGROUP_ThreadPoolAsyncTasks); }
};


class FMetadataAcceptTask : public FNonAbandonableTask
{
public:
	FRTSPStreamer* Streamer;

	FMetadataAcceptTask(FRTSPStreamer* InStreamer) : Streamer(InStreamer) {}

	void DoWork()
	{
		bool Pending;
		while (true)
		{
			if (Streamer->ListenSocket->HasPendingConnection(Pending) && Pending)
			{
				Streamer->ClientSocket = Streamer->ListenSocket->Accept(TEXT("FFmpegClient"));
				if (Streamer->ClientSocket)
				{
					UE_LOG(LogTemp, Log, TEXT("FFmpeg connected!"));
				}
				return;
			}
			FPlatformProcess::Sleep(0.01f);
		}
	}

	FORCEINLINE TStatId GetStatId() const { RETURN_QUICK_DECLARE_CYCLE_STAT(FAcceptTask, STATGROUP_ThreadPoolAsyncTasks); }
};
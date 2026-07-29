#pragma once

#include "CoreMinimal.h"
#include <stdio.h>
#include "Misc/Paths.h"
#include "ImageUtils.h"
#include "HAL/PlatformProcess.h"
#include "Networking.h"
#include "Sockets.h"
#include "SocketSubsystem.h"
#include "Json.h"


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
	void SendFrameUDP(const TArray<FColor>& Bitmap);
	
	bool StartTCPServer(int32 InWidth, int32 InHeight);
	void StartUDPServer();
	void Shutdown();
	FSocket* ListenSocket = nullptr;
	FSocket* ClientSocket = nullptr;

	//FSocket* ControlListenSocket = nullptr;
	//FSocket* ControlClientSocket = nullptr;

	// New members for the metadata channel
	FSocket* MetadataListenSocket = nullptr; // Listens on port 9001
	FSocket* MetadataClientSocket = nullptr; // The active connection

	// ... existing variables ...
	float YawRate_cam;
	float PitchRate_cam;
	float YawRate_drone;
	FVector velocity;
	FRotator angularRate;
	bool arrivedTarget = false;
	bool gimCam;
	bool recImg_resize = false;
	bool toggleSnow = false;
	bool toggleRain = false;
	bool startSnow = false;
	bool startRain = false;
	bool receivedData = false;
	bool toggelStream = false;
	FVector targetLocation;
	FRotator targetRotation;
	FRotator relTargetRotator;
	bool useTargetRotation = false;
	bool setlocation;

	// New function to start the metadata listener
	void StartMetadataServer();

	void StopClientSocket();

	// New function to read metadata
	void ReceiveMetadata();
	void CheckSocketHealth();

	// Boolean exposed to Blueprint
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	bool bIsDetected;

	bool isReceiving = false;
	bool bShouldRun = true;

	// 2D Vector exposed to Blueprint
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Data")
	FVector2D ScreenPosition;
	

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
	 //FString ip_data = TEXT("10.116.88.38");
	 //FString ip_add_control = TEXT("10.116.88.38");
	 //FString ip_add_data = TEXT("192.168.0.2");
	 FString ip_data = TEXT("0.0.0.0");
	 bool IsConnected = false;
	 int32 rstpPort = 9000;
	 int32 dataPort = 9001;
	 void ProcessJsonCommand(const TSharedPtr<FJsonObject>& JsonObject);
	 //int32 controlPort = 9002;
	 //FAcceptTask* acceptTask;
};


class FAcceptTask : public FNonAbandonableTask
{
public:
	FRTSPStreamer* Streamer;

	FAcceptTask(FRTSPStreamer* InStreamer) : Streamer(InStreamer) {}

	//bool pending = true;
	void DoWork()
	{
		//bool Pending;
		while (Streamer->bShouldRun) // some controlled flag for shutdown
		{
			if (!Streamer->ClientSocket) // not connected, wait for one
			{
				bool Pending = false;
				if (Streamer->ListenSocket->HasPendingConnection(Pending) && Pending)
				{
					Streamer->ClientSocket = Streamer->ListenSocket->Accept(TEXT("FFmpegClient"));
					if (Streamer->ClientSocket)
					{
						UE_LOG(LogTemp, Log, TEXT("FFmpeg / TCP connected!"));
					}
				}
				FPlatformProcess::Sleep(0.01f);
			}
		}
		/*while (true)
		{
			if (Streamer->ListenSocket->HasPendingConnection(Pending) && Pending)
			{
				Streamer->ClientSocket = Streamer->ListenSocket->Accept(TEXT("FFmpegClient"));
				if (Streamer->ClientSocket)
				{
					UE_LOG(LogTemp, Log, TEXT("FFmpeg / TCP connected!"));
				}
				return;
			}
			FPlatformProcess::Sleep(0.01f);
		}*/
	}

	FORCEINLINE TStatId GetStatId() const { RETURN_QUICK_DECLARE_CYCLE_STAT(FAcceptTask, STATGROUP_ThreadPoolAsyncTasks); }
};


//class FMetadataAcceptTask : public FNonAbandonableTask
//{
//public:
//	FRTSPStreamer* Streamer;
//
//	FMetadataAcceptTask(FRTSPStreamer* InStreamer) : Streamer(InStreamer) {}
//
//	void DoWork()
//	{
//		bool PendingData;
//		while (true)
//		{
//			if (Streamer->MetadataListenSocket->HasPendingConnection(PendingData) && PendingData)
//			{
//				Streamer->MetadataClientSocket = Streamer->MetadataListenSocket->Accept(TEXT("FFmpegClient"));
//				if (Streamer->MetadataClientSocket)
//				{
//					UE_LOG(LogTemp, Log, TEXT("FFmpeg connected!"));
//				}
//				return;
//			}
//			FPlatformProcess::Sleep(0.01f);
//		}
//	}
//
//	FORCEINLINE TStatId GetStatId() const { RETURN_QUICK_DECLARE_CYCLE_STAT(FMetadataAcceptTask, STATGROUP_ThreadPoolAsyncTasks); }
//};


//class FControldataAcceptTask : public FNonAbandonableTask
//{
//public:
//	FRTSPStreamer* Streamer;
//
//	FControldataAcceptTask(FRTSPStreamer* InStreamer) : Streamer(InStreamer) {}
//
//	void DoWork()
//	{
//		bool PendingControlData;
//		while (true)
//		{
//			if (Streamer->ControlListenSocket->HasPendingConnection(PendingControlData) && PendingControlData)
//			{
//				Streamer->ControlClientSocket = Streamer->ControlListenSocket->Accept(TEXT("Control Client"));
//				if (Streamer->ControlClientSocket)
//				{
//					UE_LOG(LogTemp, Log, TEXT("Control Client connected!"));
//				}
//				return;
//			}
//			FPlatformProcess::Sleep(0.01f);
//		}
//	}
//
//	FORCEINLINE TStatId GetStatId() const { RETURN_QUICK_DECLARE_CYCLE_STAT(FControldataAcceptTask, STATGROUP_ThreadPoolAsyncTasks); }
//};
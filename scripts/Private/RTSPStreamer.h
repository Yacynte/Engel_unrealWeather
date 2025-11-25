#pragma once

#include "CoreMinimal.h"
#include <stdio.h>
#include "Misc/Paths.h"
#include "HAL/PlatformProcess.h"


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

private:
	int32 Width;
	int32 Height;
	bool bIsStreaming;
	// Member variables needed for the class:
	 FProcHandle FFmpegProcessHandle;
	 void* PipeWriteChild;
	 void* PipeWriteParent;
};
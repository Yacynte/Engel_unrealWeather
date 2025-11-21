#pragma once

#include "CoreMinimal.h"
#include <stdio.h>

class FRTSPStreamer
{
public:
	FRTSPStreamer();
	~FRTSPStreamer();

	// Start the FFmpeg process
	bool StartStream(int32 InWidth, int32 InHeight, int32 InFPS, FString RTSPURL);

	// Push raw FColor data to FFmpeg
	void SendFrame(const TArray<FColor>& Bitmap);

	// Kill the process
	void StopStream();

	bool IsStreaming() const { return bIsStreaming; }

private:
	FILE* PipeHandle;
	int32 Width;
	int32 Height;
	bool bIsStreaming;
};
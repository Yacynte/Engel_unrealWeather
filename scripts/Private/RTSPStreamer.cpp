#include "RTSPStreamer.h"

FRTSPStreamer::FRTSPStreamer()
	: PipeHandle(nullptr), Width(0), Height(0), bIsStreaming(false)
{
}

FRTSPStreamer::~FRTSPStreamer()
{
	StopStream();
}

bool FRTSPStreamer::StartStream(int32 InWidth, int32 InHeight, int32 InFPS, FString RTSPURL)
{
	if (bIsStreaming) return true;

	Width = InWidth;
	Height = InHeight;

	// --- THE FFMPEG COMMAND ---
	// -f rawvideo: We are sending raw bytes, not a file
	// -pixel_format bgra: Unreal FColor uses BGRA ordering
	// -video_size: Resolution
	// -i - : Input comes from the Pipe (Standard Input)
	// -c:v libx264: Encode using H.264
	// -preset ultrafast: Sacrifice compression ratio for Speed (Crucial for ReadPixels)
	// -tune zerolatency: Do not buffer frames
	// -f rtsp: Output format
	FString Command = FString::Printf(
		TEXT("ffmpeg -y -f rawvideo -pixel_format bgra -video_size %dx%d -framerate %d -i - -c:v libx264 -preset ultrafast -tune zerolatency -f rtsp %s"),
		Width, Height, InFPS, *RTSPURL
	);

	// Open Pipe (Windows specific)
#if PLATFORM_WINDOWS
	PipeHandle = _popen(TCHAR_TO_ANSI(*Command), "wb");
#else
	PipeHandle = popen(TCHAR_TO_ANSI(*Command), "wb");
#endif

	if (!PipeHandle)
	{
		UE_LOG(LogTemp, Error, TEXT("RTSP: Failed to launch FFmpeg! Is ffmpeg in your PATH?"));
		return false;
	}

	bIsStreaming = true;
	UE_LOG(LogTemp, Log, TEXT("RTSP: Streaming started to %s"), *RTSPURL);
	return true;
}

void FRTSPStreamer::SendFrame(const TArray<FColor>& Bitmap)
{
	if (!bIsStreaming || !PipeHandle) return;

	// Sanity check data size
	if (Bitmap.Num() != Width * Height) return;

	// Write raw memory to pipe
	fwrite(Bitmap.GetData(), sizeof(FColor), Bitmap.Num(), PipeHandle);
}

void FRTSPStreamer::StopStream()
{
	if (PipeHandle)
	{
#if PLATFORM_WINDOWS
		_pclose(PipeHandle);
#else
		pclose(PipeHandle);
#endif
		PipeHandle = nullptr;
	}
	bIsStreaming = false;
}
#include "RTSPStreamer.h"

FRTSPStreamer::FRTSPStreamer()
	: FFmpegProcessHandle(nullptr), Width(0), Height(0), bIsStreaming(false)
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
	// MediaMTX requires proper PUSH mode.
	// This ffmpeg command uses the correct RTSP publish dialect:
	// -re : pace frames; without this MediaMTX often rejects publish
	// -rtsp_transport tcp : stable
	// -g 12 : keyframe interval (mandatory or VLC won't open the stream)
	// -tune zerolatency : no buffering
	//
	// Dynamically construct the path relative to the project root
	//FString FFmpegPath = TEXT("C:/Users/Batchaya/AppData/Local/ffmpeg/ffmpeg-8.0.1-full_build/bin/ffmpeg.exe");

	FString FFmpegPath = TEXT("C:/Users/Batchaya/AppData/Local/ffmpeg/ffmpeg-8.0.1-full_build/bin/ffmpeg.exe");

	// Arguments: everything *after* the executable path, and without quotes.
	FString Args = FString::Printf(
		TEXT("-re -f lavfi -i testsrc=size=%dx%d:rate=%d -c:v libx264 -preset ultrafast -tune zerolatency -rtsp_transport tcp -f rtsp %s"),
		Width,
		Height,
		InFPS,
		*RTSPURL
	);

	//  Create the Pipe for Communication (Crucial step)
	// This creates two ends: one for the parent (Unreal) to write, one for the child (FFmpeg) to read.
	FPlatformProcess::CreatePipe(PipeWriteChild, PipeWriteParent);

	// 4. Launch the Process
	FFmpegProcessHandle = FPlatformProcess::CreateProc(
		*FFmpegPath,        // Executable Path
		*Args,              // Arguments
		true,               // bLaunchDetached (Runs in background)
		true,               // bLaunchHidden (No console window)
		true,               // bLaunchReallyHidden
		nullptr,            // Process ID
		0,                  // Priority
		nullptr,            // Working Directory
		PipeWriteChild      // Pipe for child process (FFmpeg) to read from
	);

	if (!FFmpegProcessHandle.IsValid())
	{
		// Store this handle if you need to terminate the stream later!
		/*bIsStreaming = true;
		UE_LOG(LogTemp, Log, TEXT("RTSP: FFmpeg launched asynchronously."));*/
		FPlatformProcess::ClosePipe(PipeWriteChild, PipeWriteParent);
		UE_LOG(LogTemp, Error, TEXT("RTSP: Failed to launch FFmpeg process."));
		return false;
	}

	// Launch succeeded, but the parent(Unreal) doesn't need to write to the child's end.
	// Close the child handle in the parent process's memory space immediately.
	FPlatformProcess::ClosePipe(PipeWriteChild, nullptr);

	bIsStreaming = true;
	// Close the child end of the pipe as the parent process won't use it.
	//FPlatformProcess::ClosePipe(PipeWriteChild, PipeWriteParent);
	UE_LOG(LogTemp, Log, TEXT("RTSP: Streaming started to %s"), *RTSPURL);
	//UE_LOG(LogTemp, Log, TEXT("RTSP: Streaming command was %s"), *Command);
	return true;
}

void FRTSPStreamer::SendFrame(TArray<FColor> Bitmap)
{
	// Check if the stream is active and the parent pipe handle is valid
	if (!bIsStreaming || !PipeWriteParent) return;

	// Sanity check data size
	if (Bitmap.Num() != Width * Height) return;

	// 1. Calculate total size in bytes
	const int32 DataSize = Bitmap.Num() * sizeof(FColor);

	// 2. Write raw memory to the pipe
	// Note: FPlatformProcess::WritePipe handles the flushing automatically.
	FPlatformProcess::WritePipe(PipeWriteParent, (const uint8*)Bitmap.GetData(), DataSize);
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("Add image to stream"));
	// No need for fflush() here; the engine handles buffering and flushing for you.
}

void FRTSPStreamer::StopStream()
{
	if (FFmpegProcessHandle.IsValid())
	{
		// 1. Terminate the FFmpeg process
		FPlatformProcess::TerminateProc(FFmpegProcessHandle);

		// 2. Close the handle
		FPlatformProcess::CloseProc(FFmpegProcessHandle);

		// 3. Close the parent end of the pipe
		FPlatformProcess::ClosePipe(PipeWriteParent, PipeWriteChild);

		FFmpegProcessHandle.Reset();
		bIsStreaming = false;

		UE_LOG(LogTemp, Log, TEXT("RTSP: FFmpeg process terminated successfully."));
	}
}
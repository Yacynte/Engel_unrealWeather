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

    FString FFmpegPath = TEXT("C:/Users/Batchaya/AppData/Local/ffmpeg/ffmpeg-8.0.1-full_build/bin/ffmpeg.exe");

    // IMPORTANT: Input is now rawvideo from STDIN ("-i -")
    FString Args = FString::Printf(
        TEXT("-re -f rawvideo -pix_fmt bgra -s %dx%d -r %d -i tcp://127.0.0.1:9000 -c:v libx264 -preset ultrafast -tune zerolatency -f rtsp -rtsp_transport tcp %s"),
        Width,
        Height,
        InFPS,
        *RTSPURL
    );

    // Create STDIN pipe for FFmpeg
    // FPlatformProcess::CreatePipe(PipeReadChild, PipeWriteChild);

    // Start FFmpeg with child STDIN bound to PipeWriteChild
    FFmpegProcessHandle = FPlatformProcess::CreateProc(
        *FFmpegPath,
        *Args,
        true,   // detached
        true,   // hidden
        true,   // launch hidden window
        nullptr,
        0,
        nullptr,
        nullptr,   // Child STDIN
        nullptr          // Child STDOUT (optional)
    );

    if (!FFmpegProcessHandle.IsValid())
    {
        FPlatformProcess::ClosePipe(PipeWriteChild, PipeReadChild);
        UE_LOG(LogTemp, Error, TEXT("RTSP: Failed to launch FFmpeg."));
        return false;
    }

    // Parent does not use child end
     //FPlatformProcess::ClosePipe(PipeWriteChild, nullptr);

    bIsStreaming = true;
    UE_LOG(LogTemp, Log, TEXT("RTSP: Streaming started to %s"), *RTSPURL);

    return true;
}


void FRTSPStreamer::SendFrame(const TArray<FColor>& Bitmap)
{
    if (FFmpegProcessHandle.IsValid())
    {
        if (!bIsStreaming || !PipeReadChild) return;
        if (Bitmap.Num() != Width * Height) return;

        // BGRA8 is exactly what FColor uses
        const uint8* RawData = reinterpret_cast<const uint8*>(Bitmap.GetData());
        const int32 DataSize = Width * Height * 4; // 4 bytes per pixel (BGRA)

        FPlatformProcess::WritePipe(PipeReadChild, RawData, DataSize);
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("Add image to stream"));
    }
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
		FPlatformProcess::ClosePipe(PipeReadChild, PipeWriteChild);

		FFmpegProcessHandle.Reset();
		bIsStreaming = false;

		UE_LOG(LogTemp, Log, TEXT("RTSP: FFmpeg process terminated successfully."));
	}
}

void FRTSPStreamer::SendFrameTCP(const TArray<FColor>& Bitmap)
{
    if (!ClientSocket) {
        // No client connected yet → just skip sending this frame
        return;
    }
    if (!bIsStreaming) { return; }
    int32 BytesSent = 0;
    int32 TotalSize = Width * Height * 4;

    const uint8* RawBGRA = reinterpret_cast<const uint8*>(Bitmap.GetData());

    // Keep sending until all bytes are pushed
    while (BytesSent < TotalSize)
    {
        int32 SentNow = 0;
        ClientSocket->Send(RawBGRA + BytesSent, TotalSize - BytesSent, SentNow);
        if (SentNow <= 0)
        {
            UE_LOG(LogTemp, Error, TEXT("Socket send failed."));
            if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Socket send failed."));
            break;
        }
        BytesSent += SentNow;
    }
    if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("Add image to stream"));
}

bool FRTSPStreamer::ConnectToFFmpeg()
{
    ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);

    Socket = SocketSubsystem->CreateSocket(NAME_Stream, TEXT("FFmpegStream"), false);

    FIPv4Address IpAddress;
    FIPv4Address::Parse(ip_address, IpAddress);

    TSharedRef<FInternetAddr> Addr = SocketSubsystem->CreateInternetAddr();
    Addr->SetIp(IpAddress.Value);
    Addr->SetPort(9000);

    // 4MB buffer to avoid blocking
    int32 NewSize = 0;
    Socket->SetReceiveBufferSize(4 * 1024 * 1024, NewSize);
    Socket->SetSendBufferSize(4 * 1024 * 1024, NewSize);

    return Socket->Connect(*Addr);
}

bool FRTSPStreamer::StartTCPServer(int32 InWidth, int32 InHeight )
{
    Height = InHeight;
    Width = InWidth;
    if (bIsStreaming) { return true; }
    ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);

    ListenSocket = SocketSubsystem->CreateSocket(NAME_Stream, TEXT("FFmpegStreamSocket"), false);
    if (!ListenSocket) return false;

    int32 Port = 9000;

    FIPv4Address Addr;
    FIPv4Address::Parse(ip_address, Addr);

    TSharedRef<FInternetAddr> InternetAddr = SocketSubsystem->CreateInternetAddr();
    InternetAddr->SetIp(Addr.Value);
    InternetAddr->SetPort(Port);

    // Bind
    if (!ListenSocket->Bind(*InternetAddr))
    {
        UE_LOG(LogTemp, Error, TEXT("Bind failed."));
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Bind failed."));
        return false;
    }

    // Listen
    if (!ListenSocket->Listen(1))
    {
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Listening failed."));
        return false;
    }

    // Accept connection in async thread
    (new FAutoDeleteAsyncTask<FAcceptTask>(this))->StartBackgroundTask();
    bIsStreaming = true;
    return true;
}

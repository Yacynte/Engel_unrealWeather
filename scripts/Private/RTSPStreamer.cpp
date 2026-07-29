#include "RTSPStreamer.h"
#include "MyActor_weather.h"


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
        TEXT("-re -f rawvideo -pix_fmt bgra -s %dx%d -r %d -i tcp://127.0.0.1:9000 -c:v libx264 -preset ultrafast -tune zerolatency -vf \"scale=720:480\" -f rtsp -rtsp_transport tcp %s"),
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
        false,   // detached
        false,   // hidden
        false,   // launch hidden window
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
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("RTSP: Failed to launch FFmpeg."));
        return false;
    }

    // Parent does not use child end
    //FPlatformProcess::ClosePipe(PipeWriteChild, nullptr);

    bIsStreaming = true;
    UE_LOG(LogTemp, Log, TEXT("RTSP: Streaming started to %s"), *RTSPURL);
    if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("ffmpeg started"));
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

void FRTSPStreamer::StopClientSocket() {
    //bIsStreaming = false;
    //toggelStream = false;
    ClientSocket->Close();
    ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    if (SocketSubsystem)
    {
        SocketSubsystem->DestroySocket(ClientSocket);
    }
    ClientSocket = nullptr;
}


void FRTSPStreamer::StopStream()
{
    // 1. Check if streaming is actually running
    if (bIsStreaming)
    {
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("Stopping RTSP Stream..."));
    bIsStreaming = false;  // Set this FIRST to prevent new operations
    // Grab socket subsystem once
    ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);

    // 2. Close the TCP Socket (Input Source)
    if (ClientSocket)
    {
        // Close the socket connection. The implementation depends on your networking library (e.g., FSocket::Close)
        ClientSocket->Close();
        if (SocketSubsystem)
        {
            SocketSubsystem->DestroySocket(ClientSocket);
        }
        ClientSocket = nullptr;
    }
    // 2c. Close the listening socket (server) if it exists
    if (ListenSocket)
    {
        ListenSocket->Close();
        if (SocketSubsystem)
        {
            SocketSubsystem->DestroySocket(ListenSocket);
        }
        ListenSocket = nullptr;
    }

    //// 3. Terminate the External FFmpeg Process
    //if (FFmpegProcessHandle.IsValid())
    //{
    //    // Use the FPlatformProcess to terminate the running FFmpeg executable.
    //    // This is necessary to release the file handle and stop encoding.
    //    FPlatformProcess::TerminateProc(FFmpegProcessHandle);
    //    FPlatformProcess::CloseProc(FFmpegProcessHandle); // Close the handle resource

    //    // Reset the handle pointer
    //    FFmpegProcessHandle.Reset();
    //}

    // 3. Terminate the External FFmpeg Process
    if (FFmpegProcessHandle.IsValid())
    {
        // Ask the process to terminate, then wait briefly for it to exit.
        FPlatformProcess::TerminateProc(FFmpegProcessHandle);
        // small wait to let OS release resources
        int32 WaitCycles = 0;
        while (FPlatformProcess::IsProcRunning(FFmpegProcessHandle) && WaitCycles++ < 50)
        {
            FPlatformProcess::Sleep(0.02f);
        }
        FPlatformProcess::CloseProc(FFmpegProcessHandle); // Close the handle resource

        // Reset the handle pointer
        FFmpegProcessHandle.Reset();
    }

    //ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);

    if (MetadataClientSocket)
    {
        MetadataClientSocket->Close();
        if (SocketSubsystem)
        {
            SocketSubsystem->DestroySocket(MetadataClientSocket);
        }
        MetadataClientSocket = nullptr;
    }

    if (MetadataListenSocket)
    {
        MetadataListenSocket->Close();
        if (SocketSubsystem)
        {
            SocketSubsystem->DestroySocket(MetadataListenSocket);
        }
        MetadataListenSocket = nullptr;
    }

    UE_LOG(LogTemp, Log, TEXT("RTSP Stream stopped successfully."));
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
            //StopClientSocket();
            UE_LOG(LogTemp, Error, TEXT("Send via socket failed."));
            if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Send via socket failed."));
            break;
        }
        BytesSent += SentNow;
    }
    //if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("Add image to stream"));
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
    if (!ListenSocket)
    {
        UE_LOG(LogTemp, Error, TEXT("Socket creation failed."));
        return false;
    }
    // (bypassing the OS's TIME_WAIT state).
    bool bReuseAddrSuccess = ListenSocket->SetReuseAddr(true);
    if (!bReuseAddrSuccess)
    {
        UE_LOG(LogTemp, Warning, TEXT("Failed to set SO_REUSEADDR option. Bind failures may occur."));
    }

    

    FIPv4Address Addr;
    FIPv4Address::Parse(ip_address, Addr);

    TSharedRef<FInternetAddr> InternetAddr = SocketSubsystem->CreateInternetAddr();
    InternetAddr->SetIp(Addr.Value);
    InternetAddr->SetPort(rstpPort);

    // Bind
    if (!ListenSocket->Bind(*InternetAddr))
    {
        UE_LOG(LogTemp, Error, TEXT("Bind failed on port &d."), rstpPort);
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
    if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("TCP server started"));
    return true;
}



void FRTSPStreamer::StartMetadataServer()
{
    ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
    if (MetadataListenSocket)
    {
        if (GEngine) GEngine->AddOnScreenDebugMessage(151, 60.f, FColor::Green, TEXT("StartMetadataServer called but socket already exists — skipping."));
        return;
    }
    // Change: Use SOCKTYPE_Datagram for UDP
    MetadataListenSocket = SocketSubsystem->CreateSocket(NAME_DGram, TEXT("MetadataSocket"), false);
    if (!MetadataListenSocket) return;

    MetadataListenSocket->SetReuseAddr(true);
    MetadataListenSocket->SetNonBlocking(true);

    FIPv4Address Addr;
    FIPv4Address::Parse(ip_data, Addr);
    TSharedRef<FInternetAddr> InternetAddr = SocketSubsystem->CreateInternetAddr();
    InternetAddr->SetIp(Addr.Value);
    InternetAddr->SetPort(dataPort);

    // UDP only needs to Bind to receive
    if (!MetadataListenSocket->Bind(*InternetAddr))
    {
        UE_LOG(LogTemp, Error, TEXT("Metadata Bind failed on port %d."), dataPort);
        if (GEngine)
        {
            FString DebugMsg = FString::Printf(TEXT("Metadata Bind failed on %s : %d."), *ip_data, dataPort);
            GEngine->AddOnScreenDebugMessage(200, 60.f, FColor::Green, DebugMsg);
        }
        SocketSubsystem->DestroySocket(MetadataListenSocket);
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("Metadata UDP receiver started on %s : %d."), *ip_data, dataPort);
    if (GEngine)
    {
        FString DebugMsg = FString::Printf(TEXT("Metadata UDP receiver started on %s : %d."), *ip_data, dataPort);
        GEngine->AddOnScreenDebugMessage(200, 60.f, FColor::Green, DebugMsg);
    }
    return;
}


void FRTSPStreamer::ReceiveMetadata()
{
    if (!MetadataListenSocket) return;
    
    //if (GEngine) GEngine->AddOnScreenDebugMessage(105, 60.f, FColor::Green, TEXT("In ReceiveMetadata"));

    uint32 Size;
    while (MetadataListenSocket->HasPendingData(Size))
    {
        TArray<uint8> ReceivedData;
        ReceivedData.SetNumUninitialized(Size + 1   );
        int32 BytesRead = 0;
        TSharedRef<FInternetAddr> SenderAddr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();

        MetadataListenSocket->RecvFrom(ReceivedData.GetData(), Size, BytesRead, *SenderAddr);
        //if (GEngine) GEngine->AddOnScreenDebugMessage(100, 60.f, FColor::Green, TEXT("In MetadataListenSocket has PendingData "));
        if (BytesRead > 0)
        {
            //if (!isReceiving) isReceiving = true;
            // Convert byte array to FString
            ReceivedData[BytesRead] = 0;
            FString ReceivedString = FString(UTF8_TO_TCHAR(reinterpret_cast<const char*>(ReceivedData.GetData())));

            // Deserialize and route to the Dispatcher
            TSharedPtr<FJsonObject> JsonObject;
            TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(ReceivedString);
            //if (GEngine) GEngine->AddOnScreenDebugMessage(101, 60.f, FColor::Green, TEXT("Read bytes"));
            if (FJsonSerializer::Deserialize(Reader, JsonObject) && JsonObject.IsValid())
            {
                // Parse Json Command
                //if (GEngine) GEngine->AddOnScreenDebugMessage(102, 120.f, FColor::Green, TEXT("Parsed Json"));
                ProcessJsonCommand(JsonObject);                
            }
            else
            {
                UE_LOG(LogTemp, Warning, TEXT("Failed to parse metadata JSON: %s"), *ReceivedString);
            }
        }
    }
}

void FRTSPStreamer::ProcessJsonCommand(const TSharedPtr<FJsonObject>& JsonObject)
{
    FString Command;
    if (!JsonObject->TryGetStringField(TEXT("command"), Command))
    {
        UE_LOG(LogTemp, Warning, TEXT("JSON packet missing 'command' field."));
        return;
    }

    FString Target;
    if (!JsonObject->TryGetStringField(TEXT("target"), Target))
    {
        UE_LOG(LogTemp, Warning, TEXT("JSON packet missing 'target' object."));
        return;
    }

    const TSharedPtr<FJsonObject>* Params;
    if (!JsonObject->TryGetObjectField(TEXT("params"), Params))
    {
        UE_LOG(LogTemp, Warning, TEXT("JSON packet missing 'params' object."));
        return;
    }

    // Direct command routing
    if (Command == TEXT("setPose"))
    {
        if(!receivedData) receivedData = true;
        angularRate.Pitch = (*Params)->GetNumberField(TEXT("pitch"));
        angularRate.Yaw = (*Params)->GetNumberField(TEXT("yaw"));
        angularRate.Roll = (*Params)->GetNumberField(TEXT("roll"));
        velocity.X = (*Params)->GetNumberField(TEXT("x"));
        velocity.Y = (*Params)->GetNumberField(TEXT("y"));
        velocity.Z = (*Params)->GetNumberField(TEXT("z"));
        //UE_LOG(LogTemp, Log, TEXT("Updating location: X=%f, Y=%f, Z=%f"), X, Y, Z);
        //if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("SetPose"));
        //if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("SetPose"));
        UE_LOG(LogTemp, Log, TEXT("Target: %s, Received: RollRate=%.3f , PitchRate=%.3f , YawRate=%.3f , vel_x=%.3f , vel_y=%.3f , vel_z=%.3f "), *Target, angularRate.Roll, angularRate.Pitch, angularRate.Yaw, velocity.X, velocity.Y, velocity.Z);
    }
    else if (Command == TEXT("rain"))
    {
        toggleRain = true;
        startRain = (*Params)->GetBoolField(TEXT("value"));
        FString msg = startRain ? TEXT("True") : TEXT("False");
        if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, msg);
        UE_LOG(LogTemp, Log, TEXT("Setting rain to: %s"), startRain ? TEXT("True") : TEXT("False"));
    }
    else if (Command == TEXT("snow"))
    {
        toggleSnow = true;
        startSnow = (*Params)->GetBoolField(TEXT("value"));
        UE_LOG(LogTemp, Log, TEXT("Setting rain to: %s"), startSnow ? TEXT("True") : TEXT("False"));
    }
    else if (Command == TEXT("startStream"))
    {
        //toggleSnow = true;
        toggelStream = (*Params)->GetBoolField(TEXT("value"));
        UE_LOG(LogTemp, Log, TEXT("Stream state : %s"), toggelStream ? TEXT("True") : TEXT("False"));
        if (GEngine) {
            FString DebugMsg = FString::Printf(TEXT("Stream state: %s"), toggelStream ? TEXT("True") : TEXT("False"));
            GEngine->AddOnScreenDebugMessage(200, 120.f, FColor::Green, DebugMsg);
        }
    }
    else if (Command == TEXT("location"))
    {
        setlocation = true;
        targetLocation.X = (*Params)->GetNumberField(TEXT("x"));
        targetLocation.Y = (*Params)->GetNumberField(TEXT("y"));
        targetLocation.Z = (*Params)->GetNumberField(TEXT("z"));
        FString pose = (*Params)->GetStringField(TEXT("pose"));
        if (pose == TEXT("transOnly")) {
            targetRotation = FRotator();
            relTargetRotator = FRotator();
        }
        else{
            targetRotation.Pitch = (*Params)->GetNumberField(TEXT("pitch"));
            targetRotation.Yaw = (*Params)->GetNumberField(TEXT("yaw"));
            targetRotation.Roll = (*Params)->GetNumberField(TEXT("roll"));
            relTargetRotator.Pitch = (*Params)->GetNumberField(TEXT("rel_pitch"));
            relTargetRotator.Yaw = (*Params)->GetNumberField(TEXT("rel_yaw"));
            relTargetRotator.Roll = (*Params)->GetNumberField(TEXT("rel_roll"));
            useTargetRotation = true;
        }
        //if (Target == TEXT("drone")) gimCam = false;

        //UE_LOG(LogTemp, Log, TEXT("Updating location: X=%f, Y=%f, Z=%f"), X, Y, Z);
        UE_LOG(LogTemp, Log, TEXT("Received target Location: targetLocation=%.3f , targetLocation=%.3f , targetLocation=%.3f "), targetLocation.X, targetLocation.Y, targetLocation.Z);
    }
    
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Received unknown command: %s"), *Command);
    }

    if (Target == TEXT("drone")) gimCam = false;
    else if (Target == TEXT("gimbal")) gimCam = true;
    else if (Target == TEXT("arrived")) {
        arrivedTarget = true;
        receivedData = false;
        recImg_resize = true;
        CheckSocketHealth();
    }
    else if (Target == TEXT("final")) {
        arrivedTarget = true;
        receivedData = false;
        bShouldRun = false;
        StopStream();
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("Received unknown Target: %s"), *Target);
    }
}



void FRTSPStreamer::CheckSocketHealth()
{
    if (ClientSocket)
    {
        ESocketConnectionState State = ClientSocket->GetConnectionState();

        // If the state is not connected, or if we have an error, clean it up
        //if (State != SCS_Connected)
        //{
        UE_LOG(LogTemp, Warning, TEXT("Cleaning up socket in state: %d"), (int32)State);
        FString DebugMsg = FString::Printf(TEXT("Cleaning up socket in state: %d"), (int32)State);
        if (GEngine)  GEngine->AddOnScreenDebugMessage(201, 120.f, FColor::Green, DebugMsg);
        ClientSocket->Close();
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ClientSocket);
        ClientSocket = nullptr;
        //ClientSockets.RemoveAt(i);
        //}
    }
}


// FRTSPStreamer.cpp
void FRTSPStreamer::Shutdown()
{
    bShouldRun = false;

    if (ClientSocket)
    {
        ClientSocket->Close();
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ClientSocket);
        ClientSocket = nullptr;
    }
    if (ListenSocket)
    {
        ListenSocket->Close();
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(ListenSocket);
        ListenSocket = nullptr;
    }

    if (MetadataClientSocket)
    {
        MetadataClientSocket->Close();
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(MetadataClientSocket);
        MetadataClientSocket = nullptr;
    }
    if (MetadataListenSocket)
    {
        MetadataListenSocket->Close();
        ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(MetadataListenSocket);
        MetadataListenSocket = nullptr;
    }
}



// FRTSPStreamer.cpp

//bool FRTSPStreamer::StartMetadataServer()
//{
//    ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
//
//    // 1. Create the new listening socket
//    MetadataListenSocket = SocketSubsystem->CreateSocket(NAME_Stream, TEXT("MetadataSocket"), false);
//    if (!MetadataListenSocket) return false;
//
//    // 2. Set SO_REUSEADDR (Fixes Bind Fail on restart)
//    MetadataListenSocket->SetReuseAddr(true);
//
//    // 3. Setup Address (using the same IP as before)
//    FIPv4Address Addr;
//    FIPv4Address::Parse(ip_add_data, Addr); // ip_add_data is a member or defined
//
//    TSharedRef<FInternetAddr> InternetAddr1 = SocketSubsystem->CreateInternetAddr();
//    InternetAddr1->SetIp(Addr.Value);
//    InternetAddr1->SetPort(dataPort); // Use the new port (e.g., 9001)
//
//    // 4. Bind and Listen
//    if (!MetadataListenSocket->Bind(*InternetAddr1))
//    {
//        UE_LOG(LogTemp, Error, TEXT("Metadata Bind failed on port %d."), dataPort);
//        SocketSubsystem->DestroySocket(MetadataListenSocket);
//        //MetadataListenSocket = nullptr;
//        return false;
//    }
//
//    if (!MetadataListenSocket->Listen(1))
//    {
//        UE_LOG(LogTemp, Error, TEXT("Metadata Listening failed."));
//        SocketSubsystem->DestroySocket(MetadataListenSocket);
//        //MetadataListenSocket = nullptr;
//        return false;
//    }
//
//    // 5. Spawn an async task/thread to accept the connection
//    (new FAutoDeleteAsyncTask<FMetadataAcceptTask>(this))->StartBackgroundTask();
//
//    UE_LOG(LogTemp, Log, TEXT("Metadata TCP server started on port %d."), dataPort);
//    IsConnected = true;
//    return true;
//}



//bool FRTSPStreamer::ReceiveMetadata()
//{
//    //UE_LOG(LogTemp, Log, TEXT("In receive Metadata"));
//    if (!MetadataClientSocket || !IsConnected)
//    {
//        UE_LOG(LogTemp, Log, TEXT("Receive Metadata socket not connected"));
//        //if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Metadata client socket not connected "));
//        return false; // No connected client
//    }
//    bool recerived = false;
//    uint32 Size;
//    while (MetadataClientSocket->HasPendingData(Size))
//    {
//        //UE_LOG(LogTemp, Log, TEXT("In receive Metadata Listen Socket"));
//        TArray<uint8> ReceivedData;
//        ReceivedData.SetNumUninitialized(FMath::Min((int32)Size, 64)); // Read up to 64 bytes
//
//        int32 BytesRead = 0;
//        MetadataClientSocket->Recv(ReceivedData.GetData(), ReceivedData.Num(), BytesRead);
//        UE_LOG(LogTemp, Log, TEXT("In receive Metadata Listen Socket after BytesRead"));
//        if (BytesRead > 0)
//        {
//            // Convert received bytes to an FString (assuming ASCII/UTF8 format)
//            FString ReceivedString = FString(BytesRead, (char*)ReceivedData.GetData());
//            ReceivedString = ReceivedString.TrimStartAndEnd(); // Clean up whitespace
//            UE_LOG(LogTemp, Log, TEXT("receiving string... "));
//            // --- PARSING LOGIC: Assuming data is sent as "Alpha,Angle" ---
//            ReceivedString.TrimStartAndEndInline();
//            TArray<FString> Parts;
//            int count = ReceivedString.ParseIntoArray(Parts, TEXT(","), true);
//            if (!isReceiving) isReceiving = true;
//            if (count == 7)
//            {
//                // Convert string parts to float
//                angularRate.Pitch = FCString::Atof(*Parts[0]);
//                angularRate.Yaw = FCString::Atof(*Parts[1]);
//                angularRate.Roll = FCString::Atof(*Parts[2]);
//                velocity.X = FCString::Atof(*Parts[3]);
//                velocity.Y = FCString::Atof(*Parts[4]);
//                velocity.Z = FCString::Atof(*Parts[5]);
//                gimCam = (FCString::Atoi(*Parts[6]) == 0);
//                recerived = true;
//                // Log and use the values
//                UE_LOG(LogTemp, Log, TEXT("Metadata Received: RollRate=%.3f , PitchRate=%.3f , YawRate=%.3f , vel_x=%.3f , vel_y=%.3f , vel_z=%.3f "), angularRate.Roll, angularRate.Pitch, angularRate.Yaw, velocity.X, velocity.Y, velocity.Z);
//                FString msg = TEXT("Received: ") + ReceivedString;
//                //if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, msg);
//
//            }
//            else {
//                UE_LOG(LogTemp, Log, TEXT("Invalid parts count: %d"), count);
//                UE_LOG(LogTemp, Warning, TEXT("RAW data received: [%s]"), *ReceivedString);
//            }
//        }
//    }
//    //UE_LOG(LogTemp, Log, TEXT("out of receive Metadata"));
//    return recerived;
//}
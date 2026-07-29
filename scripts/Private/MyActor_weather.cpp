
#include "MyActor_weather.h"
#include "GameFramework/Character.h" // For ACharacter
#include "Components/CapsuleComponent.h" // For UCapsuleComponent
#include "NiagaraComponent.h" // For UNiagaraComponent
#include "Kismet/GameplayStatics.h" // For UGameplayStatics::GetPlayerCharacter


// For logging
#include "Engine/Engine.h" // Required for GEngine->AddOnScreenDebugMessage

// Sets default values
AMyActor_weather::AMyActor_weather()
{
	// Set this actor to call Tick() every frame. You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void AMyActor_weather::BeginPlay()
{
	Super::BeginPlay();

	//StartLog();

	InitializeEngel();

	Capture->bCaptureEveryFrame = false;
	Capture->bCaptureOnMovement = false;

}

void AMyActor_weather::InitializeEngel(){

	TargetThirdPersonCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0); // Get the first player character
	

	if (!TargetThirdPersonCharacter)
	{
		TargetThirdPersonCharacter = UGameplayStatics::GetPlayerCharacter(this, 0);
		if (!TargetThirdPersonCharacter)
		{
			UE_LOG(LogTemp, Warning, TEXT("MyActor_weather: Could not find player character automatically. Please set TargetThirdPersonCharacter in editor."));
		}
	}

	//TargetThirdPersonCharacter->SetActorLocation(locationLst[index], false, nullptr, ETeleportType::TeleportPhysics);
	//ChangeLocation();
	AMyCharacterBase* Char = Cast<AMyCharacterBase>(TargetThirdPersonCharacter);
	CamThirdPersonCharacter = Char->FindComponentByClass<UCameraComponent>();
	setCaptueCompParam();
	setCaptueCamera();
	Streamer.StartMetadataServer();
	StartStreamRTSP();

}


void AMyActor_weather::EndPlay(const EEndPlayReason::Type EndPlayReason)
{

	Streamer.Shutdown(); // a method you add to FRTSPStreamer that does the cleanup we discussed

	Super::EndPlay(EndPlayReason);
}

// Called every frame
void AMyActor_weather::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	AMyCharacterBase* Char = Cast<AMyCharacterBase>(TargetThirdPersonCharacter);
	Streamer.ReceiveMetadata();
	if (Char->recImg_resize || Streamer.recImg_resize) {
		//SaveActiveCameraImage();
		CaptureAndSaveImage();
		Char->recImg_resize = false;
		Streamer.recImg_resize = false;
	}
	if (Char->snowTog || Streamer.toggleSnow) {
		if (!snowStarted || Streamer.startSnow) {
			StartSnow();
			snowStarted = true;
		}
		else {
			StopSnow();
			snowStarted = false;
		}
		Char->snowTog = false;
	}
	if (Char->rainTog || Streamer.startRain) {
		if (!rainStarted || Streamer.toggleRain) {
			StartRain();
			rainStarted = true;
		}
		else {
			StopRain();
			rainStarted = false;
		}
		Char->rainTog = false;
	}
	if (Char->position) {
		index += 1;
		ChangeLocation(index);
		Char->position = false;
	}
	if(Streamer.setlocation){ 
		SetLocation(Streamer.targetLocation, Streamer.targetRotation, Streamer.relTargetRotator);
		Streamer.setlocation = false;
	}

	if (Char->writeLog || Streamer.receivedData) {
		if (!bLogFileInitialized) StartLog();
		SaveLog(DeltaTime);
	}
	//if (bEnableMotionLogging) SaveLog(DeltaTime);
	logNotification(Char);
	if (Streamer.arrivedTarget) {
		bLogFileInitialized = false;
		Streamer.arrivedTarget = false;
		//bEnableMotionLogging = false;
	}

	if (startcaptureCalled) {
		StartCapture(frameRate);
	}

	if (startCapture) {
		TimeSinceLastCapture += DeltaTime;
		if (TimeSinceLastCapture >= CaptureInterval) {
			CaptureAndSaveImage();
			SaveActiveCameraImage();
			TimeSinceLastCapture = 0.0f; // reset timer
		}
	}

	if (startStream) {
		TimeSinceLastImgStream += DeltaTime;
		if (TimeSinceLastImgStream >= StreamInterval) {
			StreamRTSP();
			TimeSinceLastImgStream = 0.0f; // reset stream timer
		}
	}

	if (startStreamCalled) {
		if (Streamer.receivedData) {
			ShareRate(DeltaTime);
			//Streamer.receivedData = false;
			//MoveObj(DeltaTime);
			//RotateObj(DeltaTime);
		}
	}
	
}


void AMyActor_weather::StartLog() {
	if (bLogFileInitialized) return;
	IntAsString = FString::FromInt(logPose);
	LogFilePath = FPaths::ProjectSavedDir() + "/MotionLog/"  + "MotionLog_" + IntAsString + "_" + FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")) + ".csv";
	//LogFilePath = FPaths::ProjectSavedDir() + "/MotionLog/" + "GT_" + IntAsString + "_MotionLog_" + FDateTime::Now().ToString(TEXT(" % Y % m % d_ % H % M % S")) + ".csv";
	//LogFilePath = FPaths::ProjectSavedDir() / FileName;
	// Write CSV header
	const FString Header = TEXT("Time,DeltaTime,PosX,PosY,PosZ,Pitch,Yaw,Roll,PitchCam,YawCam,RollCam\n");
	FFileHelper::SaveStringToFile(Header, *LogFilePath, FFileHelper::EEncodingOptions::AutoDetect, &IFileManager::Get(), FILEWRITE_None);

	bLogFileInitialized = true;
	logPose += 1;
	//bEnableMotionLogging = true;
	UE_LOG(LogTemp, Log, TEXT("Motion logging started: %s"), *LogFilePath);
}

void AMyActor_weather::logNotification(AMyCharacterBase* Char) {
	if (GEngine)
	{
		const FString Text = (Char->writeLog || Streamer.receivedData) ? TEXT("LOG: ON") : TEXT("LOG: OFF");
		const FColor Color = (Char->writeLog || Streamer.receivedData) ? FColor::Green : FColor::Red;
		// Use a constant key so it overwrites the same line
		GEngine->AddOnScreenDebugMessage(9001, 0.f, Color, Text);
	}
	if (!(Char->writeLog || Streamer.receivedData)) bLogFileInitialized = false;
}


TArray<UNiagaraComponent*> AMyActor_weather::GetWeatherComponent() const
{
	TArray<UNiagaraComponent*> NiagaraComponents;

	if (!TargetThirdPersonCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("MyActor_weather: TargetThirdPersonCharacter is null. Cannot get rain component."));
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("TargetThirdPersonCharacter is null!"));
		return NiagaraComponents;
	}

	// Step 1: Get the CapsuleComponent. ACharacter has a GetCapsuleComponent() method.
	UCapsuleComponent* CapsuleComp = TargetThirdPersonCharacter->GetCapsuleComponent();
	if (!CapsuleComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("MyActor_weather: TargetThirdPersonCharacter has no CapsuleComponent."));
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("CapsuleComponent is null!"));
		return NiagaraComponents;
	}


	// Iterate and check name (if you have multiple and need a specific one)


	TargetThirdPersonCharacter->GetComponents<UNiagaraComponent>(NiagaraComponents); // Get all Niagara components on the character

	return NiagaraComponents;
}

UNiagaraComponent* AMyActor_weather::GetRainComponent() const
{
	// Find the NiagaraComponent named "rain" within the CapsuleComponent's children.
	// IMPORTANT: The name "rain" here must exactly match the name of the Niagara component in BP.
	TArray<UNiagaraComponent*> NiagaraComponents = GetWeatherComponent();
	UNiagaraComponent* RainComponent = NULL;
	for (UNiagaraComponent* NiagaraComp : NiagaraComponents)
		{
			// Check the exact name you gave it in the Blueprint editor
			// Or a partial name check
			if (NiagaraComp->GetName() == TEXT("rain")) // Assuming you named it exactly "rain" in Blueprint
			{
				RainComponent = NiagaraComp;
				break;
			}
		}
		
	if (!RainComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("MyActor_weather: Could not find NiagaraComponent named 'rain' under CapsuleComponent."));
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Rain Niagara Component not found!"));
	}

	return RainComponent;
}

UNiagaraComponent* AMyActor_weather::GetSnowComponent() const
{
	// Find the NiagaraComponent named "snow" within the CapsuleComponent's children.
	// IMPORTANT: The name "snow" here must exactly match the name you gave the Niagara component in BP.
	TArray<UNiagaraComponent*> NiagaraComponents = GetWeatherComponent();
	UNiagaraComponent* SnowComponent = NULL;
	for (UNiagaraComponent* NiagaraComp : NiagaraComponents)
	{
		// Check the exact name you gave it in the Blueprint editor
		// Or a partial name check
		if (NiagaraComp->GetName() == TEXT("snow")) // Assuming you named it exactly "snow" in Blueprint
		{
			SnowComponent = NiagaraComp;
			break;
		}
	}

	if (!SnowComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("MyActor_weather: Could not find NiagaraComponent named 'snow' under CapsuleComponent."));
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Snow Niagara Component not found!"));
	}

	return SnowComponent;
}

void AMyActor_weather::ChangeLocation(int targetIndex)
{
	TargetThirdPersonCharacter->SetActorLocation(locationLst[targetIndex], false, nullptr, ETeleportType::TeleportPhysics);
}

void AMyActor_weather::SetLocation(const FVector& targetLocation, const FRotator& targetRotation, const FRotator& relTargetRotator)
{
	TargetThirdPersonCharacter->SetActorLocation(targetLocation, false, nullptr, ETeleportType::TeleportPhysics);
	if (Streamer.useTargetRotation) {
		TargetThirdPersonCharacter->SetActorRotation(targetRotation.Quaternion());
		CaptureSourceCamera->SetRelativeRotation(relTargetRotator.Quaternion());
		Streamer.useTargetRotation = false;
	}
}

void AMyActor_weather::ActivateRain()
{
	UNiagaraComponent* RainComp = GetRainComponent();
	if (RainComp)
	{
		RainComp->ActivateSystem(true); // true to reset the system if it was already active
		UE_LOG(LogTemp, Log, TEXT("Rain Activated!"));
		//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("Rain Activated!"));
	}
}

void AMyActor_weather::DeactivateRain()
{
	UNiagaraComponent* RainComp = GetRainComponent();
	if (RainComp)
	{
		RainComp->DeactivateImmediate();
		UE_LOG(LogTemp, Log, TEXT("Rain Deactivated!"));
		//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("Rain Deactivated!"));
	}
}


void AMyActor_weather::ActivateSnow()
{
	UNiagaraComponent* RainComp = GetSnowComponent();
	if (RainComp)
	{
		RainComp->ActivateSystem(true); // true to reset the system if it was already active
		UE_LOG(LogTemp, Log, TEXT("Snow Activated!"));
		//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("Rain Activated!"));
	}
}

void AMyActor_weather::DeactivateSnow()
{
	UNiagaraComponent* RainComp = GetSnowComponent();
	if (RainComp)
	{
		RainComp->DeactivateImmediate();
		UE_LOG(LogTemp, Log, TEXT("Snow Deactivated!"));
		//if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("Rain Deactivated!"));
	}
}

ACharacter* AMyActor_weather::TargetCharacter() {
	return TargetThirdPersonCharacter;
}

//void AMyActor_weather::MoveObject() {
//	TargetThirdPersonCharacter->AddMovementInput(CameraDirection, SpeedScale);
//}

void AMyActor_weather::MoveForwardForDistance(float DistanceInCm, float Speed)
{
	if (!TargetThirdPersonCharacter) return;

	StartLocation = TargetThirdPersonCharacter->GetActorLocation();
	FVector ForwardVector = TargetThirdPersonCharacter->GetActorForwardVector();
	TargetLocation = StartLocation + ForwardVector * DistanceInCm;

	MoveSpeed = Speed;
	Movecamera = true;

	UE_LOG(LogTemp, Log, TEXT("Started moving %.1f cm forward at %.1f cm/s"), DistanceInCm, Speed);
}

void AMyActor_weather::RotateCCW(FRotator angle, float angularSpeed)
{
	if (!TargetThirdPersonCharacter) return;

	StartRotator = TargetThirdPersonCharacter->GetActorRotation();
	TargetRotator = StartRotator + angle;

	AngularSpeed = angularSpeed;
	RotateCamera = true;

	UE_LOG(LogTemp, Log, TEXT("Started Rotating Camera by (%.1f, %.1f, %.1f) at %.1f cm/s"), angle.Pitch, angle.Yaw, angle.Roll, angularSpeed);
	UE_LOG(LogTemp, Log, TEXT("Started Rotating Camera to (%.1f, %.1f, %.1f) at %.1f cm/s"), TargetRotator.Pitch, TargetRotator.Yaw, TargetRotator.Roll, angularSpeed);
}

void AMyActor_weather::MoveObj(float DeltaTime)
{
	FVector CurrentLocation = TargetThirdPersonCharacter->GetActorLocation();
	FVector Direction = (TargetLocation - CurrentLocation).GetSafeNormal();
	FVector NewLocation;
	if (Movecamera) {
		float Step = MoveSpeed * DeltaTime;
		NewLocation = CurrentLocation + Direction * Step;
	}

	else {
		NewLocation = CurrentLocation + Streamer.velocity * DeltaTime;
	}
	
	UE_LOG(LogTemp, Log, TEXT("NewLocation: X=%.3f , Y=%.3f , Z=%.3f "), NewLocation.X, NewLocation.Y, NewLocation.Z);
	// Stop if we’re close enough or overshot
	if (FVector::DistSquared(NewLocation, StartLocation) >= FVector::DistSquared(TargetLocation, StartLocation) && Movecamera)
	{
		NewLocation = TargetLocation;
		Movecamera = false;
		TargetDestination = true; // Set flag to indicate we reached the target
		UE_LOG(LogTemp, Log, TEXT("Reached target location."));
	}

	TargetThirdPersonCharacter->SetActorLocation(NewLocation);
}

void AMyActor_weather::RotateObj(float DeltaTime)
{
	//UE_LOG(LogTemp, Log, TEXT("Rotate camera!"));
	FRotator CurrentRotator = TargetThirdPersonCharacter->GetActorRotation();
	float AngularStep;
	if (RotateCamera) {
		AngularStep = AngularSpeed * DeltaTime;
	}
	else {
		AngularStep = Streamer.YawRate_drone * DeltaTime;
	}
	FRotator NewRotator = CurrentRotator;

	if (CurrentRotator.Pitch < TargetRotator.Pitch) {
		NewRotator.Pitch = CurrentRotator.Pitch + AngularStep;
	}

	if (CurrentRotator.Roll < TargetRotator.Roll) {
		NewRotator.Roll = CurrentRotator.Roll + AngularStep;
	}

	if (CurrentRotator.Yaw < TargetRotator.Yaw) {
		NewRotator.Yaw = CurrentRotator.Yaw + AngularStep;
	}

	// Stop if we’re close enough or overshot
	if (NewRotator.GetManhattanDistance(StartRotator) >= TargetRotator.GetManhattanDistance(StartRotator) && RotateCamera)
	{
		NewRotator = TargetRotator;
		RotateCamera = false;
		TargetAngle = true; // Set flag to indicate we reached the target
		UE_LOG(LogTemp, Log, TEXT("Reached target Rotation."));
	}

	TargetThirdPersonCharacter->SetActorRotation(NewRotator.Quaternion());
}

//void AMyActor_weather::setCaptueCompParam()
//{
//	//UE_LOG(LogTemp, Log, TEXT("Capture and Save Image."));
//
//	if (!TargetThirdPersonCharacter) return;
//
//	Capture = TargetThirdPersonCharacter->FindComponentByClass<USceneCaptureComponent2D>();
//
//	if (!Capture || !Capture->TextureTarget) {
//		UE_LOG(LogTemp, Error, TEXT("Capture or TextureTarget is null"));
//		return;
//	}
//}



UCameraComponent* FindCameraByTag(AActor* Owner, FName Tag)
{
	if (!Owner) return nullptr;

	TArray<UActorComponent*> Components;
	Owner->GetComponents(UCameraComponent::StaticClass(), Components);

	for (UActorComponent* Comp : Components)
	{
		if (UCameraComponent* Cam = Cast<UCameraComponent>(Comp))
		{
			if (Cam->ComponentHasTag(Tag))
				return Cam;
		}
	}
	return nullptr;
}



void AMyActor_weather::setCaptueCompParam()
{
	if (!TargetThirdPersonCharacter) return;

	Capture = TargetThirdPersonCharacter->FindComponentByClass<USceneCaptureComponent2D>();

	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (!(PC && PC->PlayerCameraManager)) return;

	FVector CamLoc = PC->PlayerCameraManager->GetCameraLocation();
	FRotator CamRot = PC->PlayerCameraManager->GetCameraRotation();

	UE_LOG(LogTemp, Log, TEXT("Active Camera Location: %s, Rotation: %s"),
		*CamLoc.ToString(), *CamRot.ToString());

	PC->GetPlayerViewPoint(CamLoc, CamRot);
	

	if (!Capture || !Capture->TextureTarget)
	{
		UE_LOG(LogTemp, Error, TEXT("Capture or TextureTarget is null"));
		return;
	}

	CaptureSourceCamera = FindCameraByTag(TargetThirdPersonCharacter, TEXT("CaptureSource"));

	if (!CaptureSourceCamera)
	{
		UE_LOG(LogTemp, Error, TEXT("CaptureSourceCamera NOT FOUND. Did you tag Camera_A?"));
		return;
	}


	// Lock capture to Camera_B forever
	Capture->AttachToComponent(
		CaptureSourceCamera,
		FAttachmentTransformRules::SnapToTargetNotIncludingScale
	);
	Capture->SetRelativeLocation(FVector::ZeroVector);
	Capture->SetRelativeRotation(FRotator::ZeroRotator);
	Capture->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;


	// Optional: match camera FOV
	//Capture->FOVAngle = CaptureSourceCamera->FieldOfView;
}



void AMyActor_weather::setCaptueCamera() {

	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PC && PC->PlayerCameraManager)
	{
		FVector CamLoc = PC->PlayerCameraManager->GetCameraLocation();
		FRotator CamRot = PC->PlayerCameraManager->GetCameraRotation();

		UE_LOG(LogTemp, Log, TEXT("Active Camera Location: %s, Rotation: %s"),
			*CamLoc.ToString(), *CamRot.ToString());
		PC->GetPlayerViewPoint(CamLoc, CamRot);
	}

}


void AMyActor_weather::CaptureAndSaveImage()
{
	
	// Force the capture
	Capture->CaptureScene();
	

	// Get render target resource
	FRenderTarget* RenderTarget = Capture->TextureTarget->GameThread_GetRenderTargetResource();
	if (!RenderTarget) {
		UE_LOG(LogTemp, Error, TEXT("RenderTarget is null"));
		return;
	}

	// Read pixels
	//FReadSurfaceDataFlags ReadPixelFlags(RCM_UNorm);
	//ReadPixelFlags.SetLinearToGamma(true); // Correct color
	TArray<FColor> Bitmap, NewBitmap;
	RenderTarget->ReadPixels(Bitmap);

	FIntPoint Size(Capture->TextureTarget->SizeX, Capture->TextureTarget->SizeY);

	if (Bitmap.Num() <= 0) {
		UE_LOG(LogTemp, Error, TEXT("No pixels captured."));
		return;
	}
	if (ScreenshotPath.IsEmpty()) ScreenshotPath = FPaths::ProjectSavedDir() + "/single_cam_rec" + "/";
	// Create screenshot folder
	//FString ScreenshotPath = FPaths::ProjectSavedDir() + "/recodings_" + FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"))+"/";
	//IFileManager::Get().MakeDirectory(*ScreenshotPath, true);
	//FString IntAsString = FString::FromInt(logPose);
	//LogFilePath = FPaths::ProjectSavedDir() + "/MotionLog/" + "GT" + IntAsString + "_MotionLog_" + FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")) + ".csv";
	FString Filename = ScreenshotPath + "GT" + IntAsString + "_Capture_" + FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")) + ".png";

	// Save the image
	TArray<uint8> PNGData;
	//AMyActor_weather::ResizeBitmap(Bitmap, Size.X, Size.Y, NewBitmap);#
	FImageUtils::CompressImageArray(Size.X, Size.Y, Bitmap, PNGData);
	FFileHelper::SaveArrayToFile(PNGData, *Filename);

	//UE_LOG(LogTemp, Log, TEXT("Saved camera image to: %s"), *Filename);
}


void AMyActor_weather::StartCapture( float frameRate_)
{
	// Start capturing images at a targeted frame rate

	if (!TargetThirdPersonCharacter) return;
	//USceneCaptureComponent2D* Capture = TargetThirdPersonCharacter->FindComponentByClass<USceneCaptureComponent2D>();
	if (!Capture || !Capture->TextureTarget) {
		UE_LOG(LogTemp, Error, TEXT("Capture or TextureTarget is null"));
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Capture or TextureTarget is null!"));
		startcaptureCalled = true;
		frameRate = frameRate_;
		return;
	}

	ScreenshotPath = FPaths::ProjectSavedDir() + "/recodings_" + FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"))+"/";
	ScreenshotPath_camera = FPaths::ProjectSavedDir() + "/recodings_camera_" + FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")) + "/";
	//IFileManager::Get().MakeDirectory(*ScreenshotPath, true);
	CaptureInterval = 1 / frameRate_;
	startCapture = true;
	TimeSinceLastCapture = 0.0f; // Reset timer
	UE_LOG(LogTemp, Log, TEXT("Started capturing images at frequency of %0.2f fps and saving in %s"), 1 / CaptureInterval, *ScreenshotPath);
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Started capturing images "));
	startcaptureCalled = false;
}

void AMyActor_weather::StartStreamRTSP(float InFPS, FString ServerURL)
{
	if (!TargetThirdPersonCharacter) return;
	//USceneCaptureComponent2D* Capture = TargetThirdPersonCharacter->FindComponentByClass<USceneCaptureComponent2D>();
	if (!Capture || !Capture->TextureTarget) {
		UE_LOG(LogTemp, Error, TEXT("Capture or TextureTarget is null"));
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Capture or TextureTarget is null!"));
		startStreamCalled = false;
		streamRate = InFPS;
		return;
	}

	// Start streaming at a targeted frame rate
	StreamInterval = 1 / InFPS;
	streamRate = InFPS;
	//FViewport* Viewport = GEngine->GameViewport->Viewport;
	//if (!Viewport) return;
	//int32 Width = Viewport->GetSizeXY().X;
	//int32 Height = Viewport->GetSizeXY().Y;
	int32 Width = Capture->TextureTarget->SizeX;
	int32 Height = Capture->TextureTarget->SizeY;
	FString MyWidth = FString::FromInt(Width);
	FString MyHeight = FString::FromInt(Height);
	FString msg = TEXT("Width and Height are ") + MyWidth + TEXT(" and ") + MyHeight;
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, msg);
	startStream = Streamer.StartTCPServer(Width, Height); // && Streamer.StartMetadataServer();
	//Streamer.StartUDPServer();
	startStream = true;
	// 2974 x 1036
	//if (startStream) {
	//	startStream = Streamer.StartStream(Width, Height, InFPS, ServerURL);
	//}
	//if (startStream) { StreamRTSP(); }

	TimeSinceLastImgStream = 0.0f; // Reset timer
	streamAddress = ServerURL;
	UE_LOG(LogTemp, Log, TEXT("Started stream with image of size %dx%d"), Width, Height);
	FString Msg = FString::Printf(TEXT("Started stream with image of size %dx%d"), Width, Height);
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, Msg);
	UE_LOG(LogTemp, Log, TEXT("Started stream at frequency of %0.2f fps and saving in %s"), 1 / StreamInterval, *ServerURL);
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Started Stream "));
	startStreamCalled = true;
}


void AMyActor_weather::StreamRTSP()
{
	if (!GEngine || !GEngine->GameViewport) return;

	//FViewport* Viewport = GEngine->GameViewport->Viewport;
	//if (!Viewport) return;

	//TArray<FColor> Bitmap, NewBitmap;
	// This is the heavy operation. Since you prefer this over SceneCapture,
	// expect a frame drop here, but the visual quality will be perfect.


	// Force the capture
	Capture->CaptureScene();


	// Get render target resource
	FRenderTarget* RenderTarget = Capture->TextureTarget->GameThread_GetRenderTargetResource();
	if (!RenderTarget) {
		UE_LOG(LogTemp, Error, TEXT("RenderTarget is null"));
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("RenderTarget is null "));
		return;
	}

	// Read pixels
	//FReadSurfaceDataFlags ReadPixelFlags(RCM_UNorm);
	//ReadPixelFlags.SetLinearToGamma(true); // Correct color
	TArray<FColor> Bitmap, NewBitmap;
	RenderTarget->ReadPixels(Bitmap);

	FIntPoint Size(Capture->TextureTarget->SizeX, Capture->TextureTarget->SizeY);

	//if (Viewport->ReadPixels(Bitmap))
	//{
	if (Bitmap.Num() >= 0) {
		
		//int32 Width = Viewport->GetSizeXY().X;
		//int32 Height = Viewport->GetSizeXY().Y;
		int32 Width = Size.X;
		int32 Height = Size.Y;
		//AMyActor_weather::ResizeBitmap(Bitmap, Width, Height, NewBitmap);
		Streamer.SendFrameTCP(Bitmap);

		// If you still need the file saved to disk occasionally, 
		// you can keep your old PNG saving logic here, but don't do it every frame.
	}
}



void AMyActor_weather::StopCapture()
{
	// Stop capturing images
	startCapture = false;
	TimeSinceLastCapture = 0.0f; // Reset timer
	UE_LOG(LogTemp, Log, TEXT("Stopped capturing images."));
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Stopped capturing images."));
}

void AMyActor_weather::StartRain( float Rainrate, FVector WindSpeed )
{
	// Start the rain effect with specified parameters
	// Rainrate: Rate of rain spawn in cm/s
	// WindSpeed: Direction of wind that affects rain fall in cm/s

	UNiagaraComponent* RainComp = GetRainComponent();
	if (!RainComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("MyActor_weather: Rain component not found! Cannot start rain."));
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Rain component not found!"));
		return;
	}
	WindSpeed += FVector(0.0f, 0.0f, -980.0f); // Default downward speed if not specified

	RainComp->SetVectorParameter(TEXT("User.RainGravityForce"), WindSpeed);
	RainComp->SetFloatParameter(TEXT("User.SpawnRainRate"), Rainrate*100);
	UE_LOG(LogTemp, Log, TEXT("Starting rain..."));
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Rain activated from script"));

	ActivateRain();
}

void AMyActor_weather::StopRain()
{
	// Stop the rain effect
	UNiagaraComponent* RainComp = GetRainComponent();
	if (!RainComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("MyActor_weather: Rain component not found! Cannot stop rain."));
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Rain component not found!"));
		return;
	}
	RainComp->SetFloatParameter(TEXT("User.SpawnRainRate"), 0.0f);
	DeactivateRain();
	UE_LOG(LogTemp, Log, TEXT("Stopping rain..."));
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Rain deactivated from script"));
}


void AMyActor_weather::StartSnow(float SnowRate, FVector WindSpeed )
{
	// Start the snow effect with specified parameters
	// SnowRate: Rate of snow spawn in cm/s
	// WindSpeed: Direction of wind that affects snow fall in cm/s
	if (!TargetThirdPersonCharacter)
	{
		TargetThirdPersonCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0);
	}
	UNiagaraComponent* SnowComp = GetSnowComponent();
	if (!SnowComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("MyActor_weather: Snow component not found! Cannot start snow."));
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Snow component not found!"));
		return;
	}
	WindSpeed += FVector(0.0f, 0.0f, -980.0f); // Default downward speed if not specified
	SnowComp->SetVectorParameter(TEXT("User.SnowForce"), WindSpeed);
	SnowComp->SetFloatParameter(TEXT("User.SnowRate"), SnowRate * 100);
	SnowComp->SetFloatParameter(TEXT("User.SnowRate1"), SnowRate * 10);
	UE_LOG(LogTemp, Log, TEXT("Starting snow..."));
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Snow activated from script"));
	ActivateSnow();
}

void AMyActor_weather::StopSnow()
{
	// Stop the snow effect
	UNiagaraComponent* SnowComp = GetSnowComponent();
	if (!SnowComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("MyActor_weather: Snow component not found! Cannot stop snow."));
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Snow component not found!"));
		return;
	}
	SnowComp->SetFloatParameter(TEXT("User.SpawnSnowRate"), 0.0f);
	DeactivateSnow();
	UE_LOG(LogTemp, Log, TEXT("Stopping snow..."));
	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("Snow activated from script"));
}


void AMyActor_weather::SaveActiveCameraImage(bool keep_size)
{
	if (!GEngine || !GEngine->GameViewport) return;

	FViewport* Viewport = GEngine->GameViewport->Viewport;
	if (!Viewport) return;

	if (ScreenshotPath_camera.IsEmpty()) ScreenshotPath_camera = FPaths::ProjectSavedDir() + "/single_cam_rec" +  "/";

	TArray<FColor> Bitmap, NewBitmap;
	if (Viewport->ReadPixels(Bitmap))
	{
		int32 Width = Viewport->GetSizeXY().X;
		int32 Height = Viewport->GetSizeXY().Y;

		FString FilePath = FPaths::ProjectSavedDir() / TEXT("ActiveCamera.png");
		//FIntRect Rect(0, 0, Width, Height);

		//FHighResScreenshotConfig& ScreenshotConfig = GetHighResScreenshotConfig();
		//ScreenshotConfig.SaveImage(FilePath, Bitmap, Rect);

		//UE_LOG(LogTemp, Log, TEXT("Saved image from active camera to %s"), *FilePath);

		FString Filename = ScreenshotPath_camera + "Capture_" + FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")) + ".png";

		// Save the image
		TArray<uint8> PNGData;
		if (keep_size) FImageUtils::CompressImageArray(Width, Height, Bitmap, PNGData);
		else {
			AMyActor_weather::ResizeBitmap(Bitmap, Width, Height, NewBitmap);
			FImageUtils::CompressImageArray(NewW, NewH, NewBitmap, PNGData);
		}
		FFileHelper::SaveArrayToFile(PNGData, *Filename);
	}
}


void AMyActor_weather::ResizeBitmap(const TArray<FColor>& Src, int SrcW, int SrcH, TArray<FColor>& Dst)
{
	Dst.SetNumUninitialized(NewW * NewH);

	for (int y = 0; y < NewH; y++)
	{
		float SrcY = (float(y) / float(NewH)) * SrcH;

		for (int x = 0; x < NewW; x++)
		{
			float SrcX = (float(x) / float(NewW)) * SrcW;

			int X0 = FMath::Clamp(int(SrcX), 0, SrcW - 1);
			int Y0 = FMath::Clamp(int(SrcY), 0, SrcH - 1);

			Dst[y * NewW + x] = Src[Y0 * SrcW + X0];
		}
	}
}

//void AMyActor_weather::ShareRate(float DeltaTime)
//{
//	UE_LOG(LogTemp, Log, TEXT("In Share Metadata"));
//	if (TargetThirdPersonCharacter)
//	{
//		AMyCharacterBase* Char =
//			Cast<AMyCharacterBase>(TargetThirdPersonCharacter);
//
//		float FakeMouseX = Streamer.YawRate_cam * DeltaTime;
//		float FakeMouseY = Streamer.PitchRate_cam * DeltaTime;
//		FVector2D camRot = FVector2D(FakeMouseX, FakeMouseY);
//		if (Char)
//		{
//			Char->scriptRot = true;
//			Char->CamRot = camRot;
//			Char->ApplyVirtualLook(camRot);
//			FString Msg = FString::Printf(TEXT("Recieved camRot: %.2f, %.2f"), Streamer.YawRate_cam, Streamer.PitchRate_cam);
//			if (GEngine) GEngine->AddOnScreenDebugMessage(42, 5.f, FColor::Green, Msg);
//		}
//		else{ GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("No Char")); }
//	}
//}


void AMyActor_weather::ShareRate(float DeltaTime)
{
	UE_LOG(LogTemp, Log, TEXT("In Share Metadata"));

	if (TargetThirdPersonCharacter) {
		FVector CurrentLocation = TargetThirdPersonCharacter->GetActorLocation();
		FVector position = Streamer.velocity * DeltaTime;
		FVector NewLocation = CurrentLocation + position;

		UE_LOG(LogTemp, Log, TEXT("New Location: X=%.3f , Y=%.3f , Z=%.3f "), NewLocation.X, NewLocation.Y, NewLocation.Z);
		UE_LOG(LogTemp, Log, TEXT("Rel Position: X=%.3f , Y=%.3f , Z=%.3f "), position.X, position.Y, position.Z);
		FString Msg = FString::Printf(TEXT("Rel Position: X=%.3f , Y=%.3f , Z=%.3f "), position.X, position.Y, position.Z);
		if (GEngine) GEngine->AddOnScreenDebugMessage(2, 5.f, FColor::Green, Msg);
		Msg = FString::Printf(TEXT("New Position: X=%.3f , Y=%.3f , Z=%.3f "), NewLocation.X, NewLocation.Y, NewLocation.Z);
		if (GEngine) GEngine->AddOnScreenDebugMessage(20, 5.f, FColor::Green, Msg);

		//TargetThirdPersonCharacter->SetActorLocation(NewLocation);
		//TargetThirdPersonCharacter->AddActorWorldOffset(position, true);
		TargetThirdPersonCharacter->AddActorLocalOffset(position, true);

		//UE_LOG(LogTemp, Log, TEXT("Rotate camera!"));
		FRotator CurrentRotator = TargetThirdPersonCharacter->GetActorRotation();
		FRotator angle = Streamer.angularRate * DeltaTime;
		FRotator NewRotator = CurrentRotator + angle;
		Msg = FString::Printf(TEXT("RelRot: Roll=%.3f , Pitch=%.3f , Yaw=%.3f "), angle.Roll, angle.Pitch, angle.Yaw);
		if (GEngine) GEngine->AddOnScreenDebugMessage(35, 5.f, FColor::Green, Msg);
		Msg = FString::Printf(TEXT("CurrentRotator: Roll=%.3f , Pitch=%.3f , Yaw=%.3f "), CurrentRotator.Roll, CurrentRotator.Pitch, CurrentRotator.Yaw);
		if (GEngine) GEngine->AddOnScreenDebugMessage(33, 5.f, FColor::Green, Msg);

		if (Streamer.gimCam) {
			//CaptureSourceCamera->AddRelativeRotation(angle);
			//const FRotator CamRel = CaptureSourceCamera->GetRelativeRotation();
			FRotator camRot = CaptureSourceCamera->GetRelativeRotation();
			FRotator newCamRot = camRot + angle;
			CaptureSourceCamera->SetRelativeRotation(newCamRot.Quaternion());
			Msg = FString::Printf(TEXT(" NewRot Gim: Roll=%.3f , Pitch=%.3f , Yaw=%.3f "), newCamRot.Roll, newCamRot.Pitch, newCamRot.Yaw);
			if (GEngine) GEngine->AddOnScreenDebugMessage(32, 5.f, FColor::Green, Msg);
			
			//UE_LOG(LogTemp, Log, TEXT("Camera_A RelRot: "), *CamRel.ToString());
			//Msg = TEXT("Camera_A RelRot: %s") + CamRel.ToString();
			//if (GEngine) GEngine->AddOnScreenDebugMessage(42, 5.f, FColor::Green, Msg);
		}
		else {
			TargetThirdPersonCharacter->SetActorRotation(NewRotator.Quaternion());
			Msg = FString::Printf(TEXT("NewRot: Roll=%.3f , Pitch=%.3f , Yaw=%.3f"), NewRotator.Roll, NewRotator.Pitch, NewRotator.Yaw);
			if (GEngine) GEngine->AddOnScreenDebugMessage(42, 5.f, FColor::Green, Msg);
		}
		//UE_LOG(LogTemp, Log, TEXT("Rel Rotation: Roll=%.3f , Pitch=%.3f , Yaw=%.3f "), angle.Roll, angle.Pitch, angle.Yaw);
			
		
		
	}
}


void AMyActor_weather::SaveLog(float DeltaTime) {
	if ( !bLogFileInitialized || !TargetThirdPersonCharacter)
		return;

	const float Time = GetWorld()->GetTimeSeconds();
	const FVector Pos = TargetThirdPersonCharacter->GetActorLocation();
	const FRotator Rot = TargetThirdPersonCharacter->GetActorRotation();
	const FRotator RotCam = CaptureSourceCamera->GetRelativeRotation();
	//const FVector Pos = CamThirdPersonCharacter->GetComponentLocation();
	//const FRotator Rot = CamThirdPersonCharacter->GetComponentRotation();


	const FString Line = FString::Printf(
		TEXT("%.6f,%.6f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f\n"),
		Time,
		DeltaTime,
		Pos.X, Pos.Y, Pos.Z,
		Rot.Pitch, Rot.Yaw, Rot.Roll,
		RotCam.Pitch, RotCam.Yaw, RotCam.Roll
	);

	FFileHelper::SaveStringToFile(
		Line,
		*LogFilePath,
		FFileHelper::EEncodingOptions::AutoDetect,
		&IFileManager::Get(),
		FILEWRITE_Append
	);

}

void AMyCharacterBase::ApplyVirtualLook(const FVector2D& camRot)
{
	AddControllerYawInput(camRot.X);
	AddControllerPitchInput(camRot.Y);
}



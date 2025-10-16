
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

	// Optional: If TargetThirdPersonCharacter is not set in BP, try to find the player character
	/*
	if (!TargetThirdPersonCharacter)
	{
		TargetThirdPersonCharacter = UGameplayStatics::GetPlayerCharacter(this, 0);
		if (!TargetThirdPersonCharacter)
		{
			UE_LOG(LogTemp, Warning, TEXT("MyActor_weather: Could not find player character automatically. Please set TargetThirdPersonCharacter in editor."));
		}
	}
	*/
	InitializeEngel();

}

void AMyActor_weather::InitializeEngel()
{
	TargetThirdPersonCharacter = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0); // Get the first player character

	if (!TargetThirdPersonCharacter)
	{
		TargetThirdPersonCharacter = UGameplayStatics::GetPlayerCharacter(this, 0);
		if (!TargetThirdPersonCharacter)
		{
			UE_LOG(LogTemp, Warning, TEXT("MyActor_weather: Could not find player character automatically. Please set TargetThirdPersonCharacter in editor."));
		}
	}

	setCaptueCompParam();
	setCaptueCamera();
}

// Called every frame
void AMyActor_weather::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (Movecamera) {
		MoveObj(DeltaTime);
	}
	if (startCapture) {
		TimeSinceLastCapture += DeltaTime;
		if (TimeSinceLastCapture >= CaptureInterval) {
			CaptureAndSaveImage();
			SaveActiveCameraImage();
			TimeSinceLastCapture = 0.0f; // reset timer
		}
	}
	if (RotateCamera) {
		
		RotateObj(DeltaTime);
	}
	
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
	// IMPORTANT: The name "rain" here must exactly match the name you gave the Niagara component in BP.
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

	float Step = MoveSpeed * DeltaTime;
	FVector NewLocation = CurrentLocation + Direction * Step;

	// Stop if we’re close enough or overshot
	if (FVector::DistSquared(NewLocation, StartLocation) >= FVector::DistSquared(TargetLocation, StartLocation))
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

	float AngularStep = AngularSpeed * DeltaTime;
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
	if (NewRotator.GetManhattanDistance(StartRotator) >= TargetRotator.GetManhattanDistance(StartRotator))
	{
		NewRotator = TargetRotator;
		RotateCamera = false;
		TargetAngle = true; // Set flag to indicate we reached the target
		UE_LOG(LogTemp, Log, TEXT("Reached target Rotation."));
	}

	TargetThirdPersonCharacter->SetActorRotation(NewRotator.Quaternion());
}

void AMyActor_weather::setCaptueCompParam()
{
	//UE_LOG(LogTemp, Log, TEXT("Capture and Save Image."));

	if (!TargetThirdPersonCharacter) return;

	Capture = TargetThirdPersonCharacter->FindComponentByClass<USceneCaptureComponent2D>();

	if (!Capture || !Capture->TextureTarget) {
		UE_LOG(LogTemp, Error, TEXT("Capture or TextureTarget is null"));
		return;
	}
}

void AMyActor_weather::setCaptueCamera() {

	APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PC && PC->PlayerCameraManager)
	{
		FVector CamLoc = PC->PlayerCameraManager->GetCameraLocation();
		FRotator CamRot = PC->PlayerCameraManager->GetCameraRotation();

		UE_LOG(LogTemp, Log, TEXT("Active Camera Location: %s, Rotation: %s"),
			*CamLoc.ToString(), *CamRot.ToString());
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
	FReadSurfaceDataFlags ReadPixelFlags(RCM_UNorm);
	ReadPixelFlags.SetLinearToGamma(true); // Correct color
	TArray<FColor> Bitmap;
	RenderTarget->ReadPixels(Bitmap, ReadPixelFlags);

	FIntPoint Size(Capture->TextureTarget->SizeX, Capture->TextureTarget->SizeY);

	if (Bitmap.Num() <= 0) {
		UE_LOG(LogTemp, Error, TEXT("No pixels captured."));
		return;
	}

	// Create screenshot folder
	//FString ScreenshotPath = FPaths::ProjectSavedDir() + "/recodings_" + FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"))+"/";
	//IFileManager::Get().MakeDirectory(*ScreenshotPath, true);

	FString Filename = ScreenshotPath + "Capture_" + FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")) + ".png";

	// Save the image
	TArray<uint8> PNGData;
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
		return;
	}
	ScreenshotPath = FPaths::ProjectSavedDir() + "/recodings_" + FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"))+"/";
	ScreenshotPath_camera = FPaths::ProjectSavedDir() + "/recodings_camera_" + FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S")) + "/";
	IFileManager::Get().MakeDirectory(*ScreenshotPath, true);
	CaptureInterval = 1 / frameRate_;
	startCapture = true;
	TimeSinceLastCapture = 0.0f; // Reset timer
	UE_LOG(LogTemp, Log, TEXT("Started capturing images at frequency of %0.2f fps and saving in %s"), 1 / CaptureInterval, *ScreenshotPath);
	if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green, TEXT("Started capturing images "));
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


void AMyActor_weather::SaveActiveCameraImage()
{
	if (!GEngine || !GEngine->GameViewport) return;

	FViewport* Viewport = GEngine->GameViewport->Viewport;
	if (!Viewport) return;

	TArray<FColor> Bitmap;
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
		FImageUtils::CompressImageArray(Width, Height, Bitmap, PNGData);
		FFileHelper::SaveArrayToFile(PNGData, *Filename);
	}
}

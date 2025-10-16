// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include "Engine/SceneCapture2D.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/SceneCapture2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "HighResScreenshot.h"
#include "ImageUtils.h" // Needed for FImageUtils::CreateBitmap

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MyActor_weather.generated.h"
//#include<iomanip>

class ACharacter; // Forward declare ACharacter
class UNiagaraComponent; // Forward declare NiagaraComponent


UCLASS()
class AMyActor_weather : public AActor
{
	GENERATED_BODY()
	
public:	
	// Sets default values for this actor's properties
	AMyActor_weather();

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;


public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

	void MoveForwardForDistance(float DistanceInCm, float Speed);

	void RotateCCW(FRotator angle, float angularSpeed);

	/** Reference to the Third Person Character in the world.
	 * Set this in the Blueprint editor after placing MyActor_weather,
	 * or find it dynamically in BeginPlay.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera")
	ACharacter* TargetThirdPersonCharacter; // Or AMyThirdPersonCharacter* if you have one

	//FVector CameraDirection;
	//float SpeedScale;
	bool Movecamera;
	bool RotateCamera;
	bool startCapture;
	FVector StartLocation;
	FRotator  StartRotator;
	FRotator TargetRotator;
	FVector TargetLocation;
	float MoveSpeed;
	float AngularSpeed;
	//float frameRate;
	bool TargetDestination = false;
	bool TargetAngle = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Camera")
	USceneCaptureComponent2D* Capture;

	void CaptureAndSaveImage();

	void SaveActiveCameraImage();

	void setCaptueCamera();

	void InitializeEngel();

	void setCaptueCompParam();

	UFUNCTION(BlueprintCallable, Category = "Weather")
	ACharacter* TargetCharacter();

	/** Function to activate the rain effect. */
	UFUNCTION(BlueprintCallable, Category = "Weather")
	void ActivateRain();

	/** Function to deactivate the rain effect. */
	UFUNCTION(BlueprintCallable, Category = "Weather")
	void DeactivateRain();

	/** Function to activate the snow effect. */
	UFUNCTION(BlueprintCallable, Category = "Weather")
	void ActivateSnow();

	/** Function to deactivate the snow effect. */
	UFUNCTION(BlueprintCallable, Category = "Weather")
	void DeactivateSnow();

	// Helper function to get the Niagara Component
	TArray<UNiagaraComponent*> GetWeatherComponent() const;

	// Helper function to get the Niagara Rain Component
	UNiagaraComponent* GetRainComponent() const;

	// Helper function to get the Niagara Rain Component
	UNiagaraComponent* GetSnowComponent() const;

	void MoveObj(float DeltaTime);

	void RotateObj(float DeltaTime);

	void StartRain(float Rainrate, FVector RaingSpeed = FVector(0.0f, 0.0f, 0.0f));

	void StopRain();

	void StartSnow(float SnowRate, FVector SnowSpeed = FVector(0.0f, 0.0f, 0.0f));

	void StopSnow();

	void StartCapture(float frameRate);

	void StopCapture();
	// Helper function to move the Camera Component
	//void MoveObject();

private:
	float TimeSinceLastCapture = 0.0f;
	float CaptureInterval; // 20 fps = 0.05 seconds
	FString ScreenshotPath;
	FString ScreenshotPath_camera;

};

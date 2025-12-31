// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "InputActionValue.h"
#include "GameFramework/Pawn.h"
#include "IsoCameraPawn.generated.h"

class UCameraComponent;
class USceneComponent;
class UInputMappingContext;
class UInputAction;

UCLASS()
class ISOMETRICCAMERA_API AIsoCameraPawn : public APawn
{
	GENERATED_BODY()

public:
	AIsoCameraPawn();

protected:
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

private:
	// Components
	UPROPERTY(VisibleAnywhere, Category="Components")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, Category="Components")
	TObjectPtr<UCameraComponent> Camera;

	UPROPERTY(EditAnywhere, Category="Follow")
	TObjectPtr<AActor> FollowTarget = nullptr;

	UPROPERTY(EditAnywhere, Category="Follow")
	FVector FollowOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, Category="Follow")
	bool bAutoCenterFollowTarget = true;

	// 0 = bottom of bounds, 0.5 = middle, 1 = top
	UPROPERTY(EditAnywhere, Category="Follow", meta=(ClampMin="0.0", ClampMax="1.0", EditCondition="bAutoCenterFollowTarget"))
	float FollowAimHeightAlpha = 0.35f;

    // Enhanced Input assets (assign in BP or Details)
    UPROPERTY(EditDefaultsOnly, Category="Input|Enhanced")
    TObjectPtr<UInputMappingContext> CameraMappingContext;

    UPROPERTY(EditDefaultsOnly, Category="Input|Enhanced")
    TObjectPtr<UInputAction> IA_Zoom;

	UPROPERTY(EditDefaultsOnly, Category="Input|Enhanced")
	TObjectPtr<UInputAction> IA_PanForward;

	UPROPERTY(EditDefaultsOnly, Category="Input|Enhanced")
	TObjectPtr<UInputAction> IA_PanRight;

    UPROPERTY(EditDefaultsOnly, Category="Input|Enhanced")
    TObjectPtr<UInputAction> IA_Rotate;

private:
	// Core camera parameters (orbit-style)
	UPROPERTY(EditAnywhere, Category="Iso Camera|Core")
	float YawDeg = 45.0f;

	UPROPERTY(EditAnywhere, Category="Iso Camera|Core")
	float PitchDeg = -35.0f;

	// The point the camera orbits/looks at. Default: this pawn's actor location.
	UPROPERTY(EditAnywhere, Category="Iso Camera|Core")
	FVector FocusOffset = FVector::ZeroVector;

	// Zoom via distance (stable)
	UPROPERTY(EditAnywhere, Category="Iso Camera|Zoom")
	float OrbitRadius = 3000.0f;

	UPROPERTY(EditAnywhere, Category="Iso Camera|Zoom")
	float OrbitRadiusMin = 800.0f;

	UPROPERTY(EditAnywhere, Category="Iso Camera|Zoom")
	float OrbitRadiusMax = 8000.0f;

	// Height above focus (can be constant or scale with zoom)
	UPROPERTY(EditAnywhere, Category="Iso Camera|Zoom")
	float Height = 1400.0f;

	UPROPERTY(EditAnywhere, Category="Iso Camera|Zoom")
	bool bScaleHeightWithZoom = true;

	UPROPERTY(EditAnywhere, Category="Iso Camera|Zoom", meta=(EditCondition="bScaleHeightWithZoom"))
	float HeightMin = 600.0f;

	UPROPERTY(EditAnywhere, Category="Iso Camera|Zoom", meta=(EditCondition="bScaleHeightWithZoom"))
	float HeightMax = 2600.0f;

	// Input feel
	UPROPERTY(EditAnywhere, Category="Iso Camera|Input")
	float ZoomSpeed = 800.0f; // units per wheel axis step

	UPROPERTY(EditAnywhere, Category="Iso Camera|Input")
	float PanSpeed = 2000.0f; // units per second

	UPROPERTY(EditAnywhere, Category="Iso Camera|Input")
	float RotateSpeedDegPerSec = 90.0f;

	// Keep FOV stable (do not use FOV as zoom)
	UPROPERTY(EditAnywhere, Category="Iso Camera|Camera")
	float FixedFOV = 60.0f;

	// Optional debug override
	UPROPERTY(EditAnywhere, Category="Iso Camera|Debug")
	bool bUseDebugCamera = false;

	UPROPERTY(EditAnywhere, Category="Iso Camera|Debug", meta=(EditCondition="bUseDebugCamera"))
	FVector DebugLocation = FVector(-800.0f, 0.0f, 800.0f);

	UPROPERTY(EditAnywhere, Category="Iso Camera|Debug", meta=(EditCondition="bUseDebugCamera"))
	float DebugPitch = -45.0f;

	UPROPERTY(EditAnywhere, Category="Iso Camera|Debug", meta=(EditCondition="bUseDebugCamera"))
	float DebugYaw = 45.0f;

	UPROPERTY(EditAnywhere, Category="Iso Camera|Debug", meta=(EditCondition="bUseDebugCamera"))
	float DebugFOV = 60.0f;

private:
	// Input handlers (legacy input system)
	void ZoomCamera(float AxisValue);
	void PanForward(float AxisValue);
	void PanRight(float AxisValue);
	void RotateYaw(float AxisValue);
    // Enhanced Input handlers
    void HandleZoom(const FInputActionValue& Value);
    void HandlePan(const FInputActionValue& Value);
	void HandlePanForward(const FInputActionValue& Value);
	void HandlePanRight(const FInputActionValue& Value);
    void HandleRotate(const FInputActionValue& Value);

	// Update camera based on current parameters
	void UpdateCameraTransform();

	// Helpers
	FVector GetFocusPoint() const;
	void ApplyHeightScalingFromRadius();
};

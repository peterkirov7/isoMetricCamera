// Fill out your copyright notice in the Description page of Project Settings.

#include "IsoCameraPawn.h"

#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h" // not used; safe to remove
#include "Components/InputComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Components/PrimitiveComponent.h"

AIsoCameraPawn::AIsoCameraPawn()
{
	PrimaryActorTick.bCanEverTick = true;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(Root);

	// Stable defaults
	Camera->SetProjectionMode(ECameraProjectionMode::Perspective);
	Camera->SetFieldOfView(FixedFOV);

}

void AIsoCameraPawn::BeginPlay()
{
	Super::BeginPlay();
    if (APlayerController* PC = GetWorld()->GetFirstPlayerController())
    {
        if (ULocalPlayer* LP = PC->GetLocalPlayer())
        {
            if (UEnhancedInputLocalPlayerSubsystem* Subsystem =
                LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
            {
                Subsystem->AddMappingContext(CameraMappingContext, 0);
            }
        }
    }
	// Ensure radius/height are in valid bounds at runtime
	OrbitRadius = FMath::Clamp(OrbitRadius, OrbitRadiusMin, OrbitRadiusMax);
	ApplyHeightScalingFromRadius();

	// Apply camera once on start
	UpdateCameraTransform();
}

void AIsoCameraPawn::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	// If you want, you can keep this on every tick; it's cheap.
	// If you prefer only on change, you can remove this and call UpdateCameraTransform()
	// from input handlers only.
	UpdateCameraTransform();
}

void AIsoCameraPawn::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	check(PlayerInputComponent);

	UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent);
	if (!EIC)
	{
		UE_LOG(LogTemp, Error, TEXT("IsoCameraPawn: InputComponent is not EnhancedInputComponent."));
		return;
	}

	// These UPROPERTY pointers must be set on the pawn (or BP child):
	// CameraMappingContext, IA_Zoom, IA_Pan, IA_Rotate
	check(IA_Zoom);
	check(IA_PanForward);
	check(IA_PanRight);

	EIC->BindAction(IA_PanForward, ETriggerEvent::Triggered, this, &AIsoCameraPawn::HandlePanForward);
	EIC->BindAction(IA_PanRight,   ETriggerEvent::Triggered, this, &AIsoCameraPawn::HandlePanRight);
	EIC->BindAction(IA_Zoom, ETriggerEvent::Triggered, this, &AIsoCameraPawn::HandleZoom);

	if (IA_Rotate)
	{
		EIC->BindAction(IA_Rotate, ETriggerEvent::Triggered, this, &AIsoCameraPawn::HandleRotate);
	}
}


FVector AIsoCameraPawn::GetFocusPoint() const
{
	if (FollowTarget)
	{
		FVector Origin, Extents;
		FollowTarget->GetActorBounds(true, Origin, Extents);

		const FVector Pivot = FollowTarget->GetActorLocation();

		if (bAutoCenterFollowTarget)
		{
			// Aim relative to pivot using bounds height (Extents.Z is half-height)
			// Alpha 0.0 -> pivot, Alpha 1.0 -> pivot + full height (2*Extents.Z)
			const float FullHeight = 2.0f * Extents.Z;
			FVector Aim = Pivot;
			Aim.Z += FullHeight * FollowAimHeightAlpha;

			return Aim + FollowOffset;
		}

		return Pivot + FollowOffset;
	}

	return GetActorLocation() + FollowOffset;
}

void AIsoCameraPawn::ApplyHeightScalingFromRadius()
{
	if (!bScaleHeightWithZoom)
	{
		// Height stays exactly as set
		return;
	}

	const float Denom = FMath::Max(KINDA_SMALL_NUMBER, (OrbitRadiusMax - OrbitRadiusMin));
	const float Alpha = (OrbitRadius - OrbitRadiusMin) / Denom;
	Height = FMath::Lerp(HeightMin, HeightMax, FMath::Clamp(Alpha, 0.0f, 1.0f));
}

void AIsoCameraPawn::ZoomCamera(float AxisValue)
{
	if (FMath::IsNearlyZero(AxisValue))
	{
		return;
	}

	OrbitRadius = FMath::Clamp(
		OrbitRadius - AxisValue * ZoomSpeed,
		OrbitRadiusMin,
		OrbitRadiusMax
	);

	ApplyHeightScalingFromRadius();

	// If you remove Tick updates, keep this:
	// UpdateCameraTransform();
}

void AIsoCameraPawn::PanForward(float AxisValue)
{
	if (FMath::IsNearlyZero(AxisValue))
	{
		return;
	}

	// Pan in world X/Y relative to camera yaw (ignoring pitch)
	const FRotator YawOnly(0.0f, YawDeg, 0.0f);
	const FVector Forward = YawOnly.Vector(); // X/Y forward

	AddActorWorldOffset(Forward * (AxisValue * PanSpeed * GetWorld()->GetDeltaSeconds()), true);
}

void AIsoCameraPawn::PanRight(float AxisValue)
{
	if (FMath::IsNearlyZero(AxisValue))
	{
		return;
	}

	const FRotator YawOnly(0.0f, YawDeg, 0.0f);
	const FVector Right = FRotationMatrix(YawOnly).GetUnitAxis(EAxis::Y);

	AddActorWorldOffset(Right * (AxisValue * PanSpeed * GetWorld()->GetDeltaSeconds()), true);
}

void AIsoCameraPawn::RotateYaw(float AxisValue)
{
	if (FMath::IsNearlyZero(AxisValue))
	{
		return;
	}

	YawDeg += AxisValue * RotateSpeedDegPerSec * GetWorld()->GetDeltaSeconds();

	// Keep yaw bounded
	YawDeg = FMath::Fmod(YawDeg, 360.0f);
	if (YawDeg < 0.0f) YawDeg += 360.0f;
}

void AIsoCameraPawn::UpdateCameraTransform()
{
	if (!Camera)
	{
		return;
	}

	if (bUseDebugCamera)
	{
		Camera->SetWorldLocation(DebugLocation);
		Camera->SetWorldRotation(FRotator(DebugPitch, DebugYaw, 0.0f));
		Camera->SetFieldOfView(DebugFOV);
		return;
	}

	const FVector Focus = GetFocusPoint();

	// Desired camera rotation
	const FRotator CamRot(PitchDeg, YawDeg, 0.0f);

	// Forward vector points where camera looks; place camera behind it by OrbitRadius.
	const FVector Forward = CamRot.Vector();
	const FVector CamLoc = Focus - (Forward * OrbitRadius);


	Camera->SetWorldLocation(CamLoc);
	Camera->SetWorldRotation(CamRot);

	// Keep FOV stable
	Camera->SetFieldOfView(FixedFOV);
}

void AIsoCameraPawn::HandleZoom(const FInputActionValue& Value)
{
	const float Axis = Value.Get<float>();
	if (FMath::IsNearlyZero(Axis)) return;

	OrbitRadius = FMath::Clamp(
		OrbitRadius - Axis * ZoomSpeed,
		OrbitRadiusMin,
		OrbitRadiusMax
	);

	ApplyHeightScalingFromRadius();
}

void AIsoCameraPawn::HandlePanForward(const FInputActionValue& Value)
{
    const float Axis = Value.Get<float>();
    if (FMath::IsNearlyZero(Axis)) return;

    const float DT = GetWorld()->GetDeltaSeconds();
    const FRotator YawOnly(0.f, YawDeg, 0.f);
    const FVector Forward = YawOnly.Vector();

    AddActorWorldOffset(Forward * Axis * PanSpeed * DT, true);
}

void AIsoCameraPawn::HandlePanRight(const FInputActionValue& Value)
{
    const float Axis = Value.Get<float>();
    if (FMath::IsNearlyZero(Axis)) return;

    const float DT = GetWorld()->GetDeltaSeconds();
    const FRotator YawOnly(0.f, YawDeg, 0.f);
    const FVector Right = FRotationMatrix(YawOnly).GetUnitAxis(EAxis::Y);

    AddActorWorldOffset(Right * Axis * PanSpeed * DT, true);
}


void AIsoCameraPawn::HandleRotate(const FInputActionValue& Value)
{
	const float Axis = Value.Get<float>();
	if (FMath::IsNearlyZero(Axis)) return;

	YawDeg += Axis * RotateSpeedDegPerSec * GetWorld()->GetDeltaSeconds();
}
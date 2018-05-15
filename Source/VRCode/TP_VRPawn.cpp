/*
* Author: Skyler Clark (@sclark39)
* Website: http://skylerclark.com
* License: MIT License
*/

#include "VRCode.h"
#include "VRPawn.h"
#include "VRHand.h"
#include "IXRTrackingSystem.h"
#include "Runtime/HeadMountedDisplay/Public/IHeadMountedDisplay.h"
#include "Kismet/KismetMathLibrary.h"
#include "HeadMountedDisplayFunctionLibrary.h"

// Sets default values
AVRPawn::AVRPawn()
{
 	// Set this pawn to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	VROrigin = CreateDefaultSubobject<USceneComponent>(TEXT("VROrigin"));
	VROrigin->SetupAttachment(RootComponent);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(VROrigin);

	LeftHand = CreateDefaultSubobject<UChildActorComponent>(TEXT("LeftHand"));
	LeftHand->SetupAttachment(VROrigin);

	RightHand = CreateDefaultSubobject<UChildActorComponent>(TEXT("RightHand"));
	RightHand->SetupAttachment(VROrigin);

	// Create movement component
	Movement = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Movement"));
	Movement->UpdatedComponent = VROrigin;

	AutoPossessPlayer = EAutoReceiveInput::Player0;
}

// Called when the game starts or when spawned
void AVRPawn::BeginPlay()
{
	Super::BeginPlay();

	// Setup Player Height for Various Platforms
	IHeadMountedDisplay *hmd = GEngine->XRSystem->GetHMDDevice();
	if ( hmd )
	{
		UHeadMountedDisplayFunctionLibrary::SetTrackingOrigin(EHMDTrackingOrigin::Floor);
	}

}

void AVRPawn::FinishTeleport( AVRHand *Current, const FVector &TeleportPosition, const FRotator &TeleportRotator )
{
	Current->DisableTeleporter();

	// Move the player
	TeleportTo( TeleportPosition, GetActorRotation(), false, false );

	// Fade back in
	APlayerCameraManager *PlayerCamera = UGameplayStatics::GetPlayerCameraManager( GetWorld(), 0 );
	PlayerCamera->StartCameraFade( 1, 0, FadeInDuration, TeleportFadeColor, false, true );

	// All done.
	IsTeleporting = false;
}

void AVRPawn::ExecuteTeleport( AVRHand *Current )
{
	if (IsTeleporting)
	{
		return;
	}

	// 	if ( !Current->HasValidTeleportLocation )
	// 	{
	// 		Current->DisableTeleporter();
	// 		return;
	// 	}

	FVector TeleportPosition;
	FRotator TeleportRotator;

	Current->GetTeleportDestination( TeleportPosition, TeleportRotator );

	// We're doing this!
	IsTeleporting = true;

	// Fade out screen
	APlayerCameraManager *PlayerCamera = UGameplayStatics::GetPlayerCameraManager( GetWorld(), 0 );
	PlayerCamera->StartCameraFade( 0, 1, FadeOutDuration, TeleportFadeColor, false, true );
	
	// Wait for Fade to complete before continuing the teleport
	//FTimerHandle TimerHandle;
	//FTimerDelegate TimerDelegate;
	//TimerDelegate.BindUFunction( this, FName( TEXT( "FinishTeleport" ) ), Current, TeleportPosition, TeleportRotator );
	//GetWorldTimerManager().SetTimer( TimerHandle, TimerDelegate, FadeOutDuration, false );

	FTimerDelegate TimerCallback;
	TimerCallback.BindLambda([this, Current, TeleportPosition]
	{
		Current->DisableTeleporter();

		// Move the player
		TeleportTo(TeleportPosition, GetActorRotation(), false, false);

		// Fade back in
		APlayerCameraManager *PlayerCamera = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0);
		PlayerCamera->StartCameraFade(1, 0, FadeInDuration, TeleportFadeColor, false, true);

		// All done.
		IsTeleporting = false;
	});

	FTimerHandle Handle;
	GetWorldTimerManager().SetTimer(Handle, TimerCallback, FadeOutDuration, false);
}


void AVRPawn::HandleButtonStyleTeleportActivation( UChildActorComponent *Hand, EInputEvent KeyEvent )
{
	if (MovementScheme != EMovementScheme::LocomotionAndTeleport)
	{
		return;
	}

	AVRHand *Current = Cast<AVRHand>( Hand->GetChildActor() );
	AVRHand *Other = Cast<AVRHand>( ( Hand == LeftHand ? RightHand : LeftHand )->GetChildActor() );

	if ( KeyEvent == IE_Pressed )
	{
		if (Current)
		{
			Current->TeleportRotator = GetActorRotation();
			Current->ActivateTeleporter();
		}
		if (Other)
		{
			Other->DisableTeleporter();
		}
	}
	else
	{
		if (Current && Current->IsTeleporterActive)
		{
			ExecuteTeleport(Current);
		}
	}
}

void AVRPawn::HandleLeftStick()
{
	AVRHand *Left = Cast<AVRHand>(LeftHand->GetChildActor());
	float fwd = InputComponent->GetAxisValue(TEXT("ThumbLeft_Fwd"));
	float side = InputComponent->GetAxisValue(TEXT("ThumbLeft_Side"));

	if (MovementScheme == EMovementScheme::TeleportOnly)
	{
		const float ThumbDeadzoneSq = ThumbDeadzone * ThumbDeadzone;
		if (fwd > 0 && fwd*fwd > ThumbDeadzoneSq)
		{
			if (Left)
			{
				Left->TeleportRotator = GetActorRotation();
				Left->ActivateTeleporter();
			}
		}
		else
		{
			if (Left && Left->IsTeleporterActive)
			{
				ExecuteTeleport(Left);
			}
		}
	}
	else if (fwd != 0 || side != 0)
	{
		FVector DevicePosition;
		FRotator DeviceRotation;
		UHeadMountedDisplayFunctionLibrary::GetOrientationAndPosition(DeviceRotation, DevicePosition);

		DeviceRotation = DeviceRotation + GetActorRotation();

		// fwd
		if (fwd != 0)
		{
			auto vector = UKismetMathLibrary::GetForwardVector(DeviceRotation);
			vector.Z = 0;
			AddMovementInput(vector.GetSafeNormal(), fwd);
		}

		// strafe
		if (side != 0)
		{
			auto vector = UKismetMathLibrary::GetRightVector(DeviceRotation);
			vector.Z = 0;
			AddMovementInput(vector.GetSafeNormal(), side);
		}
	}
}

void AVRPawn::HandleRightStick()
{
	float rotation = InputComponent->GetAxisValue(TEXT("ThumbRight_Side"));
	

	if (RotationScheme == ERotationScheme::Free)
	{
		if (rotation != 0)
		{
			FRotator actorRotation = GetActorRotation();
			actorRotation.Yaw += rotation * 45.0f * GetWorld()->GetDeltaSeconds();
			SetActorRotation(actorRotation);
		}
	}
	else
	{
		if (!justRotated)
		{
			if (rotation*rotation > ThumbDeadzone*ThumbDeadzone)
			{
				justRotated = true;
				
				auto sign = (0.0f < rotation) - (rotation < 0.0f);
				FRotator actorRotation = GetActorRotation();
				actorRotation.Yaw += sign * 45.0f;
				SetActorRotation(actorRotation);
			}
		}
		else
		{
			if (rotation*rotation < ThumbMinDeadzone*ThumbMinDeadzone)
			{
				justRotated = false;
			}
		}
	}
}

// Called every frame
void AVRPawn::Tick( float DeltaTime )
{
	Super::Tick( DeltaTime );

	if ( InputComponent )
	{
		HandleLeftStick();
		HandleRightStick();
	}

}

void AVRPawn::HandleGrip( UChildActorComponent *Hand, EInputEvent KeyEvent )
{
	AVRHand *Current = Cast<AVRHand>( Hand->GetChildActor() );
	if ( Current )
	{
		if (KeyEvent == IE_Pressed)
		{
			Current->GrabActor();
		}
		else // released
		{
			Current->ReleaseActor();
		}
	}
}

void AVRPawn::BindInputActionUFunction( class UInputComponent* PlayerInputComponent, FName ActionName, EInputEvent KeyEvent, FName FuncName, UChildActorComponent *Hand )
{
	FInputActionBinding InputActionBinding( ActionName, KeyEvent );

	FInputActionHandlerSignature InputActionHandler;
	InputActionHandler.BindUFunction( this, FuncName, Hand, KeyEvent );

	InputActionBinding.ActionDelegate = InputActionHandler;
	PlayerInputComponent->AddActionBinding( InputActionBinding );
}

// Called to bind functionality to input
void AVRPawn::SetupPlayerInputComponent( class UInputComponent* PlayerInputComponent )
{
	Super::SetupPlayerInputComponent( PlayerInputComponent );

	BindInputActionUFunction( PlayerInputComponent, TEXT( "TeleportLeft" ), IE_Pressed, TEXT( "HandleButtonStyleTeleportActivation" ), LeftHand );
	BindInputActionUFunction( PlayerInputComponent, TEXT( "TeleportLeft" ), IE_Released, TEXT( "HandleButtonStyleTeleportActivation" ), LeftHand );
	BindInputActionUFunction( PlayerInputComponent, TEXT( "TeleportRight" ), IE_Pressed, TEXT( "HandleButtonStyleTeleportActivation" ), RightHand );
	BindInputActionUFunction( PlayerInputComponent, TEXT( "TeleportRight" ), IE_Released, TEXT( "HandleButtonStyleTeleportActivation" ), RightHand );

	BindInputActionUFunction( PlayerInputComponent, TEXT( "GripLeft" ), IE_Pressed, TEXT( "HandleGrip" ), LeftHand );
	BindInputActionUFunction( PlayerInputComponent, TEXT( "GripLeft" ), IE_Released, TEXT( "HandleGrip" ), LeftHand );
	BindInputActionUFunction( PlayerInputComponent, TEXT( "GrabLeft" ), IE_Pressed, TEXT( "HandleGrip" ), LeftHand );
	BindInputActionUFunction( PlayerInputComponent, TEXT( "GrabLeft" ), IE_Released, TEXT( "HandleGrip" ), LeftHand );

	BindInputActionUFunction( PlayerInputComponent, TEXT( "GripRight" ), IE_Pressed, TEXT( "HandleGrip" ), RightHand );
	BindInputActionUFunction( PlayerInputComponent, TEXT( "GripRight" ), IE_Released, TEXT( "HandleGrip" ), RightHand );
	BindInputActionUFunction( PlayerInputComponent, TEXT( "GrabRight" ), IE_Pressed, TEXT( "HandleGrip" ), RightHand );
	BindInputActionUFunction( PlayerInputComponent, TEXT( "GrabRight" ), IE_Released, TEXT( "HandleGrip" ), RightHand );

	PlayerInputComponent->BindAxis( TEXT( "ThumbLeft_Fwd" ) );
	PlayerInputComponent->BindAxis( TEXT( "ThumbRight_Fwd" ) );

	PlayerInputComponent->BindAxis( TEXT( "ThumbLeft_Side" ) );
	PlayerInputComponent->BindAxis( TEXT( "ThumbRight_Side" ) );
}

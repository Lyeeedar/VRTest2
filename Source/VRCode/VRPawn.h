/*
* Author: Skyler Clark (@sclark39)
* Website: http://skylerclark.com
* License: MIT License
*/

#pragma once

#include "GameFramework/Pawn.h"

#include "VRPawn.generated.h"

UENUM(BlueprintType)
enum class EMovementScheme : uint8
{
	TeleportOnly,
	LocomotionAndTeleport
};

UENUM(BlueprintType)
enum class ERotationScheme : uint8
{
	Snap,
	Free
};

UCLASS()
class VRCODE_API AVRPawn : public APawn
{
	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Code Components", meta = (AllowPrivateAccess = "true"))
	class USceneComponent *VROrigin;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Code Components", meta = (AllowPrivateAccess = "true"))
	class UCameraComponent *Camera;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Code Components", meta = (AllowPrivateAccess = "true"))
	class UChildActorComponent *LeftHand;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Code Components", meta = (AllowPrivateAccess = "true"))
	class UChildActorComponent *RightHand;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Code Components", meta = (AllowPrivateAccess = "true"))
	class UPawnMovementComponent *Movement;

	void HandleRightStick();
	void HandleLeftStick();

public:

	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Code Constants" )
	float FadeInDuration = 0.1;

	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Code Constants" )
	float FadeOutDuration = 0.1;

	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Code Constants" )
	FLinearColor TeleportFadeColor = FLinearColor::Black;

	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Code Constants" )
	float ThumbDeadzone = 0.7;

	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Code Constants" )
	float ThumbMinDeadzone = 0.2;

	UPROPERTY( EditAnywhere, BlueprintReadWrite, Category = "Code Constants" )
	float DefaultPlayerHeight = 180;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Code Constants")
	EMovementScheme MovementScheme = EMovementScheme::LocomotionAndTeleport;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Code Constants")
	ERotationScheme RotationScheme = ERotationScheme::Snap;

	bool IsTeleporting;

	bool justRotated;

	// Sets default values for this pawn's properties
	AVRPawn();

	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
	
	// Called every frame
	virtual void Tick( float DeltaSeconds ) override;

	// Called to bind functionality to input
	virtual void SetupPlayerInputComponent( class UInputComponent* InputComponent ) override;
	
	UFUNCTION()
	void HandleButtonStyleTeleportActivation( class UChildActorComponent *Hand, EInputEvent KeyEvent );

	UFUNCTION()
	void HandleGrip( class UChildActorComponent *Hand, EInputEvent KeyEvent );

	UFUNCTION()
	void HandleTrigger( class UChildActorComponent *Hand, EInputEvent KeyEvent );

	void BindInputActionUFunction( class UInputComponent* PlayerInputComponent, FName ActionName, EInputEvent KeyEvent, FName FuncName, class UChildActorComponent *Hand );

	UFUNCTION()
	void FinishTeleport( class AVRHand *Current, const FVector &TeleportPosition, const FRotator &TeleportRotator );

	void ExecuteTeleport( class AVRHand *Current );

	
	
};

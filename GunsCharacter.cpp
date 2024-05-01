#include "GunsCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundCue.h"
#include "Engine/SkeletalMeshSocket.h"
#include "DrawDebugHelpers.h"
#include "Particles/ParticleSystemComponent.h"

// Sets default values
AGunsCharacter::AGunsCharacter() :
	// Base Case rates for turning/looking up
	BaseTurnRate(45.f),
	BaseLookUpRate(45.f),
	//Turn rates for aiming/not aiming
	HipTurnRate(90.f),
	HipLookUpRate(90.f),
	AimingTurnRate(20.f),
	AimingLookUpRate(20.f),
	//Mouse look Sensitivity scale facters
	MouseHipTurnRate(1.f),
	MouseHipLookUpRate(1.f),
	MouseAimingTurnRate(0.2f),
	MouseAimingLookUpRate(0.2f),
	//True when aiming the weapon
	bAiming(false),
	//Camera field of view values
	CameraDefaultFOV(0.f), //set in BeginPlay
	CameraZoomedFOV(30.f),
	CameraCurrentFOV(0.f),
	ZoomInterpSpeed(20.5)

{
 	// Set this character to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;

	// Create a camera boom (pulls in toward the character if there is a collision
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 180.f; // The camera follows at this distance behind the character
	CameraBoom->bUsePawnControlRotation = true; // Rotate the arm based on the controller
	CameraBoom->TargetOffset= FVector(0.f, 50.f, 70.f);

	// Create a follow camera
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName); // Attach to end of CameraBoom
	FollowCamera->bUsePawnControlRotation = false; // Camera does not rotate relitive to arm

	// Don't rotate when the controller rotates. Let the controller only affect the camera
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = true;
	bUseControllerRotationRoll = false;

	// Configure character movement
	GetCharacterMovement()->bOrientRotationToMovement = false; // character moves in the direction of input
	GetCharacterMovement()->RotationRate = FRotator(0.f, 540.f, 0.f); // ...at this rotation rate
	GetCharacterMovement()->JumpZVelocity = 600.f;
	GetCharacterMovement()->AirControl = 0.2f;



}

// Called when the game starts or when spawned
void AGunsCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (FollowCamera)
	{
		CameraDefaultFOV = GetFollowCamera()->FieldOfView;
		CameraDefaultFOV = CameraDefaultFOV;
	}
	


	
}

void AGunsCharacter::MoveForward(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		//find out which way is forward
		const FRotator Rotation{ Controller->GetControlRotation() };
		const FRotator YawRotation{ 0, Rotation.Yaw, 0 };

		const FVector Direction{ FRotationMatrix{YawRotation}.GetUnitAxis(EAxis::X) };
		AddMovementInput(Direction, Value);
	}
}

void AGunsCharacter::MoveRight(float Value)
{
	if ((Controller != nullptr) && (Value != 0.0f))
	{
		//find out which way is forward
		const FRotator Rotation{ Controller->GetControlRotation() };
		const FRotator YawRotation{ 0, Rotation.Yaw, 0 };

		const FVector Direction{ FRotationMatrix{YawRotation}.GetUnitAxis(EAxis::Y) };
		AddMovementInput(Direction, Value);
	}
}

void AGunsCharacter::TurnAtRate(float Rate)
{
	AddControllerYawInput(Rate * BaseTurnRate * GetWorld()->GetDeltaSeconds()); // deg/sec * sec/frame 
}

void AGunsCharacter::LookUpAtRate(float Rate)
{
	AddControllerPitchInput(Rate * BaseLookUpRate * GetWorld()->GetDeltaSeconds()); // deg/sec * sec/frame 
}

void AGunsCharacter::Turn(float Value)
{
	float TurnScaleFactor{};
	if (bAiming)
	{
		TurnScaleFactor = MouseAimingTurnRate;
	}
	else
	{
		TurnScaleFactor = MouseHipTurnRate;
	}
	AddControllerYawInput(Value * TurnScaleFactor);
}

void AGunsCharacter::LookUp(float Value)
{
	float LookUpScaleFactor{};
	if (bAiming)
	{
		LookUpScaleFactor = MouseAimingLookUpRate;
	}
	else
	{
		LookUpScaleFactor = MouseHipLookUpRate;
	}
	AddControllerPitchInput(Value * LookUpScaleFactor);
}


void AGunsCharacter::FireWeapon() 
{
	if (FireSound)
	{
		UGameplayStatics::PlaySound2D(this, FireSound);
	}
	const USkeletalMeshSocket* BarrellSocket = GetMesh()->GetSocketByName("BarrellSocket");
	if (BarrellSocket)
	{
		const FTransform SocketTransform = BarrellSocket->GetSocketTransform(GetMesh());

		if (MuzzleFlash)
		{
			UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), MuzzleFlash, SocketTransform);
		}

		FVector BeamEnd;
		bool bBeamEnd = GetBeamEndLocation(SocketTransform.GetLocation(), BeamEnd);

		if (bBeamEnd)
		{
			if (ImpactParticles)
			{
				UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticles, BeamEnd);
			}

			UParticleSystemComponent* Beam = UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), BeamParticles, SocketTransform);

			if (Beam)
			{
				Beam->SetVectorParameter(FName("Target"), BeamEnd);
			}

		}
	}
	UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance();
	if (AnimInstance && HipFireMontage)
	{
		AnimInstance->Montage_Play(HipFireMontage);
		AnimInstance->Montage_JumpToSection(FName("StartFire"));
	}
}

bool AGunsCharacter::GetBeamEndLocation(const FVector& MuzzleSocketLocation, FVector& OutBeamLocation)
{
	//Get current Size of the viewport
	FVector2D ViewportSize;
	if (GEngine && GEngine->GameViewport)
	{
		GEngine->GameViewport->GetViewportSize(ViewportSize);
	}
	
	//Get screen space location of crosshairs 
	FVector2D CrosshairLocation(ViewportSize.X / 2.f, ViewportSize.Y / 2.f);
	CrosshairLocation.Y -= 50.f;
	FVector CrosshairWorldPosition;
	FVector CrosshairWorldDirection;


	//Get world position and direction of crosshairs
	bool bScreenToWorld = UGameplayStatics::DeprojectScreenToWorld(UGameplayStatics::GetPlayerController(this, 0),
		CrosshairLocation,
		CrosshairWorldPosition,
		CrosshairWorldDirection);

	if (bScreenToWorld) //Was deprojection successful?
	{
		FHitResult ScreenTraceHit;
		const FVector Start{ CrosshairWorldPosition };
		const FVector End{ CrosshairWorldPosition + CrosshairWorldDirection * 50'00.f };

		//Set beam end point to line trace end point
		OutBeamLocation = End;

		//Trace outword from crosshairs world location
		GetWorld()->LineTraceSingleByChannel(ScreenTraceHit, Start, End, ECollisionChannel::ECC_Visibility);
		if (ScreenTraceHit.bBlockingHit) //Was there a trace hit
		{
			//Beam end point is now trace hit location
			OutBeamLocation = ScreenTraceHit.Location;

		}

		// perform a second trace, this time from the gun barrell
		FHitResult WeaponTraceHit;
		const FVector WeaponTraceStart{ MuzzleSocketLocation };
		const FVector WeaponTraceEnd{ OutBeamLocation };
		GetWorld()->LineTraceSingleByChannel(WeaponTraceHit, WeaponTraceStart, WeaponTraceEnd, ECollisionChannel::ECC_Visibility);
		if (WeaponTraceHit.bBlockingHit)//object between barrel and BeamEndPoint?
		{
			OutBeamLocation = WeaponTraceHit.Location;
		}
		return true;
	}
	
	return false;
}

void AGunsCharacter::AimingButtonPressed()
{
	bAiming = true;
	//GetFollowCamera()->SetFieldOfView(CameraZoomedFOV);
}

void AGunsCharacter::AimingButtonReleased()
{
	bAiming = false;
	//GetFollowCamera()->SetFieldOfView(CameraDefaultFOV);
}

void AGunsCharacter::CameraInterpZoom(float DeltaTime)
{
	//Set current camera field of view
	if (bAiming)
	{
		//Interpolat to zoomed FOV
		CameraCurrentFOV = FMath::FInterpTo(CameraCurrentFOV, CameraZoomedFOV, DeltaTime, ZoomInterpSpeed);
		//GetFollowCamera()->SetFieldOfView(CameraCurrentFOV);
	}
	else
	{
		//Interpolate to default FOV
		CameraCurrentFOV = FMath::FInterpTo(CameraCurrentFOV, CameraDefaultFOV, DeltaTime, ZoomInterpSpeed);
		//GetFollowCamera()->SetFieldOfView(CameraCurrentFOV);

	}
	GetFollowCamera()->SetFieldOfView(CameraCurrentFOV);
}

void AGunsCharacter::SetZoomLookRates()
{
	if (bAiming)
	{
		BaseTurnRate = AimingTurnRate;
		BaseTurnRate = AimingLookUpRate;
	}
	else
	{
		BaseTurnRate = HipTurnRate;
		BaseLookUpRate = HipLookUpRate;
	}
}

// Called every frame
void AGunsCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	//Handle Interpolation for the Zoom when aiming
	CameraInterpZoom(DeltaTime);

	//Change lood secnsitivity based on aiming
	SetZoomLookRates();



		/*
		**********THIS IS THE UNFACTORED CODE PRIOR PUTTING IT IN THE PRIOR PUTTING IT IN THE AGunsCharacter::CameraInterpZoom(DeltaTime) FUNCTION*************
		//Set current camera field of view
		if (bAiming)
		{
			//Interpolat to zoomed FOV
			CameraCurrentFOV = FMath::FInterpTo(CameraCurrentFOV, CameraZoomedFOV, DeltaTime, ZoomInterpSpeed);
			//GetFollowCamera()->SetFieldOfView(CameraCurrentFOV);
		}
		else
		{
			//Interpolate to default FOV
			CameraCurrentFOV = FMath::FInterpTo(CameraCurrentFOV, CameraDefaultFOV, DeltaTime, ZoomInterpSpeed);
			//GetFollowCamera()->SetFieldOfView(CameraCurrentFOV);

		}
		GetFollowCamera()->SetFieldOfView(CameraCurrentFOV);
		********************************************************************************************************************************************************
		*/
}

// Called to bind functionality to input
void AGunsCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	check(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &AGunsCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &AGunsCharacter::MoveRight);
	PlayerInputComponent->BindAxis("TurnRate", this, &AGunsCharacter::TurnAtRate);
	PlayerInputComponent->BindAxis("LookUpRate", this, &AGunsCharacter::LookUpAtRate);
	PlayerInputComponent->BindAxis("Turn", this, &AGunsCharacter::Turn);	// Mouse X
	PlayerInputComponent->BindAxis("LookUp", this, &AGunsCharacter::LookUp); // Mouse Y

	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &AGunsCharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &AGunsCharacter::StopJumping);

	PlayerInputComponent->BindAction("FireButton", IE_Pressed, this, &AGunsCharacter::FireWeapon);

	PlayerInputComponent->BindAction("AimingButton", IE_Pressed, this, &AGunsCharacter::AimingButtonPressed);
	PlayerInputComponent->BindAction("AimingButton", IE_Released, this, &AGunsCharacter::AimingButtonReleased);






}


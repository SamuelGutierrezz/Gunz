#include "GunsAnimInstance.h"
#include "GunsCharacter.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"


void UGunsAnimInstance::UpdateAnimationProperties(float DeltaTime)
{
	if (GunsCharacter == nullptr)
	{
		GunsCharacter = Cast<AGunsCharacter>(TryGetPawnOwner());
	}
	if (GunsCharacter)
	{
		//Get the lateral speed of the character from velocity
		FVector Velocity{ GunsCharacter->GetVelocity() };
		Velocity.Z = 0;
		Speed = Velocity.Size();

		//is the character in the air?
		bIsInAir = GunsCharacter->GetCharacterMovement()->IsFalling();

		//is the character accelerating?
		if (GunsCharacter->GetCharacterMovement()->GetCurrentAcceleration().Size() > 0.f)
		{
			bIsAccelerating = true;
		}
		else
		{
			bIsAccelerating = false;
		}

		FRotator AimRotation = GunsCharacter->GetBaseAimRotation();
		FRotator MovementRotation = UKismetMathLibrary::MakeRotFromX(GunsCharacter->GetVelocity());
		MovementOffsetYaw = UKismetMathLibrary::NormalizedDeltaRotator(MovementRotation, AimRotation).Yaw;
		
		if (GunsCharacter->GetVelocity().Size() > 0.f)
		{
			LastMovementOffsetYaw = MovementOffsetYaw;
		}

		bAiming = GunsCharacter->GetAiming();
		
		//FString RotationMessage = FString::Printf(TEXT("Base Aim Rotation: %f"), AimRotation.Yaw);
		//FString MovementRotationMessage = FString::Printf(TEXT("Movement Rotation: %f"), MovementRotation.Yaw);  THESE FOUR WILL DEBUG YOUR YAW REFERENCES
		//FString OffsetMessage = FString::Printf(TEXT("Movement Offset Yaw: %f"), MovementOffsetYaw);
		
		/*if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(1, 0.f, FColor::White, OffsetMessage);
		}
		*/
	}
}

void UGunsAnimInstance::NativeInitializeAnimation()
{
	GunsCharacter = Cast<AGunsCharacter>(TryGetPawnOwner());
}

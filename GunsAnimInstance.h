#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "GunsAnimInstance.generated.h"

/**
 * 
 */
UCLASS()
class GUNZ_API UGunsAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable)
	void UpdateAnimationProperties(float DeltaTime);

	virtual void NativeInitializeAnimation() override;

protected:
	UPROPERTY(VisibleAnywhere, BluePrintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	class AGunsCharacter* GunsCharacter;

	// the speed of the character
	UPROPERTY(VisibleAnywhere, BluePrintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float Speed;

	// whether or not the character is in the air
	UPROPERTY(VisibleAnywhere, BluePrintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	bool bIsInAir;

	// whether or not the character is moving
	UPROPERTY(VisibleAnywhere, BluePrintReadOnly, Category = Movement, meta = (AllowPrivateAccess = "true"))
	bool bIsAccelerating;

	//Offset yaw used for strafing
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement, meta = (AllowPrivateAccess = "true"))
	float MovementOffsetYaw;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Movement, meta = (AllowPrivateAccess = "true"))
	//Offset Yaw the frame before we stopped moving
	float LastMovementOffsetYaw;
	
};

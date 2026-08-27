#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "BowAnimInstance.generated.h"

UCLASS(Blueprintable)
class VIALACTEA_API UBowAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient, BlueprintReadOnly, Category = "Bow")
	bool bIsDrawing = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Bow")
	float DrawAlpha = 0.f;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Bow")
	bool bHasPullTarget = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Bow")
	FVector PullTargetLocationCS = FVector::ZeroVector;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Bow")
	FVector PullTargetLocationWS = FVector::ZeroVector;

	UFUNCTION(BlueprintCallable, Category = "Bow")
	void SetBowAnimationData(bool bInIsDrawing, float InDrawAlpha, bool bInHasPullTarget, const FVector& InPullTargetLocationCS, const FVector& InPullTargetLocationWS);
};

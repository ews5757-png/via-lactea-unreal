#include "Weapon/BowAnimInstance.h"

void UBowAnimInstance::SetBowAnimationData(bool bInIsDrawing, float InDrawAlpha, bool bInHasPullTarget, const FVector& InPullTargetLocationCS, const FVector& InPullTargetLocationWS)
{
	bIsDrawing = bInIsDrawing;
	DrawAlpha = InDrawAlpha;
	bHasPullTarget = bInHasPullTarget;
	PullTargetLocationCS = InPullTargetLocationCS;
	PullTargetLocationWS = InPullTargetLocationWS;
}

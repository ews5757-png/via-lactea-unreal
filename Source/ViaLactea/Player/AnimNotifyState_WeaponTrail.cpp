#include "Player/AnimNotifyState_WeaponTrail.h"

#include "Components/SkeletalMeshComponent.h"
#include "Player/MainCharacterBase.h"

void UAnimNotifyState_WeaponTrail::NotifyBegin(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	float TotalDuration,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyBegin(MeshComp, Animation, TotalDuration, EventReference);

	if (!MeshComp)
	{
		return;
	}

	if (AMainCharacterBase* Character = Cast<AMainCharacterBase>(MeshComp->GetOwner()))
	{
		Character->SetWeaponTrailState(Target, true);
	}
}

void UAnimNotifyState_WeaponTrail::NotifyEnd(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::NotifyEnd(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}

	if (AMainCharacterBase* Character = Cast<AMainCharacterBase>(MeshComp->GetOwner()))
	{
		Character->SetWeaponTrailState(Target, false);
	}
}

FString UAnimNotifyState_WeaponTrail::GetNotifyName_Implementation() const
{
	switch (Target)
	{
	case EWeaponTarget::Left:
		return TEXT("Weapon Trail Left");
	case EWeaponTarget::Both:
		return TEXT("Weapon Trail Both");
	case EWeaponTarget::Right:
	default:
		return TEXT("Weapon Trail");
	}
}

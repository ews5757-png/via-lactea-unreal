#include "Player/AnimNotifyState_CharacterCollision.h"

#include "Components/SkeletalMeshComponent.h"
#include "Player/MainCharacterBase.h"

void UAnimNotifyState_CharacterCollision::NotifyBegin(
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
		Character->HandleAnimAction(EEquipMentHandleAction::EnableCollision, Target);
	}
}

void UAnimNotifyState_CharacterCollision::NotifyEnd(
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
		Character->HandleAnimAction(EEquipMentHandleAction::DisableCollision, Target);
	}
}

FString UAnimNotifyState_CharacterCollision::GetNotifyName_Implementation() const
{
	switch (Target)
	{
	case EWeaponTarget::Right:
		return TEXT("Character Collision Right");
	case EWeaponTarget::Left:
		return TEXT("Character Collision Left");
	case EWeaponTarget::Both:
	default:
		return TEXT("Character Collision");
	}
}

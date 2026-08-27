// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/AnimNotifyWeaponAction.h"

#include "Components/SkeletalMeshComponent.h"
#include "Player/MainCharacterBase.h"

void UAnimNotifyWeaponAction::Notify(
	USkeletalMeshComponent* MeshComp,
	UAnimSequenceBase* Animation,
	const FAnimNotifyEventReference& EventReference)
{
	Super::Notify(MeshComp, Animation, EventReference);

	if (!MeshComp)
	{
		return;
	}

	if (AMainCharacterBase* Character = Cast<AMainCharacterBase>(MeshComp->GetOwner()))
	{
		Character->HandleAnimAction(Action, Target);
		if (Action == EEquipMentHandleAction::DisableCollision)
		{
			Character->HandleAnimAction(EEquipMentHandleAction::ResetHitActors, Target);
		}
	}
}

FString UAnimNotifyWeaponAction::GetNotifyName_Implementation() const
{
	return TEXT("Weapon Action");
}

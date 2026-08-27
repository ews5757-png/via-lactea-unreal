// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/AnimNotifyState_WeaponIK.h"

#include "Components/SkeletalMeshComponent.h"
#include "Player/MainCharacterBase.h"

void UAnimNotifyState_WeaponIK::NotifyBegin(
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
		Character->SetWeaponIKState(Target, true);
	}
}

void UAnimNotifyState_WeaponIK::NotifyEnd(
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
		Character->SetWeaponIKState(Target, false);
	}
}

FString UAnimNotifyState_WeaponIK::GetNotifyName_Implementation() const
{
	return TEXT("Weapon IK");
}

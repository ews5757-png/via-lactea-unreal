// Fill out your copyright notice in the Description page of Project Settings.

#include "Player/AnimNotifyState_DefenseWindow.h"

#include "Components/SkeletalMeshComponent.h"
#include "Player/MainCharacterBase.h"

void UAnimNotifyState_DefenseWindow::NotifyBegin(
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
		Character->SetDefenseWindowState(WindowType, true);
	}
}

void UAnimNotifyState_DefenseWindow::NotifyEnd(
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
		Character->SetDefenseWindowState(WindowType, false);
	}
}

FString UAnimNotifyState_DefenseWindow::GetNotifyName_Implementation() const
{
	switch (WindowType)
	{
	case EDefenseWindowType::Invincible:
		return TEXT("Defense Invincible");
	case EDefenseWindowType::Parry:
		return TEXT("Defense Parry");
	case EDefenseWindowType::Block:
		return TEXT("Defense Block");
	default:
		return TEXT("Defense Window");
	}
}

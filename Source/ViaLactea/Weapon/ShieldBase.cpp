// Fill out your copyright notice in the Description page of Project Settings.


#include "ShieldBase.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "GameFramework/Character.h"

AShieldBase::AShieldBase()
{
	EquipmentType = EEquipmentSlotType::Shield;
	SecondaryActionStaminaCost = 0.f;
}

bool AShieldBase::CanStartAction(EEquipmentActionType ActionType) const
{
	if (ActionType == EEquipmentActionType::Secondary)
	{
		return OwnerCharacter && !bBlockInputHeld;
	}

	return Super::CanStartAction(ActionType);
}

void AShieldBase::StartPrimaryAction()
{
}

void AShieldBase::StartSecondaryAction()
{
	if (bBlockInputHeld || !OwnerCharacter) return;

	SetGuardInputHeld(true);

	if (SecondaryActionMontage)
	{
		CurrentMontage = SecondaryActionMontage;
		PlayOwnerMontage(SecondaryActionMontage, 1.f, NAME_None, true);
	}
	else
	{
		SetBlockWindowActive(true);
	}
}

void AShieldBase::StartAbilityAction()
{
}

void AShieldBase::StopPrimaryAction()
{
}

void AShieldBase::StopSecondaryAction()
{
	if (!bBlockInputHeld && !bIsBlocking && !bCanParry) return;

	SetGuardInputHeld(false);

	if (BlockReleaseMontage)
	{
		CurrentMontage = BlockReleaseMontage;
		PlayOwnerMontage(BlockReleaseMontage);
	}
	else
	{
		if (SecondaryActionMontage)
		{
			StopOwnerMontage(SecondaryActionMontage, 0.0f);
		}
		CurrentMontage = nullptr;
	}
}

void AShieldBase::StopAbilityAction()
{
}

void AShieldBase::CancelAction()
{
	if (!bBlockInputHeld && !bIsBlocking && !bCanParry) return;

	SetGuardInputHeld(false);

	if (SecondaryActionMontage)
	{
		StopOwnerMontage(SecondaryActionMontage, 0.0f);
	}
	CurrentMontage = nullptr;
}

void AShieldBase::ForceResetState()
{
	UE_LOG(LogTemp, Warning, TEXT("[Shield::ForceResetState] bIsBlocking: %d"), bIsBlocking);
	CancelAction();
}

void AShieldBase::SetGuardInputHeld(bool bHeld)
{
	const bool bWasHeld = bBlockInputHeld;
	bBlockInputHeld = bHeld;

	if (!bBlockInputHeld)
	{
		SetParryWindowActive(false);
		SetBlockWindowActive(false);
	}

	if (bWasHeld != bBlockInputHeld)
	{
		OnGuardChanged.Broadcast(bBlockInputHeld);
	}
}

void AShieldBase::SetParryWindowActive(bool bActive)
{
	if (bCanParry == bActive)
	{
		return;
	}

	bCanParry = bActive;
}

void AShieldBase::SetBlockWindowActive(bool bActive)
{
	// 입력을 계속 누르고 있으면 NotifyState가 끝나도 블로킹 유지
	const bool bNewBlocking = bBlockInputHeld && (bActive || bIsBlocking);
	if (bIsBlocking == bNewBlocking)
	{
		return;
	}

	bIsBlocking = bNewBlocking;
	OnBlockChanged.Broadcast(bIsBlocking);
}

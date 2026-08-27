// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CharacterWeaponBase.h"
#include "CharacterSubEquipmentBase.generated.h"

/**
 *
 */
UCLASS()
class VIALACTEA_API ACharacterSubEquipmentBase : public ACharacterWeaponBase
{
	GENERATED_BODY()

public:
	ACharacterSubEquipmentBase();

	virtual void StartPrimaryAction() override  {}
	virtual void StartSecondaryAction() override  {}
	virtual void StartAbilityAction() override {}
	virtual void StopPrimaryAction() override {}
	virtual void StopSecondaryAction() override {}
	virtual void StopAbilityAction() override {}
};

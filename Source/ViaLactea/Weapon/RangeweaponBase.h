// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CharacterWeaponBase.h"
#include "RangeweaponBase.generated.h"

/**
 * 
 */
UCLASS()
class VIALACTEA_API ARangeweaponBase : public ACharacterWeaponBase
{
	GENERATED_BODY()

public:
	ARangeweaponBase();

	virtual void StartPrimaryAction() override;
	virtual void StartSecondaryAction() override;
	virtual void StartAbilityAction() override;
	virtual void StopPrimaryAction() override;
	virtual void StopSecondaryAction() override;
	virtual void StopAbilityAction() override;
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CharacterSubEquipmentBase.h"
#include "ShieldBase.generated.h"

DECLARE_MULTICAST_DELEGATE_OneParam(FOnBlockChanged, bool);

/**
 *
 */
UCLASS()
class VIALACTEA_API AShieldBase : public ACharacterSubEquipmentBase
{
	GENERATED_BODY()

public:
	AShieldBase();

public:
	virtual bool CanStartAction(EEquipmentActionType ActionType) const override;
	virtual void StartPrimaryAction() override;
	virtual void StartSecondaryAction() override;
	virtual void StartAbilityAction() override;
	virtual void StopPrimaryAction() override;
	virtual void StopSecondaryAction() override;
	virtual void StopAbilityAction() override;
	virtual void CancelAction() override;
	virtual void ForceResetState() override;

	void SetGuardInputHeld(bool bHeld);
	void SetParryWindowActive(bool bActive);
	void SetBlockWindowActive(bool bActive);
	bool IsBlockInputHeld() const { return bBlockInputHeld; }

	// 막기 해제 시 재생할 몽타주
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Shield|Animation")
	class UAnimMontage* BlockReleaseMontage = nullptr;

	// 막기 상태 여부
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Shield|State")
	bool bIsBlocking = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Shield|State")
	bool bBlockInputHeld = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Shield|State")
	bool bCanParry = false;

	FOnBlockChanged OnGuardChanged;
	FOnBlockChanged OnBlockChanged;
};

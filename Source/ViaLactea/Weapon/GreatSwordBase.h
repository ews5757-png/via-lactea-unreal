#pragma once

#include "CoreMinimal.h"
#include "MeleeWeaponBase.h"
#include "GreatSwordBase.generated.h"

class UAnimMontage;

enum class EGreatSwordChargePhase : uint8
{
	None,
	Starting,
	Looping,
	LevelUp,
	Attacking,
	Canceling
};

UCLASS(Blueprintable)
class VIALACTEA_API AGreatSwordBase : public AMeleeWeaponBase
{
	GENERATED_BODY()

public:
	AGreatSwordBase();

	virtual void Tick(float DeltaTime) override;
	virtual void StartAbilityAction() override;
	virtual void StopAbilityAction() override;
	virtual float GetStaminaCostForAction(EEquipmentActionType ActionType) const override;
	virtual bool CanStartAction(EEquipmentActionType ActionType) const override;
	virtual void ForceResetState() override;
	virtual void OnNaturalMontageEnd() override;
	virtual void OnInterruptedMontageEnd() override;
	virtual bool IsAbilityMontage(const UAnimMontage* Montage) const override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|GreatSword|Ability")
	TArray<UAnimMontage*> ChargeAttackMontages;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|GreatSword|Ability", meta = (ClampMin = "0.0"))
	TArray<float> ChargeAttackDamageMultipliers = { 1.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|GreatSword|Sound", meta = (ClampMin = "0.01"))
	TArray<float> ChargeAttackSoundPitchMultipliers = { 1.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|GreatSword|Sound", meta = (ClampMin = "0.0"))
	TArray<float> ChargeAttackSoundVolumeMultipliers = { 1.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|GreatSword|Ability")
	UAnimMontage* ChargeStartMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|GreatSword|Ability")
	UAnimMontage* ChargeLoopMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|GreatSword|Ability")
	UAnimMontage* ChargeCancelMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|GreatSword|Ability")
	UAnimMontage* ChargeLevelUpMontage = nullptr;

	// Each entry is the additional hold duration before the next stage while looping.
	// Defaults: stage 1 immediately on loop enter, +2s to stage 2, +3s to stage 3.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|GreatSword|Ability")
	TArray<float> ChargeStageDurations = { 2.f, 3.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|GreatSword|Ability", meta = (ClampMin = "0.0"))
	float ChargeLoopStaminaDrainPerSecond = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|GreatSword|Ability", meta = (ClampMin = "0.0"))
	float ChargeReleaseStaminaCost = 15.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Weapon|GreatSword|Ability")
	bool bIsChargingAbility = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Weapon|GreatSword|Ability")
	int32 CurrentChargeLevel = 0;

	virtual float GetDamageMultiplierForMontage(const UAnimMontage* Montage) const override;
	virtual float GetSoundVolumeMultiplierForMontage(const UAnimMontage* Montage) const override;
	virtual float GetSoundPitchMultiplierForMontage(const UAnimMontage* Montage) const override;

private:
	void AdvanceChargeStage();
	void ScheduleNextChargeStage();
	void PlayChargeLoopMontage();
	void PlayChargeStageUpMontage();
	void PlayChargeAttackMontage();
	void PlayChargeCancelMontage();
	void PlayAbilityMontage(UAnimMontage* Montage, EGreatSwordChargePhase NewPhase, bool bFreezeAtEnd = false);
	void HandleChargeMontageBlendOut(UAnimMontage* Montage, bool bInterrupted);
	void ResetChargeState();
	int32 GetAttackMontageIndexForCurrentChargeLevel() const;

	FTimerHandle ChargeStageTimerHandle;
	double ChargeStartTime = 0.0;
	int32 NextChargeStageIndex = 0;
	bool bIsAbilityInputHeld = false;
	EGreatSwordChargePhase ChargePhase = EGreatSwordChargePhase::None;
};

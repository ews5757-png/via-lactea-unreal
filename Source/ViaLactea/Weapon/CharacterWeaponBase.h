// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CharacterEquipmentBase.h"
#include "CharacterWeaponBase.generated.h"



UCLASS()
class VIALACTEA_API ACharacterWeaponBase : public ACharacterEquipmentBase
{
	GENERATED_BODY()

public:
	ACharacterWeaponBase();
public:

	FORCEINLINE EWeaponAnimType GetWeaponAnimType() const { return WeaponType; }

	virtual void OnEquip(ACharacter* NewOwner) override;
	virtual void OnRep_Owner() override;
	virtual bool OnUnequip() override;
	virtual void CompleteUnequip() override;

	UFUNCTION()
	virtual void OnMontageStarted(UAnimMontage* Montage);

	virtual bool CanStartAction(EEquipmentActionType ActionType) const override;
	virtual bool ShouldConsumeStaminaForAction(EEquipmentActionType ActionType) const override;
	virtual float GetStaminaCostForAction(EEquipmentActionType ActionType) const override;

	virtual void StartPrimaryAction() override;

	virtual void StartSecondaryAction() override;

	virtual void StartAbilityAction() override;

	virtual bool GetLeftHandIKWorldTransform(FTransform& OutTransform) const;

	virtual bool GetRightHandIKWorldTransform(FTransform& OutTransform) const;

	UFUNCTION(BlueprintCallable, Category = "Equipment|Weapon|Trail")
	void SetWeaponTrailEnabled(bool bEnabled);

	UFUNCTION(BlueprintPure, Category = "Equipment|Weapon|Trail")
	bool IsWeaponTrailEnabled() const { return WeaponTrailRequestCount > 0; }

	virtual void StopPrimaryAction() override;

	virtual void StopSecondaryAction() override {}

	virtual void StopAbilityAction() override {}

	virtual bool IsAbilityMontage(const class UAnimMontage* Montage) const;

	// 이동으로 인한 강제 중단 → 근접 무기는 공격 캔슬
	virtual void CancelAction() override;

	// 원인 불문 상태 강제 초기화 (피격, 구르기, 사망 등)
	virtual void ForceResetState();

	// 몽타주 자연 종료 시 호출 - 자식 클래스에서 후처리 가능
	virtual void OnNaturalMontageEnd() {}

	// 몽타주가 인터럽트로 종료될 때 호출 - 자식 클래스에서 정리 가능
	virtual void OnInterruptedMontageEnd() {}

public:

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|Animation")
	TArray<class UAnimMontage*> basicAttackMontages;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|Animation")
	class UAnimMontage* SecondaryActionMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|Animation")
	class UAnimMontage* AbilityActionMontage = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|State")
	int32 ComboIndex = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|State")
	class UAnimMontage* CurrentMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|State", meta = (AllowPrivateAccess = "true", DisplayName = "Weapon Anim Type"))
	EWeaponAnimType WeaponType = EWeaponAnimType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|Combat", meta = (ClampMin = "0.0", DisplayName = "Weapon Damage"))
	float Damage = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|Combat", meta = (ClampMin = "0.0"))
	TArray<float> PrimaryAttackDamageMultipliers = { 1.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|Combat", meta = (ClampMin = "0.0"))
	float SecondaryActionDamageMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|Combat", meta = (ClampMin = "0.0"))
	float AbilityActionDamageMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|Cost", meta = (ClampMin = "0.0"))
	TArray<float> PrimaryActionStaminaCosts = { 10.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|Cost", meta = (ClampMin = "0.0"))
	float SecondaryActionStaminaCost = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|Cost", meta = (ClampMin = "0.0"))
	float AbilityActionStaminaCost = 20.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Weapon|Combat")
	float CurrentDamageMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|Combat")
	TSubclassOf<class UDamageType> DamageTypeClass;

	UFUNCTION(BlueprintPure, Category = "Equipment|Weapon|Combat")
	float GetCurrentActionDamage() const;

	UFUNCTION(BlueprintPure, Category = "Equipment|Weapon|Combat")
	float GetCurrentDamageMultiplier() const { return CurrentDamageMultiplier; }

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Equipment|Weapon|HitStop", meta = (ClampMin = "0.0"))
	float HitStopDuration = 0.05f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Equipment|Weapon|HitStop", meta = (ClampMin = "0.0"))
	float HitStopPlayRate = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|Sound")
	TObjectPtr<class USoundBase> SwingSoundCue = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|Sound")
	TObjectPtr<class USoundBase> HitSoundCue = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|Sound")
	TObjectPtr<class USoundBase> EquipSoundCue = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|Sound")
	TObjectPtr<class USoundBase> UnequipSoundCue = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|Sound")
	class USoundAttenuation* SoundAttenuationSettings = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|Sound", meta = (ClampMin = "0.0"))
	float DefaultSoundVolumeMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|Sound", meta = (ClampMin = "0.0"))
	TArray<float> PrimaryAttackSoundVolumeMultipliers = { 1.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|Sound", meta = (ClampMin = "0.0"))
	float SecondaryActionSoundVolumeMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|Sound", meta = (ClampMin = "0.0"))
	float AbilityActionSoundVolumeMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|Sound", meta = (ClampMin = "0.01"))
	TArray<float> PrimaryAttackSoundPitchMultipliers = { 1.f };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|Sound", meta = (ClampMin = "0.01"))
	float SecondaryActionSoundPitchMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|Sound", meta = (ClampMin = "0.01"))
	float AbilityActionSoundPitchMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|Sound", meta = (ClampMin = "0.01", ClampMax = "1.0"))
	float SwingHitStopPitchMultiplier = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|Effect")
	TObjectPtr<class UNiagaraSystem> HitEffectSystem = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|Effect", meta = (ClampMin = "0.0"))
	float HitEffectScale = 0.1f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|Effect")
	FName HitEffectCustomColorParameterName = TEXT("User.CustomColor");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|Effect", meta = (DisplayName = "Default Hit Effect Custom Color"))
	FLinearColor HitEffectCustomColor = FLinearColor::White;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|Effect", meta = (DisplayName = "Ability Hit Effect Custom Color"))
	FLinearColor AbilityHitEffectCustomColor = FLinearColor::White;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Weapon|Ability VFX")
	class UNiagaraComponent* AbilityNiagaraComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|Ability VFX")
	class UNiagaraSystem* AbilityNiagaraSystem = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|Ability VFX")
	FName AbilityNiagaraAttachSocket = NAME_None;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Weapon|Trail")
	class UNiagaraComponent* WeaponTrailComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|Trail")
	class UNiagaraSystem* WeaponTrailSystem = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Weapon|Trail")
	class UParticleSystemComponent* WeaponCascadeTrailComponent;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|Trail")
	class UParticleSystem* WeaponCascadeTrailSystem = nullptr;

protected:
	virtual void BeginPlay() override;
	virtual void HandlePlaySwingSound() override;
	virtual void HandlePlayEquipSound() override;
	virtual void HandlePlayUnequipSound() override;
	bool ResolveWeaponIKSocketWorldTransform(FName SocketName, FTransform& OutTransform) const;
	bool HasWeaponTrailSocket(FName SocketName) const;
	void RefreshWeaponTrailAssets();
	void RefreshWeaponTrailAttachments();
	void TriggerOwnerHitStop() const;
	void TriggerTargetHitStop(AActor* Target);
	void ApplyDamageToActor(AActor* OtherActor, class UPrimitiveComponent* HitComponent = nullptr, class UPrimitiveComponent* SourceComponent = nullptr);
	class UAudioComponent* SpawnSoundAtWeapon(class USoundBase* Sound, float VolumeMultiplier, float PitchMultiplier) const;
	void PlayHitSound(AActor* HitActor);
	void PlayHitEffect(AActor* HitActor, class UPrimitiveComponent* HitComponent = nullptr, class UPrimitiveComponent* SourceComponent = nullptr);
	void ResolveHitEffectTransform(AActor* HitActor, class UPrimitiveComponent* HitComponent, class UPrimitiveComponent* SourceComponent, FVector& OutLocation, FRotator& OutRotation, FVector& OutNormal) const;
	virtual float GetSoundVolumeMultiplierForMontage(const class UAnimMontage* Montage) const;
	virtual float GetSoundPitchMultiplierForMontage(const class UAnimMontage* Montage) const;
	void DragSwingSoundsForHitStop();
	void SetActiveSwingSoundPitchMultiplier(float PitchMultiplier);
	void CleanupActiveSwingSounds();
	void SetAbilityNiagaraEnabled(bool bEnabled);
	void RefreshAbilityNiagaraAttachment();
	float GetPrimaryAttackDamageMultiplier(int32 AttackIndex) const;
	virtual float GetDamageMultiplierForMontage(const class UAnimMontage* Montage) const;
	void SetCurrentDamageMultiplier(float NewMultiplier);
	void SetCurrentDamageMultiplierForPrimaryAttackIndex(int32 AttackIndex);
	void SetCurrentDamageMultiplierForMontage(const class UAnimMontage* Montage);
	void ResetWeaponTrailState();

	UFUNCTION(BlueprintNativeEvent, Category = "Equipment|Weapon|Trail")
	void HandleWeaponTrailStateChanged(bool bEnabled);
	virtual void HandleWeaponTrailStateChanged_Implementation(bool bEnabled);

	UFUNCTION()
	void OnMontageEnded(UAnimMontage* Montage, bool bInterrupted);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlaySwingSound(class USoundBase* Sound, float VolumeMultiplier, float PitchMultiplier);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayHitSound(class USoundBase* Sound, FVector Location, float VolumeMultiplier, float PitchMultiplier);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastPlayHitEffect(class UNiagaraSystem* Effect, FVector Location, FRotator Rotation, float Scale, FVector Normal, FVector AngleDirection, FLinearColor CustomColor);

	UFUNCTION(NetMulticast, Unreliable)
	void MulticastSetSwingSoundPitchMultiplier(float PitchMultiplier);

public:	
	// Called every frame
	virtual void Tick(float DeltaTime) override;

private:
	int32 WeaponTrailRequestCount = 0;
	bool bHasPreviousTrailStartLocation = false;
	FVector PreviousTrailStartLocation = FVector::ZeroVector;
	FVector CurrentTrailStartLocation = FVector::ZeroVector;
	FTimerHandle SwingSoundHitStopTimerHandle;
	TArray<TWeakObjectPtr<class UAudioComponent>> ActiveSwingSoundComponents;
	TArray<float> ActiveSwingSoundBasePitchMultipliers;
};

#pragma once

#include "CoreMinimal.h"
#include "Engine/EngineTypes.h"
#include "TimerManager.h"
#include "MeleeWeaponBase.h"
#include "HammerBase.generated.h"

UCLASS(Blueprintable)
class VIALACTEA_API AHammerBase : public AMeleeWeaponBase
{
	GENERATED_BODY()

public:
	AHammerBase();

	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaTime) override;
	virtual void StartAbilityAction() override;
	virtual void StartPrimaryAction() override;
	virtual void OnMontageStarted(UAnimMontage* Montage) override;
	virtual void OnEquip(ACharacter* NewOwner) override;
	virtual bool OnUnequip() override;
	virtual void CompleteUnequip() override;
	virtual bool CanStartAction(EEquipmentActionType ActionType) const override;
	virtual bool ShouldConsumeStaminaForAction(EEquipmentActionType ActionType) const override;
	virtual float GetStaminaCostForAction(EEquipmentActionType ActionType) const override;
	virtual void CancelAction() override;
	virtual void ForceResetState() override;
	virtual void OnNaturalMontageEnd() override;
	virtual void OnInterruptedMontageEnd() override;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Equipment|Weapon|Hammer|Throw", meta = (DisplayName = "Hammer Speed", ClampMin = "0.0"))
	float ThrowSpeed = 1500.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Equipment|Weapon|Hammer|Throw", meta = (ClampMin = "0.0"))
	float RecallBrakeDuration = 0.6f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Equipment|Weapon|Hammer|Throw")
	float ReturnArrivalDistance = 60.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Equipment|Weapon|Hammer|Throw", meta = (ClampMin = "0.0"))
	float MissAutoReturnDelay = 2.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Equipment|Weapon|Hammer|Throw")
	float MeshSpinSpeed = 720.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Equipment|Weapon|Hammer|Throw")
	FRotator ThrowRotationOffset = FRotator::ZeroRotator;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|Hammer|Throw|VFX")
	class UNiagaraSystem* HammerThrowNiagaraSystem = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|Hammer|Throw|VFX")
	FName HammerThrowNiagaraAttachSocket = NAME_None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|Hammer|Throw|VFX")
	class UMaterialInterface* HammerThrowOverlayMaterial = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|Hammer|Throw|Sound")
	class USoundBase* HammerFlightLoopSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|Hammer|Throw|Sound")
	class USoundBase* HammerEffectLoopSound = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|Hammer|Throw|Sound", meta = (ClampMin = "0.0"))
	float HammerFlightLoopVolumeMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|Hammer|Throw|Sound", meta = (ClampMin = "0.01"))
	float HammerFlightLoopPitchMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|Hammer|Throw|Sound", meta = (ClampMin = "0.0"))
	float HammerEffectLoopVolumeMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|Hammer|Throw|Sound", meta = (ClampMin = "0.01"))
	float HammerEffectLoopPitchMultiplier = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|Hammer|Throw|Sound")
	class USoundAttenuation* HammerThrowLoopAttenuationSettings = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|Hammer|Throw|Sound", meta = (ClampMin = "0.0"))
	float HammerThrowLoopFadeInTime = 0.08f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|Hammer|Throw|Sound", meta = (ClampMin = "0.0"))
	float HammerThrowLoopFadeOutTime = 0.12f;

private:
	void SetOwnerCollisionIgnore(bool bIgnore);
	void SyncHammerFlightTrailState();
	void SetHammerThrowVisualsEnabled(bool bEnabled);
	void SetHammerThrowLoopSoundsEnabled(bool bEnabled);
	void UpdateHammerThrowLoopSounds();
	void PlayHammerThrowLoopAudio(class UAudioComponent* AudioComponent, class USoundBase* Sound, float VolumeMultiplier, float PitchMultiplier) const;
	void StopHammerThrowLoopAudio(class UAudioComponent* AudioComponent) const;
	void ThrowHammer(bool bPlayMontage = true);
	void RecallHammer();
	virtual void HandleThrowHammer() override;
	bool TryStickToWorld(const FVector& CurrentLocation, const FVector& NextLocation);
	void UpdateHammerSpin(float TravelSpeed, float DeltaTime, bool bReverseSpin);
	void ApplyHammerTravelRotation(const FVector& TravelDirection);
	void ScheduleMissAutoReturn();
	void ClearMissAutoReturn();
	void HandleMissAutoReturn();
	void MarkHammerHitSomething();
	void BeginHammerReturn();
	void ReattachHammer();

	UFUNCTION(Server, Reliable)
	void ServerStartHammerAbilityAction();

	UFUNCTION(Server, Reliable)
	void ServerHandleHammerThrowNotify();

	UFUNCTION()
	void OnRep_HammerThrown();

	UPROPERTY(ReplicatedUsing = OnRep_HammerThrown)
	bool bHammerThrown = false;

	UPROPERTY(Replicated)
	bool bHammerReturning = false;

	UPROPERTY(Replicated)
	bool bHammerStuck = false;

	UPROPERTY(Replicated)
	FVector ThrowVelocity = FVector::ZeroVector;

	FTimerHandle MissAutoReturnTimerHandle;
	float HammerSpinAngleDegrees = 0.f;
	bool bHitSomethingDuringThrow = false;
	bool bHammerThrowPending = false;
	bool bHammerFlightTrailActive = false;
	ECollisionEnabled::Type SavedMeshCollisionEnabled = ECollisionEnabled::NoCollision;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Weapon|Hammer|Throw|VFX", meta = (AllowPrivateAccess = "true"))
	class UNiagaraComponent* HammerThrowNiagaraComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Weapon|Hammer|Throw|Sound", meta = (AllowPrivateAccess = "true"))
	class UAudioComponent* HammerFlightLoopAudioComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Weapon|Hammer|Throw|Sound", meta = (AllowPrivateAccess = "true"))
	class UAudioComponent* HammerEffectLoopAudioComponent;
};

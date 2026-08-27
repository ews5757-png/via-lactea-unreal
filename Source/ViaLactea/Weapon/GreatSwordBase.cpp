#include "GreatSwordBase.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"
#include "Player/MainCharacterBase.h"
#include "Base/Component/VL_StatComponent.h"

namespace
{
	void LogChargeMontageRuntime(const TCHAR* Context, ACharacter* OwnerCharacter, UAnimMontage* TargetMontage, UAnimMontage* CurrentMontage, EGreatSwordChargePhase Phase)
	{
		UAnimInstance* AnimInstance = OwnerCharacter && OwnerCharacter->GetMesh() ? OwnerCharacter->GetMesh()->GetAnimInstance() : nullptr;
		UAnimMontage* ActiveMontage = AnimInstance ? AnimInstance->GetCurrentActiveMontage() : nullptr;

		const bool bTargetPlaying = AnimInstance && TargetMontage ? AnimInstance->Montage_IsPlaying(TargetMontage) : false;
		const bool bCurrentPlaying = AnimInstance && CurrentMontage ? AnimInstance->Montage_IsPlaying(CurrentMontage) : false;

		const float TargetPosition = AnimInstance && TargetMontage ? AnimInstance->Montage_GetPosition(TargetMontage) : -1.f;
		const float CurrentPosition = AnimInstance && CurrentMontage ? AnimInstance->Montage_GetPosition(CurrentMontage) : -1.f;

		const FName TargetSection = AnimInstance && TargetMontage ? AnimInstance->Montage_GetCurrentSection(TargetMontage) : NAME_None;
		const FName CurrentSection = AnimInstance && CurrentMontage ? AnimInstance->Montage_GetCurrentSection(CurrentMontage) : NAME_None;

		UE_LOG(LogTemp, Warning, TEXT("[GreatSwordCharge] %s | Target=%s | Current=%s | Active=%s | Phase=%d | TargetPlaying=%d | CurrentPlaying=%d | TargetPos=%.3f | CurrentPos=%.3f | TargetSection=%s | CurrentSection=%s | TargetLen=%.3f | CurrentLen=%.3f | TargetAutoBlendOut=%d | CurrentAutoBlendOut=%d | TargetBlendOutTrigger=%.3f | CurrentBlendOutTrigger=%.3f"),
			Context,
			*GetNameSafe(TargetMontage),
			*GetNameSafe(CurrentMontage),
			*GetNameSafe(ActiveMontage),
			static_cast<int32>(Phase),
			bTargetPlaying,
			bCurrentPlaying,
			TargetPosition,
			CurrentPosition,
			*TargetSection.ToString(),
			*CurrentSection.ToString(),
			TargetMontage ? TargetMontage->GetPlayLength() : -1.f,
			CurrentMontage ? CurrentMontage->GetPlayLength() : -1.f,
			TargetMontage ? TargetMontage->bEnableAutoBlendOut : false,
			CurrentMontage ? CurrentMontage->bEnableAutoBlendOut : false,
			TargetMontage ? TargetMontage->BlendOutTriggerTime : -1.f,
			CurrentMontage ? CurrentMontage->BlendOutTriggerTime : -1.f);
	}
}

AGreatSwordBase::AGreatSwordBase()
{
	WeaponType = EWeaponAnimType::TwoHandedMelee;
	EquipmentType = EEquipmentSlotType::TwoHanded;
	Damage = 36.f;
	PrimaryAttackDamageMultipliers = { 1.0f, 1.15f, 1.4f };
	PrimaryAttackSoundPitchMultipliers = { 0.75f, 0.75f, 0.7f };
	SecondaryActionDamageMultiplier = 1.2f;
	AbilityActionDamageMultiplier = 1.0f;
	ChargeAttackDamageMultipliers = { 1.5f, 2.1f, 2.8f };
	ChargeAttackSoundPitchMultipliers = { 0.65f, 0.65f, 0.65f };
	ChargeAttackSoundVolumeMultipliers = { 1.f, 1.f, 1.f };

	WeaponCollisionRadius = 10.f;
	WeaponCollisionHalfHeight = 70.f;
	UpdateWeaponCollisionShape();
}

float AGreatSwordBase::GetStaminaCostForAction(EEquipmentActionType ActionType) const
{
	if (ActionType == EEquipmentActionType::Ability)
	{
		return 0.f;
	}
	return Super::GetStaminaCostForAction(ActionType);
}

void AGreatSwordBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (ChargePhase != EGreatSwordChargePhase::Looping || !bIsChargingAbility || !bIsAbilityInputHeld)
	{
		return;
	}

	AMainCharacterBase* MainChar = Cast<AMainCharacterBase>(OwnerCharacter);
	UVL_StatComponent* StatComp = MainChar ? MainChar->GetStatComponent() : nullptr;
	if (!StatComp)
	{
		return;
	}

	StatComp->ConsumeStaminaTick(ChargeLoopStaminaDrainPerSecond * DeltaTime);

	if (StatComp->GetCurrentStamina() <= 0.f)
	{
		bIsAbilityInputHeld = false;
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(ChargeStageTimerHandle);
		}
		PlayChargeCancelMontage();
	}
}

void AGreatSwordBase::StartAbilityAction()
{
	UE_LOG(LogTemp, Warning, TEXT("[GreatSwordCharge] StartAbilityAction | Owner=%s | bIsCharging=%d | CurrentMontage=%s | Phase=%d"),
		*GetNameSafe(OwnerCharacter),
		bIsChargingAbility,
		*GetNameSafe(CurrentMontage),
		static_cast<int32>(ChargePhase));

	if (!OwnerCharacter || bIsChargingAbility || CurrentMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GreatSwordCharge] StartAbilityAction BLOCKED"));
		return;
	}

	ChargeStageDurations.Sort();

	bIsChargingAbility = true;
	bIsAbilityInputHeld = true;
	CurrentChargeLevel = 0;
	NextChargeStageIndex = 0;
	ChargePhase = EGreatSwordChargePhase::None;
	ChargeStartTime = 0.0;

	UE_LOG(LogTemp, Warning, TEXT("[GreatSwordCharge] ChargeInitialized | ChargeStartTime=%.3f"),
		ChargeStartTime);

	if (ChargeStartMontage)
	{
		PlayAbilityMontage(ChargeStartMontage, EGreatSwordChargePhase::Starting);
		return;
	}

	PlayChargeLoopMontage();
}

void AGreatSwordBase::StopAbilityAction()
{
	UE_LOG(LogTemp, Warning, TEXT("[GreatSwordCharge] StopAbilityAction | bIsCharging=%d | Level=%d | Phase=%d | CurrentMontage=%s"),
		bIsChargingAbility,
		CurrentChargeLevel,
		static_cast<int32>(ChargePhase),
		*GetNameSafe(CurrentMontage));

	if (!bIsChargingAbility || ChargePhase == EGreatSwordChargePhase::Attacking || ChargePhase == EGreatSwordChargePhase::Canceling)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GreatSwordCharge] StopAbilityAction BLOCKED"));
		return;
	}

	bIsAbilityInputHeld = false;
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ChargeStageTimerHandle);
	}

	if (CurrentChargeLevel <= 0)
	{
		PlayChargeCancelMontage();
		return;
	}

	PlayChargeAttackMontage();
}

bool AGreatSwordBase::CanStartAction(EEquipmentActionType ActionType) const
{
	if (ActionType == EEquipmentActionType::Ability)
	{
		return OwnerCharacter && !bIsChargingAbility && !CurrentMontage && (ChargeStartMontage || ChargeLoopMontage);
	}

	return Super::CanStartAction(ActionType);
}

void AGreatSwordBase::ForceResetState()
{
	ResetChargeState();
	Super::ForceResetState();
}

void AGreatSwordBase::OnNaturalMontageEnd()
{
	// 차지 체이닝은 HandleChargeMontageBlendOut에서 BlendOut 시점에 처리됨.
	// 여기 도달하면 BlendOut에서 이미 다음 몽타주가 시작되었거나, 차지가 아닌 경우.
	switch (ChargePhase)
	{
	case EGreatSwordChargePhase::Starting:
	case EGreatSwordChargePhase::LevelUp:
	case EGreatSwordChargePhase::Looping:
		if (!bIsChargingAbility)
		{
			ResetChargeState();
		}
		return;

	case EGreatSwordChargePhase::Attacking:
	case EGreatSwordChargePhase::Canceling:
		ResetChargeState();
		return;

	default:
		Super::OnNaturalMontageEnd();
		return;
	}
}

void AGreatSwordBase::OnInterruptedMontageEnd()
{
	UE_LOG(LogTemp, Warning, TEXT("[GreatSwordCharge] OnInterruptedMontageEnd | Phase=%d | CurrentMontage=%s"),
		static_cast<int32>(ChargePhase),
		*GetNameSafe(CurrentMontage));
	LogChargeMontageRuntime(TEXT("OnInterruptedMontageEnd Runtime"), OwnerCharacter, nullptr, CurrentMontage, ChargePhase);
	Super::OnInterruptedMontageEnd();
	ResetChargeState();
}

void AGreatSwordBase::HandleChargeMontageBlendOut(UAnimMontage* Montage, bool bInterrupted)
{
	UE_LOG(LogTemp, Warning, TEXT("[GreatSwordCharge] HandleChargeMontageBlendOut | Montage=%s | CurrentMontage=%s | Interrupted=%d | Phase=%d | bIsCharging=%d | InputHeld=%d | Level=%d"),
		*GetNameSafe(Montage),
		*GetNameSafe(CurrentMontage),
		bInterrupted,
		static_cast<int32>(ChargePhase),
		bIsChargingAbility,
		bIsAbilityInputHeld,
		CurrentChargeLevel);
	LogChargeMontageRuntime(TEXT("HandleChargeMontageBlendOut Runtime"), OwnerCharacter, Montage, CurrentMontage, ChargePhase);

	if (Montage != CurrentMontage || bInterrupted)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GreatSwordCharge] HandleChargeMontageBlendOut SKIPPED"));
		return;
	}

	switch (ChargePhase)
	{
	case EGreatSwordChargePhase::Starting:
	case EGreatSwordChargePhase::LevelUp:
	case EGreatSwordChargePhase::Looping:
		if (!bIsChargingAbility)
		{
			return; // OnNaturalMontageEnd에서 ResetChargeState 처리
		}

		if (!bIsAbilityInputHeld)
		{
			if (CurrentChargeLevel > 0)
			{
				PlayChargeAttackMontage();
			}
			else
			{
				PlayChargeCancelMontage();
			}
			return;
		}

		// Looping은 같은 몽타주를 다시 재생
		PlayChargeLoopMontage();
		return;

	default:
		return;
	}
}

bool AGreatSwordBase::IsAbilityMontage(const UAnimMontage* Montage) const
{
	if (!Montage)
	{
		return false;
	}

	if (Montage == ChargeStartMontage ||
		Montage == ChargeLoopMontage ||
		Montage == ChargeCancelMontage ||
		Montage == ChargeLevelUpMontage)
	{
		return true;
	}

	return ChargeAttackMontages.Contains(const_cast<UAnimMontage*>(Montage));
}

float AGreatSwordBase::GetDamageMultiplierForMontage(const UAnimMontage* Montage) const
{
	if (!Montage)
	{
		return Super::GetDamageMultiplierForMontage(Montage);
	}

	for (int32 AttackIndex = 0; AttackIndex < ChargeAttackMontages.Num(); ++AttackIndex)
	{
		if (ChargeAttackMontages[AttackIndex] == Montage)
		{
			if (ChargeAttackDamageMultipliers.IsValidIndex(AttackIndex))
			{
				return ChargeAttackDamageMultipliers[AttackIndex];
			}

			if (ChargeAttackDamageMultipliers.Num() > 0)
			{
				return ChargeAttackDamageMultipliers.Last();
			}

			return AbilityActionDamageMultiplier;
		}
	}

	return Super::GetDamageMultiplierForMontage(Montage);
}

float AGreatSwordBase::GetSoundVolumeMultiplierForMontage(const UAnimMontage* Montage) const
{
	if (!Montage)
	{
		return Super::GetSoundVolumeMultiplierForMontage(Montage);
	}

	for (int32 AttackIndex = 0; AttackIndex < ChargeAttackMontages.Num(); ++AttackIndex)
	{
		if (ChargeAttackMontages[AttackIndex] == Montage)
		{
			if (ChargeAttackSoundVolumeMultipliers.IsValidIndex(AttackIndex))
			{
				return ChargeAttackSoundVolumeMultipliers[AttackIndex];
			}

			if (ChargeAttackSoundVolumeMultipliers.Num() > 0)
			{
				return ChargeAttackSoundVolumeMultipliers.Last();
			}

			return DefaultSoundVolumeMultiplier;
		}
	}

	return Super::GetSoundVolumeMultiplierForMontage(Montage);
}

float AGreatSwordBase::GetSoundPitchMultiplierForMontage(const UAnimMontage* Montage) const
{
	if (!Montage)
	{
		return Super::GetSoundPitchMultiplierForMontage(Montage);
	}

	for (int32 AttackIndex = 0; AttackIndex < ChargeAttackMontages.Num(); ++AttackIndex)
	{
		if (ChargeAttackMontages[AttackIndex] == Montage)
		{
			if (ChargeAttackSoundPitchMultipliers.IsValidIndex(AttackIndex))
			{
				return ChargeAttackSoundPitchMultipliers[AttackIndex];
			}

			if (ChargeAttackSoundPitchMultipliers.Num() > 0)
			{
				return ChargeAttackSoundPitchMultipliers.Last();
			}

			return AbilityActionSoundPitchMultiplier;
		}
	}

	return Super::GetSoundPitchMultiplierForMontage(Montage);
}

void AGreatSwordBase::AdvanceChargeStage()
{
	UE_LOG(LogTemp, Warning, TEXT("[GreatSwordCharge] AdvanceChargeStage | bIsCharging=%d | InputHeld=%d | CurrentLevel=%d | NextStageIdx=%d | Phase=%d"),
		bIsChargingAbility,
		bIsAbilityInputHeld,
		CurrentChargeLevel,
		NextChargeStageIndex,
		static_cast<int32>(ChargePhase));

	if (!bIsChargingAbility || !bIsAbilityInputHeld)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GreatSwordCharge] AdvanceChargeStage BLOCKED"));
		return;
	}

	const int32 MaxChargeLevel = FMath::Max(1, ChargeStageDurations.Num() + 1);
	if (CurrentChargeLevel >= MaxChargeLevel)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GreatSwordCharge] AdvanceChargeStage BLOCKED | AlreadyAtMaxLevel=%d"), CurrentChargeLevel);
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(ChargeStageTimerHandle);
		}
		return;
	}

	++CurrentChargeLevel;
	NextChargeStageIndex = CurrentChargeLevel - 1;

	UE_LOG(LogTemp, Warning, TEXT("[GreatSwordCharge] AdvanceChargeStage APPLIED | NewLevel=%d | NewNextStageIdx=%d"),
		CurrentChargeLevel,
		NextChargeStageIndex);

	if (ChargeLevelUpMontage)
	{
		PlayChargeStageUpMontage();
	}

	if (!ChargeLevelUpMontage && ChargePhase != EGreatSwordChargePhase::Attacking && ChargePhase != EGreatSwordChargePhase::Canceling)
	{
		PlayChargeLoopMontage();
	}
}

void AGreatSwordBase::ScheduleNextChargeStage()
{
	UWorld* World = GetWorld();
	if (!World || !bIsChargingAbility || !bIsAbilityInputHeld || !ChargeStageDurations.IsValidIndex(NextChargeStageIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("[GreatSwordCharge] ScheduleNextChargeStage SKIPPED | World=%d | bIsCharging=%d | InputHeld=%d | NextStageIdx=%d | DurationsNum=%d"),
			World != nullptr,
			bIsChargingAbility,
			bIsAbilityInputHeld,
			NextChargeStageIndex,
			ChargeStageDurations.Num());
		return;
	}

	const float Delay = FMath::Max(0.f, ChargeStageDurations[NextChargeStageIndex]);

	UE_LOG(LogTemp, Warning, TEXT("[GreatSwordCharge] ScheduleNextChargeStage | CurrentLevel=%d | NextStageIdx=%d | Delay=%.3f"),
		CurrentChargeLevel,
		NextChargeStageIndex,
		Delay);

	World->GetTimerManager().ClearTimer(ChargeStageTimerHandle);

	World->GetTimerManager().SetTimer(
		ChargeStageTimerHandle,
		this,
		&AGreatSwordBase::AdvanceChargeStage,
		Delay,
		false);
}

void AGreatSwordBase::PlayChargeLoopMontage()
{
	UE_LOG(LogTemp, Warning, TEXT("[GreatSwordCharge] PlayChargeLoopMontage | bIsCharging=%d | InputHeld=%d | LoopMontage=%s | CurrentMontage=%s | Phase=%d"),
		bIsChargingAbility,
		bIsAbilityInputHeld,
		*GetNameSafe(ChargeLoopMontage),
		*GetNameSafe(CurrentMontage),
		static_cast<int32>(ChargePhase));

	if (!bIsChargingAbility || !bIsAbilityInputHeld)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GreatSwordCharge] PlayChargeLoopMontage BLOCKED"));
		return;
	}

	if (!ChargeLoopMontage)
	{
		return;
	}

	if (CurrentChargeLevel <= 0)
	{
		CurrentChargeLevel = 1;
		NextChargeStageIndex = 0;
		ChargeStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;

		UE_LOG(LogTemp, Warning, TEXT("[GreatSwordCharge] Stage1Activated | LoopEnteredAt=%.3f"), ChargeStartTime);
	}
	else
	{
		NextChargeStageIndex = CurrentChargeLevel - 1;
	}

	ScheduleNextChargeStage();
	PlayAbilityMontage(ChargeLoopMontage, EGreatSwordChargePhase::Looping);
}

void AGreatSwordBase::PlayChargeStageUpMontage()
{
	UE_LOG(LogTemp, Warning, TEXT("[GreatSwordCharge] PlayChargeStageUpMontage | LevelUpMontage=%s | CurrentMontage=%s | Phase=%d"),
		*GetNameSafe(ChargeLevelUpMontage),
		*GetNameSafe(CurrentMontage),
		static_cast<int32>(ChargePhase));

	if (!ChargeLevelUpMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GreatSwordCharge] PlayChargeStageUpMontage BLOCKED"));
		return;
	}

	PlayAbilityMontage(ChargeLevelUpMontage, EGreatSwordChargePhase::LevelUp);
}

void AGreatSwordBase::PlayChargeAttackMontage()
{
	const int32 AttackMontageIndex = GetAttackMontageIndexForCurrentChargeLevel();
	UE_LOG(LogTemp, Warning, TEXT("[GreatSwordCharge] PlayChargeAttackMontage | Level=%d | AttackIdx=%d | CurrentMontage=%s | Phase=%d"),
		CurrentChargeLevel,
		AttackMontageIndex,
		*GetNameSafe(CurrentMontage),
		static_cast<int32>(ChargePhase));

	if (!ChargeAttackMontages.IsValidIndex(AttackMontageIndex) || !ChargeAttackMontages[AttackMontageIndex])
	{
		UE_LOG(LogTemp, Warning, TEXT("[GreatSwordCharge] PlayChargeAttackMontage BLOCKED"));
		ResetChargeState();
		return;
	}

	if (ChargeReleaseStaminaCost > KINDA_SMALL_NUMBER)
	{
		AMainCharacterBase* MainChar = Cast<AMainCharacterBase>(OwnerCharacter);
		UVL_StatComponent* StatComp = MainChar ? MainChar->GetStatComponent() : nullptr;
		if (!StatComp || !StatComp->ConsumeStamina(ChargeReleaseStaminaCost))
		{
			UE_LOG(LogTemp, Warning, TEXT("[GreatSwordCharge] PlayChargeAttackMontage CANCELLED - not enough stamina"));
			PlayChargeCancelMontage();
			return;
		}
	}

	PlayAbilityMontage(ChargeAttackMontages[AttackMontageIndex], EGreatSwordChargePhase::Attacking);
}

void AGreatSwordBase::PlayChargeCancelMontage()
{
	UE_LOG(LogTemp, Warning, TEXT("[GreatSwordCharge] PlayChargeCancelMontage | CancelMontage=%s | CurrentMontage=%s | Phase=%d"),
		*GetNameSafe(ChargeCancelMontage),
		*GetNameSafe(CurrentMontage),
		static_cast<int32>(ChargePhase));

	if (!ChargeCancelMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GreatSwordCharge] PlayChargeCancelMontage BLOCKED"));
		ResetChargeState();
		return;
	}

	PlayAbilityMontage(ChargeCancelMontage, EGreatSwordChargePhase::Canceling);
}

void AGreatSwordBase::PlayAbilityMontage(UAnimMontage* Montage, EGreatSwordChargePhase NewPhase, bool bFreezeAtEnd)
{
	UE_LOG(LogTemp, Warning, TEXT("[GreatSwordCharge] PlayAbilityMontage | Montage=%s | NewPhase=%d | FreezeAtEnd=%d | PrevPhase=%d | PrevCurrentMontage=%s"),
		*GetNameSafe(Montage),
		static_cast<int32>(NewPhase),
		bFreezeAtEnd,
		static_cast<int32>(ChargePhase),
		*GetNameSafe(CurrentMontage));

	if (!Montage || !OwnerCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GreatSwordCharge] PlayAbilityMontage BLOCKED"));
		return;
	}

	ChargePhase = NewPhase;
	CurrentMontage = Montage;
	SetCurrentDamageMultiplierForMontage(Montage);
	PlayOwnerMontage(Montage, 1.f, NAME_None, bFreezeAtEnd);
	LogChargeMontageRuntime(TEXT("PlayAbilityMontage AfterPlay"), OwnerCharacter, Montage, CurrentMontage, ChargePhase);

	if (UAnimInstance* AnimInstance = OwnerCharacter->GetMesh() ? OwnerCharacter->GetMesh()->GetAnimInstance() : nullptr)
	{
		FOnMontageBlendingOutStarted BlendOutDelegate;
		BlendOutDelegate.BindUObject(this, &AGreatSwordBase::HandleChargeMontageBlendOut);
		AnimInstance->Montage_SetBlendingOutDelegate(BlendOutDelegate, Montage);

		UE_LOG(LogTemp, Warning, TEXT("[GreatSwordCharge] BlendOutDelegateBound | Montage=%s"), *GetNameSafe(Montage));
		LogChargeMontageRuntime(TEXT("PlayAbilityMontage AfterDelegateBound"), OwnerCharacter, Montage, CurrentMontage, ChargePhase);
	}
}

void AGreatSwordBase::ResetChargeState()
{
	UE_LOG(LogTemp, Warning, TEXT("[GreatSwordCharge] ResetChargeState | Phase=%d | CurrentMontage=%s | Level=%d | NextStageIdx=%d"),
		static_cast<int32>(ChargePhase),
		*GetNameSafe(CurrentMontage),
		CurrentChargeLevel,
		NextChargeStageIndex);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(ChargeStageTimerHandle);
	}

	bIsChargingAbility = false;
	bIsAbilityInputHeld = false;
	CurrentChargeLevel = 0;
	NextChargeStageIndex = 0;
	ChargeStartTime = 0.0;
	ChargePhase = EGreatSwordChargePhase::None;
}

int32 AGreatSwordBase::GetAttackMontageIndexForCurrentChargeLevel() const
{
	if (ChargeAttackMontages.Num() <= 0)
	{
		return INDEX_NONE;
	}

	return FMath::Clamp(CurrentChargeLevel - 1, 0, ChargeAttackMontages.Num() - 1);
}

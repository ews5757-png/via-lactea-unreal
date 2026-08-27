#include "GreatHammerBase.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "Player/MainCharacterBase.h"
#include "Base/Component/VL_StatComponent.h"

AGreatHammerBase::AGreatHammerBase()
{
	WeaponType = EWeaponAnimType::TwoHandedMelee;
	EquipmentType = EEquipmentSlotType::TwoHanded;
	Damage = 40.f;
	PrimaryAttackDamageMultipliers = { 1.0f, 1.25f };
	DefaultSoundVolumeMultiplier = 0.5f;
	PrimaryAttackSoundVolumeMultipliers = { 0.5f, 0.5f };
	PrimaryAttackSoundPitchMultipliers = { 0.8f, 0.8f };
	SecondaryActionDamageMultiplier = 1.2f;
	SecondaryActionSoundVolumeMultiplier = 0.5f;
	AbilityActionDamageMultiplier = 0.35f;
	AbilityActionSoundVolumeMultiplier = 0.5f;
	AbilityActionSoundPitchMultiplier = 0.8f;

	WeaponCollisionRadius = 14.f;
	WeaponCollisionHalfHeight = 60.f;
	UpdateWeaponCollisionShape();
}

float AGreatHammerBase::GetStaminaCostForAction(EEquipmentActionType ActionType) const
{
	if (ActionType == EEquipmentActionType::Ability)
	{
		return 0.f;
	}
	return Super::GetStaminaCostForAction(ActionType);
}

void AGreatHammerBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateWhirlwindInputRotation();
	DrainWhirlwindLoopStamina(DeltaTime);
	CheckTerminalSectionComplete();
}

void AGreatHammerBase::StartAbilityAction()
{
	UE_LOG(LogTemp, Warning, TEXT("[GreatHammerWhirlwind] StartAbilityAction | Owner=%s | Active=%d | CurrentMontage=%s | Phase=%d"),
		*GetNameSafe(OwnerCharacter),
		bIsWhirlwindActive,
		*GetNameSafe(CurrentMontage),
		static_cast<int32>(WhirlwindPhase));

	if (!OwnerCharacter || bIsWhirlwindActive || CurrentMontage || !WhirlwindMontage)
	{
		return;
	}

	if (!BeginWhirlwindAbilityLocal())
	{
		return;
	}

	if (OwnerCharacter->IsLocallyControlled())
	{
		if (HasAuthority())
		{
			MulticastStartWhirlwindAbility();
		}
		else
		{
			ServerStartWhirlwindAbility();
		}
	}
}

void AGreatHammerBase::StopAbilityAction()
{
	UE_LOG(LogTemp, Warning, TEXT("[GreatHammerWhirlwind] StopAbilityAction | Active=%d | Level=%d | Phase=%d | CurrentMontage=%s"),
		bIsWhirlwindActive,
		CurrentWhirlwindLevel,
		static_cast<int32>(WhirlwindPhase),
		*GetNameSafe(CurrentMontage));

	if (!bIsWhirlwindActive ||
		WhirlwindPhase == EGreatHammerWhirlwindPhase::Finishing ||
		WhirlwindPhase == EGreatHammerWhirlwindPhase::Canceling)
	{
		return;
	}

	FName TargetSection = NAME_None;
	EGreatHammerWhirlwindPhase TargetPhase = EGreatHammerWhirlwindPhase::None;
	const float HeldDuration = GetWhirlwindHeldDuration();
	if (!GetWhirlwindStopRoute(HeldDuration, TargetSection, TargetPhase))
	{
		return;
	}

	if (TargetPhase == EGreatHammerWhirlwindPhase::Finishing && WhirlwindFinishStaminaCost > KINDA_SMALL_NUMBER)
	{
		AMainCharacterBase* MainChar = Cast<AMainCharacterBase>(OwnerCharacter);
		UVL_StatComponent* StatComp = MainChar ? MainChar->GetStatComponent() : nullptr;
		if (!StatComp || !StatComp->ConsumeStamina(WhirlwindFinishStaminaCost))
		{
			UE_LOG(LogTemp, Warning, TEXT("[GreatHammerWhirlwind] StopAbilityAction FINISH->CANCEL - not enough stamina"));
			TargetSection = WhirlwindCancelSection;
			TargetPhase = EGreatHammerWhirlwindPhase::Canceling;
		}
	}

	if (!ApplyWhirlwindStopRoute(TargetSection, TargetPhase, HeldDuration))
	{
		return;
	}

	if (OwnerCharacter && OwnerCharacter->IsLocallyControlled())
	{
		if (HasAuthority())
		{
			MulticastStopWhirlwindAbility(TargetSection, TargetPhase, HeldDuration);
		}
		else
		{
			// 스태미너 부족으로 Cancel로 교체된 경우 서버에 0 전달해 강제 캔슬 유도
			const float DurationToSend = (TargetPhase == EGreatHammerWhirlwindPhase::Canceling) ? 0.f : HeldDuration;
			ServerStopWhirlwindAbility(DurationToSend);
		}
	}
}

bool AGreatHammerBase::CanStartAction(EEquipmentActionType ActionType) const
{
	if (ActionType == EEquipmentActionType::Ability)
	{
		return OwnerCharacter && !bIsWhirlwindActive && !CurrentMontage && WhirlwindMontage;
	}

	return Super::CanStartAction(ActionType);
}

void AGreatHammerBase::ForceResetState()
{
	ResetWhirlwindState();
	Super::ForceResetState();
}

void AGreatHammerBase::OnNaturalMontageEnd()
{
	Super::OnNaturalMontageEnd();
	ResetWhirlwindState();
}

void AGreatHammerBase::OnInterruptedMontageEnd()
{
	Super::OnInterruptedMontageEnd();
	ResetWhirlwindState();
}

bool AGreatHammerBase::IsAbilityMontage(const UAnimMontage* Montage) const
{
	return Montage && Montage == WhirlwindMontage;
}

bool AGreatHammerBase::BeginWhirlwindAbilityLocal()
{
	if (!OwnerCharacter || bIsWhirlwindActive || CurrentMontage || !WhirlwindMontage)
	{
		return false;
	}

	bIsWhirlwindActive = true;
	bIsAbilityInputHeld = true;
	CurrentWhirlwindLevel = 0;
	WhirlwindPhase = EGreatHammerWhirlwindPhase::Starting;
	WhirlwindStartTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
	CurrentMontage = WhirlwindMontage;
	SetCurrentDamageMultiplier(AbilityActionDamageMultiplier);

	const float Duration = PlayWhirlwindMontageLocal(WhirlwindStartSection);
	if (Duration <= 0.f)
	{
		CurrentMontage = nullptr;
		ResetWhirlwindState();
		return false;
	}

	SetAbilityNiagaraEnabled(true);
	ConfigureWhirlwindSectionFlow();
	EnterWhirlwindLoopSection();
	return true;
}

float AGreatHammerBase::PlayWhirlwindMontageLocal(FName StartSection) const
{
	UAnimInstance* AnimInstance = GetOwnerAnimInstance();
	if (!AnimInstance || !WhirlwindMontage)
	{
		return 0.f;
	}

	const float Duration = AnimInstance->Montage_Play(WhirlwindMontage, 1.f);
	if (Duration > 0.f && StartSection != NAME_None)
	{
		AnimInstance->Montage_JumpToSection(StartSection, WhirlwindMontage);
	}

	return Duration;
}

void AGreatHammerBase::StopWhirlwindMontageLocal(float BlendOutTime) const
{
	UAnimInstance* AnimInstance = GetOwnerAnimInstance();
	if (!AnimInstance || !WhirlwindMontage)
	{
		return;
	}

	AnimInstance->Montage_Stop(BlendOutTime, WhirlwindMontage);
}

void AGreatHammerBase::ServerStartWhirlwindAbility_Implementation()
{
	BeginWhirlwindAbilityLocal();
	MulticastStartWhirlwindAbility();
}

void AGreatHammerBase::MulticastStartWhirlwindAbility_Implementation()
{
	if (OwnerCharacter && OwnerCharacter->IsLocallyControlled())
	{
		return;
	}

	BeginWhirlwindAbilityLocal();
}

void AGreatHammerBase::ServerStopWhirlwindAbility_Implementation(float ClientHeldDuration)
{
	FName TargetSection = NAME_None;
	EGreatHammerWhirlwindPhase TargetPhase = EGreatHammerWhirlwindPhase::None;
	if (!GetWhirlwindStopRoute(ClientHeldDuration, TargetSection, TargetPhase))
	{
		return;
	}

	if (ApplyWhirlwindStopRoute(TargetSection, TargetPhase, ClientHeldDuration))
	{
		MulticastStopWhirlwindAbility(TargetSection, TargetPhase, ClientHeldDuration);
	}
}

void AGreatHammerBase::MulticastStopWhirlwindAbility_Implementation(FName TargetSection, EGreatHammerWhirlwindPhase NewPhase, float HeldDuration)
{
	if (OwnerCharacter && OwnerCharacter->IsLocallyControlled())
	{
		return;
	}

	ApplyWhirlwindStopRoute(TargetSection, NewPhase, HeldDuration);
}

void AGreatHammerBase::ConfigureWhirlwindSectionFlow() const
{
	SetWhirlwindNextSection(WhirlwindStartSection, WhirlwindLoopSection);
	SetWhirlwindNextSection(WhirlwindLoopSection, WhirlwindLoopSection);
}

void AGreatHammerBase::SetWhirlwindNextSection(FName FromSection, FName ToSection) const
{
	UAnimInstance* AnimInstance = GetOwnerAnimInstance();
	if (!AnimInstance || !WhirlwindMontage || FromSection.IsNone() || ToSection.IsNone())
	{
		return;
	}

	if (WhirlwindMontage->GetSectionIndex(FromSection) == INDEX_NONE ||
		WhirlwindMontage->GetSectionIndex(ToSection) == INDEX_NONE)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GreatHammerWhirlwind] Invalid section link | From=%s | To=%s | Montage=%s"),
			*FromSection.ToString(),
			*ToSection.ToString(),
			*GetNameSafe(WhirlwindMontage));
		return;
	}

	AnimInstance->Montage_SetNextSection(FromSection, ToSection, WhirlwindMontage);
}

void AGreatHammerBase::EnterWhirlwindLoopSection()
{
	if (!bIsWhirlwindActive || !bIsAbilityInputHeld || WhirlwindPhase != EGreatHammerWhirlwindPhase::Starting)
	{
		return;
	}

	WhirlwindPhase = EGreatHammerWhirlwindPhase::Looping;
}

void AGreatHammerBase::RouteWhirlwindToSection(FName TargetSection, EGreatHammerWhirlwindPhase NewPhase)
{
	if (!WhirlwindMontage || TargetSection.IsNone() || WhirlwindMontage->GetSectionIndex(TargetSection) == INDEX_NONE)
	{
		ResetWhirlwindState();
		return;
	}

	WhirlwindPhase = NewPhase;
	ActiveTerminalSection = TargetSection;
	SetWhirlwindNextSection(WhirlwindStartSection, TargetSection);
	SetWhirlwindNextSection(WhirlwindLoopSection, TargetSection);
}

void AGreatHammerBase::CheckTerminalSectionComplete()
{
	if (ActiveTerminalSection.IsNone() || !bIsWhirlwindActive)
	{
		return;
	}

	UAnimInstance* AnimInstance = GetOwnerAnimInstance();
	if (!AnimInstance || !WhirlwindMontage)
	{
		return;
	}

	const FName CurrentSection = AnimInstance->Montage_GetCurrentSection(WhirlwindMontage);
	if (CurrentSection != ActiveTerminalSection)
	{
		return;
	}

	if (GetTimeRemainingInCurrentSection() <= WhirlwindTerminalBlendOutTime)
	{
		Super::OnNaturalMontageEnd();
		StopWhirlwindMontageLocal(WhirlwindTerminalBlendOutTime);
		ResetWhirlwindState();
	}
}

void AGreatHammerBase::UpdateWhirlwindInputRotation()
{
	if (!bRotateWhirlwindToInput ||
		!bIsWhirlwindActive ||
		!bIsAbilityInputHeld ||
		(WhirlwindPhase != EGreatHammerWhirlwindPhase::Starting && WhirlwindPhase != EGreatHammerWhirlwindPhase::Looping))
	{
		return;
	}

	AMainCharacterBase* MainCharacter = Cast<AMainCharacterBase>(OwnerCharacter);
	if (!MainCharacter || !MainCharacter->IsLocallyControlled())
	{
		return;
	}

	MainCharacter->RotateToInput(WhirlwindInputRotationInterpSpeed);
}

void AGreatHammerBase::UpdateWhirlwindLevelFromElapsed()
{
	UpdateWhirlwindLevelFromElapsed(GetWhirlwindHeldDuration());
}

void AGreatHammerBase::UpdateWhirlwindLevelFromElapsed(float ElapsedTime)
{
	if (ElapsedTime <= 0.f)
	{
		return;
	}

	const int32 UpdatedLevel = GetWhirlwindLevelForElapsed(ElapsedTime);

	if (UpdatedLevel > CurrentWhirlwindLevel)
	{
		CurrentWhirlwindLevel = UpdatedLevel;
	}
}

float AGreatHammerBase::GetWhirlwindHeldDuration() const
{
	UWorld* World = GetWorld();
	if (!World || WhirlwindStartTime <= 0.0)
	{
		return 0.f;
	}

	return FMath::Max(0.f, static_cast<float>(World->GetTimeSeconds() - WhirlwindStartTime));
}

int32 AGreatHammerBase::GetWhirlwindLevelForElapsed(float ElapsedTime) const
{
	int32 UpdatedLevel = 0;
	for (int32 StageIndex = 0; StageIndex < WhirlwindFinishSections.Num(); ++StageIndex)
	{
		if (ElapsedTime < GetWhirlwindFinishThresholdTime(StageIndex))
		{
			break;
		}

		UpdatedLevel = StageIndex + 1;
	}

	return UpdatedLevel;
}

bool AGreatHammerBase::GetWhirlwindStopRoute(float HeldDuration, FName& OutTargetSection, EGreatHammerWhirlwindPhase& OutPhase) const
{
	const int32 HeldLevel = GetWhirlwindLevelForElapsed(HeldDuration);
	if (HeldLevel <= 0)
	{
		OutTargetSection = WhirlwindCancelSection;
		OutPhase = EGreatHammerWhirlwindPhase::Canceling;
		return !OutTargetSection.IsNone();
	}

	if (WhirlwindFinishSections.Num() <= 0)
	{
		return false;
	}

	const int32 FinishSectionIndex = FMath::Clamp(HeldLevel - 1, 0, WhirlwindFinishSections.Num() - 1);
	OutTargetSection = WhirlwindFinishSections[FinishSectionIndex];
	OutPhase = EGreatHammerWhirlwindPhase::Finishing;
	return !OutTargetSection.IsNone();
}

bool AGreatHammerBase::ApplyWhirlwindStopRoute(FName TargetSection, EGreatHammerWhirlwindPhase NewPhase, float HeldDuration)
{
	if (!bIsWhirlwindActive ||
		WhirlwindPhase == EGreatHammerWhirlwindPhase::Finishing ||
		WhirlwindPhase == EGreatHammerWhirlwindPhase::Canceling)
	{
		return false;
	}

	if (!WhirlwindMontage || TargetSection.IsNone() || WhirlwindMontage->GetSectionIndex(TargetSection) == INDEX_NONE)
	{
		ResetWhirlwindState();
		return false;
	}

	bIsAbilityInputHeld = false;
	CurrentWhirlwindLevel = GetWhirlwindLevelForElapsed(HeldDuration);

	RouteWhirlwindToSection(TargetSection, NewPhase);
	return true;
}

void AGreatHammerBase::ResetWhirlwindState()
{
	SetAbilityNiagaraEnabled(false);
	bIsWhirlwindActive = false;
	bIsAbilityInputHeld = false;
	CurrentWhirlwindLevel = 0;
	WhirlwindStartTime = 0.0;
	ActiveTerminalSection = NAME_None;
	WhirlwindPhase = EGreatHammerWhirlwindPhase::None;
}

FName AGreatHammerBase::GetFinishSectionNameForCurrentWhirlwindLevel() const
{
	if (WhirlwindFinishSections.Num() <= 0)
	{
		return NAME_None;
	}

	const int32 FinishSectionIndex = FMath::Clamp(CurrentWhirlwindLevel - 1, 0, WhirlwindFinishSections.Num() - 1);
	return WhirlwindFinishSections[FinishSectionIndex];
}

float AGreatHammerBase::GetWhirlwindFinishThresholdTime(int32 FinishSectionIndex) const
{
	return FMath::Max(0.f, WhirlwindCancelDuration) +
		FMath::Max(0.f, WhirlwindFinishStageDuration) * static_cast<float>(FinishSectionIndex + 1);
}

float AGreatHammerBase::GetSectionLength(FName SectionName) const
{
	if (!WhirlwindMontage || SectionName.IsNone())
	{
		return 0.f;
	}

	const int32 SectionIndex = WhirlwindMontage->GetSectionIndex(SectionName);
	if (SectionIndex == INDEX_NONE)
	{
		return 0.f;
	}

	return WhirlwindMontage->GetSectionLength(SectionIndex);
}

float AGreatHammerBase::GetTimeRemainingInCurrentSection() const
{
	UAnimInstance* AnimInstance = GetOwnerAnimInstance();
	if (!AnimInstance || !WhirlwindMontage)
	{
		return 0.f;
	}

	const float MontagePosition = AnimInstance->Montage_GetPosition(WhirlwindMontage);
	const int32 SectionIndex = WhirlwindMontage->GetSectionIndexFromPosition(MontagePosition);
	if (SectionIndex == INDEX_NONE)
	{
		return 0.f;
	}

	float SectionStartTime = 0.f;
	float SectionEndTime = 0.f;
	WhirlwindMontage->GetSectionStartAndEndTime(SectionIndex, SectionStartTime, SectionEndTime);

	return FMath::Max(0.f, SectionEndTime - MontagePosition);
}

UAnimInstance* AGreatHammerBase::GetOwnerAnimInstance() const
{
	return OwnerCharacter && OwnerCharacter->GetMesh() ? OwnerCharacter->GetMesh()->GetAnimInstance() : nullptr;
}

void AGreatHammerBase::DrainWhirlwindLoopStamina(float DeltaTime)
{
	if (!bIsWhirlwindActive || !bIsAbilityInputHeld || WhirlwindPhase != EGreatHammerWhirlwindPhase::Looping)
	{
		return;
	}

	AMainCharacterBase* MainChar = Cast<AMainCharacterBase>(OwnerCharacter);
	UVL_StatComponent* StatComp = MainChar ? MainChar->GetStatComponent() : nullptr;
	if (!StatComp)
	{
		return;
	}

	StatComp->ConsumeStaminaTick(WhirlwindLoopStaminaDrainPerSecond * DeltaTime);

	if (StatComp->GetCurrentStamina() <= 0.f)
	{
		UE_LOG(LogTemp, Warning, TEXT("[GreatHammerWhirlwind] DrainWhirlwindLoopStamina - stamina depleted, force canceling"));
		bIsAbilityInputHeld = false;
		RouteWhirlwindToSection(WhirlwindCancelSection, EGreatHammerWhirlwindPhase::Canceling);

		if (OwnerCharacter && OwnerCharacter->IsLocallyControlled())
		{
			if (HasAuthority())
			{
				MulticastStopWhirlwindAbility(WhirlwindCancelSection, EGreatHammerWhirlwindPhase::Canceling, GetWhirlwindHeldDuration());
			}
			else
			{
				ServerStopWhirlwindAbility(0.f);
			}
		}
	}
}

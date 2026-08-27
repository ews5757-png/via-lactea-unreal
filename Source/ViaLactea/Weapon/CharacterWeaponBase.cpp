// Fill out your copyright notice in the Description page of Project Settings.


#include "CharacterWeaponBase.h"
#include "Animation/AnimInstance.h"
#include "Camera/PlayerCameraManager.h"
#include "Components/AudioComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/EngineTypes.h"
#include "GameFramework/Character.h"
#include "GameFramework/DamageType.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Particles/ParticleSystem.h"
#include "Particles/ParticleSystemComponent.h"
#include "Player/MainCharacterBase.h"
#include "Kismet/GameplayStatics.h"
#include "Base/Character/VL_AICharacterBase.h"
//#include "Monster/Normal/VL_NormalMonster1.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"

namespace
{
	const FName LeftWeaponIKSocketName(TEXT("Weapon_ik_L"));
	const FName RightWeaponIKSocketName(TEXT("Weapon_ik_R"));
	const FName WeaponTrailStartSocketName(TEXT("Trail_Start"));
	const FName WeaponTrailEndSocketName(TEXT("Trail_End"));
	const FName HitEffectScaleParameterName(TEXT("User.Scale"));
	const FName HitEffectAngleParameterName(TEXT("User.Angle"));
	constexpr float DefaultWeaponTrailWidth = 1.f;
}

// Sets default values
ACharacterWeaponBase::ACharacterWeaponBase()
{
	PrimaryActorTick.bCanEverTick = true;

	WeaponTrailComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("WeaponTrailComponent"));
	WeaponTrailComponent->SetupAttachment(SKEquipmentMesh);
	WeaponTrailComponent->SetAutoActivate(false);
	WeaponTrailComponent->bAutoActivate = false;

	WeaponCascadeTrailComponent = CreateDefaultSubobject<UParticleSystemComponent>(TEXT("WeaponCascadeTrailComponent"));
	WeaponCascadeTrailComponent->SetupAttachment(SKEquipmentMesh);
	WeaponCascadeTrailComponent->SetAutoActivate(false);
	WeaponCascadeTrailComponent->bAutoActivate = false;

	AbilityNiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("AbilityNiagaraComponent"));
	AbilityNiagaraComponent->SetupAttachment(SKEquipmentMesh);
	AbilityNiagaraComponent->SetAutoActivate(false);
	AbilityNiagaraComponent->bAutoActivate = false;

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> DefaultHitEffect(
		TEXT("/Game/Character/FX/NS_Big_Hammer_Impact.NS_Big_Hammer_Impact"));
	if (DefaultHitEffect.Succeeded())
	{
		HitEffectSystem = DefaultHitEffect.Object;
	}

	DamageTypeClass = UDamageType::StaticClass();
}

float ACharacterWeaponBase::GetCurrentActionDamage() const
{
	return FMath::Max(0.f, Damage) * FMath::Max(0.f, CurrentDamageMultiplier);
}

void ACharacterWeaponBase::BeginPlay()
{
	Super::BeginPlay();
	RefreshWeaponTrailAssets();
	RefreshWeaponTrailAttachments();
	RefreshAbilityNiagaraAttachment();

	if (WeaponTrailComponent)
	{
		WeaponTrailComponent->DeactivateImmediate();
	}

	if (WeaponCascadeTrailComponent)
	{
		WeaponCascadeTrailComponent->DeactivateImmediate();
		WeaponCascadeTrailComponent->EndTrails();
	}

	if (AbilityNiagaraComponent)
	{
		AbilityNiagaraComponent->DeactivateImmediate();
	}
}

void ACharacterWeaponBase::OnMontageStarted(UAnimMontage* Montage)
{
	if (CurrentMontage && Montage != CurrentMontage)
	{
		ForceResetState();
	}
}

void ACharacterWeaponBase::HandlePlaySwingSound()
{
	if (!HasAuthority())
	{
		return;
	}

	if (SwingSoundCue)
	{
		MulticastPlaySwingSound(
			SwingSoundCue,
			GetSoundVolumeMultiplierForMontage(CurrentMontage),
			GetSoundPitchMultiplierForMontage(CurrentMontage));
	}
}

void ACharacterWeaponBase::HandlePlayEquipSound()
{
	if (!HasAuthority())
	{
		return;
	}

	if (EquipSoundCue)
	{
		MulticastPlaySwingSound(
			EquipSoundCue,
			DefaultSoundVolumeMultiplier,
			1.f);
	}
}

void ACharacterWeaponBase::HandlePlayUnequipSound()
{
	if (!HasAuthority())
	{
		return;
	}

	if (UnequipSoundCue)
	{
		MulticastPlaySwingSound(
			UnequipSoundCue,
			DefaultSoundVolumeMultiplier,
			1.f);
	}
}

bool ACharacterWeaponBase::ResolveWeaponIKSocketWorldTransform(FName SocketName, FTransform& OutTransform) const
{
	OutTransform = FTransform::Identity;

	if (!SKEquipmentMesh)
	{
		return false;
	}

	if (SocketName.IsNone() || !SKEquipmentMesh->DoesSocketExist(SocketName))
	{
		return false;
	}

	OutTransform = SKEquipmentMesh->GetSocketTransform(SocketName, RTS_World);
	OutTransform.NormalizeRotation();
	return true;
}

bool ACharacterWeaponBase::HasWeaponTrailSocket(FName SocketName) const
{
	return SKEquipmentMesh && !SocketName.IsNone() && SKEquipmentMesh->DoesSocketExist(SocketName);
}

void ACharacterWeaponBase::RefreshWeaponTrailAssets()
{
	if (WeaponTrailComponent)
	{
		WeaponTrailComponent->SetAsset(WeaponTrailSystem);
	}

	if (WeaponCascadeTrailComponent)
	{
		WeaponCascadeTrailComponent->SetTemplate(WeaponCascadeTrailSystem);
	}
}

void ACharacterWeaponBase::RefreshWeaponTrailAttachments()
{
	if (!SKEquipmentMesh)
	{
		return;
	}

	const FName AttachSocketName = HasWeaponTrailSocket(WeaponTrailStartSocketName)
		? WeaponTrailStartSocketName
		: NAME_None;

	if (WeaponTrailComponent)
	{
		WeaponTrailComponent->AttachToComponent(
			SKEquipmentMesh,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			AttachSocketName);
	}

	if (WeaponCascadeTrailComponent)
	{
		WeaponCascadeTrailComponent->AttachToComponent(
			SKEquipmentMesh,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			AttachSocketName);
	}
}

bool ACharacterWeaponBase::GetLeftHandIKWorldTransform(FTransform& OutTransform) const
{
	return ResolveWeaponIKSocketWorldTransform(LeftWeaponIKSocketName, OutTransform);
}

bool ACharacterWeaponBase::GetRightHandIKWorldTransform(FTransform& OutTransform) const
{
	return ResolveWeaponIKSocketWorldTransform(RightWeaponIKSocketName, OutTransform);
}

void ACharacterWeaponBase::SetWeaponTrailEnabled(bool bEnabled)
{
	const int32 PreviousRequestCount = WeaponTrailRequestCount;
	WeaponTrailRequestCount = bEnabled
		? PreviousRequestCount + 1
		: FMath::Max(0, PreviousRequestCount - 1);

	const bool bWasActive = PreviousRequestCount > 0;
	const bool bShouldBeActive = WeaponTrailRequestCount > 0;

	if (bWasActive == bShouldBeActive)
	{
		return;
	}

	HandleWeaponTrailStateChanged(bShouldBeActive);
}

void ACharacterWeaponBase::OnEquip(ACharacter* NewOwner)
{
	CurrentMontage = nullptr;
	ComboIndex = 0;
	SetCurrentDamageMultiplier(1.f);
	ResetWeaponTrailState();
	SetAbilityNiagaraEnabled(false);
	Super::OnEquip(NewOwner);

	if (OwnerCharacter)
	{
		UAnimInstance* AnimInstance = OwnerCharacter->GetMesh()->GetAnimInstance();
		if (AnimInstance)
		{
			AnimInstance->OnMontageEnded.AddUniqueDynamic(this, &ACharacterWeaponBase::OnMontageEnded);
			AnimInstance->OnMontageStarted.AddUniqueDynamic(this, &ACharacterWeaponBase::OnMontageStarted);
		}
	}
}

void ACharacterWeaponBase::OnRep_Owner()
{
	Super::OnRep_Owner();

	CurrentMontage = nullptr;
	ComboIndex = 0;
	SetCurrentDamageMultiplier(1.f);
	ResetWeaponTrailState();
	SetAbilityNiagaraEnabled(false);

	if (OwnerCharacter)
	{
		UAnimInstance* AnimInstance = OwnerCharacter->GetMesh() ? OwnerCharacter->GetMesh()->GetAnimInstance() : nullptr;
		if (AnimInstance)
		{
			AnimInstance->OnMontageEnded.AddUniqueDynamic(this, &ACharacterWeaponBase::OnMontageEnded);
			AnimInstance->OnMontageStarted.AddUniqueDynamic(this, &ACharacterWeaponBase::OnMontageStarted);
		}
	}
}

bool ACharacterWeaponBase::CanStartAction(EEquipmentActionType ActionType) const
{
	if (!OwnerCharacter)
	{
		return false;
	}

	switch (ActionType)
	{
	case EEquipmentActionType::Primary:
	{
		if (basicAttackMontages.Num() == 0)
		{
			return false;
		}

		const int32 AttackIndex = basicAttackMontages.IsValidIndex(ComboIndex) ? ComboIndex : 0;
		return basicAttackMontages.IsValidIndex(AttackIndex) && basicAttackMontages[AttackIndex] != nullptr;
	}
	case EEquipmentActionType::Secondary:
		return SecondaryActionMontage != nullptr;
	case EEquipmentActionType::Ability:
		return AbilityActionMontage != nullptr;
	default:
		return false;
	}
}

bool ACharacterWeaponBase::ShouldConsumeStaminaForAction(EEquipmentActionType ActionType) const
{
	return CanStartAction(ActionType) && GetStaminaCostForAction(ActionType) > KINDA_SMALL_NUMBER;
}

float ACharacterWeaponBase::GetStaminaCostForAction(EEquipmentActionType ActionType) const
{
	switch (ActionType)
	{
	case EEquipmentActionType::Primary:
	{
		if (PrimaryActionStaminaCosts.Num() == 0)
		{
			return 0.f;
		}

		const int32 CostIndex = PrimaryActionStaminaCosts.IsValidIndex(ComboIndex) ? ComboIndex : 0;
		return FMath::Max(0.f, PrimaryActionStaminaCosts[CostIndex]);
	}
	case EEquipmentActionType::Secondary:
		return FMath::Max(0.f, SecondaryActionStaminaCost);
	case EEquipmentActionType::Ability:
		return FMath::Max(0.f, AbilityActionStaminaCost);
	default:
		return 0.f;
	}
}

bool ACharacterWeaponBase::OnUnequip()
{
	ResetWeaponTrailState();
	SetAbilityNiagaraEnabled(false);

	if (OwnerCharacter)
	{
		if (UAnimInstance* AnimInstance = OwnerCharacter->GetMesh()->GetAnimInstance())
		{
			AnimInstance->OnMontageEnded.RemoveDynamic(this, &ACharacterWeaponBase::OnMontageEnded);
			AnimInstance->OnMontageStarted.RemoveDynamic(this, &ACharacterWeaponBase::OnMontageStarted);
		}
	}

	if (CurrentMontage && OwnerCharacter)
	{
		StopOwnerMontage(CurrentMontage, 0.25f);
	}

	CurrentMontage = nullptr;
	ComboIndex = 0;
	SetCurrentDamageMultiplier(1.f);
	return Super::OnUnequip();
}

void ACharacterWeaponBase::CompleteUnequip()
{
	ResetWeaponTrailState();
	SetAbilityNiagaraEnabled(false);

	if (OwnerCharacter)
	{
		if (UAnimInstance* AnimInstance = OwnerCharacter->GetMesh()->GetAnimInstance())
		{
			AnimInstance->OnMontageEnded.RemoveDynamic(this, &ACharacterWeaponBase::OnMontageEnded);
			AnimInstance->OnMontageStarted.RemoveDynamic(this, &ACharacterWeaponBase::OnMontageStarted);
		}
	}

	if (CurrentMontage && OwnerCharacter)
	{
		StopOwnerMontage(CurrentMontage, 0.25f);
	}

	ResetWeaponTrailState();
	CurrentMontage = nullptr;
	ComboIndex = 0;
	SetCurrentDamageMultiplier(1.f);
	Super::CompleteUnequip();
}

void ACharacterWeaponBase::OnMontageEnded(UAnimMontage* Montage, bool bInterrupted)
{
	UE_LOG(LogTemp, Warning, TEXT("[PrimaryDebug][WeaponMontageEnded] Weapon=%s Owner=%s Role=%d HasAuthority=%d LocalOwner=%d Montage=%s Current=%s Interrupted=%d ComboIndex=%d"),
		*GetNameSafe(this),
		*GetNameSafe(OwnerCharacter),
		GetOwner() ? static_cast<int32>(GetOwner()->GetLocalRole()) : -1,
		HasAuthority() ? 1 : 0,
		OwnerCharacter && OwnerCharacter->IsLocallyControlled() ? 1 : 0,
		*GetNameSafe(Montage),
		*GetNameSafe(CurrentMontage),
		bInterrupted ? 1 : 0,
		ComboIndex);

	if (Montage != CurrentMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PrimaryDebug][WeaponMontageEndedIgnored] Reason=NotCurrent"));
		return;
	}

	if (OwnerCharacter)
	{
		UAnimInstance* AnimInstance = OwnerCharacter->GetMesh() ? OwnerCharacter->GetMesh()->GetAnimInstance() : nullptr;
		if (AnimInstance && AnimInstance->Montage_IsPlaying(Montage))
		{
			UE_LOG(LogTemp, Warning, TEXT("[PrimaryDebug][WeaponMontageEndedIgnored] Reason=StillPlaying Montage=%s"), *GetNameSafe(Montage));
			return;
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("[PrimaryDebug][WeaponComboReset] Weapon=%s Montage=%s ComboIndexBefore=%d"),
		*GetNameSafe(this),
		*GetNameSafe(Montage),
		ComboIndex);

	CurrentMontage = nullptr;
	ComboIndex = 0;
	SetCurrentDamageMultiplier(1.f);

	if (bInterrupted)
	{
		OnInterruptedMontageEnd();
		return;
	}

	OnNaturalMontageEnd();
}

void ACharacterWeaponBase::StartPrimaryAction()
{
	UE_LOG(LogTemp, Warning, TEXT("[PrimaryDebug][WeaponStartPrimaryEnter] Weapon=%s Owner=%s Role=%d HasAuthority=%d LocalOwner=%d ComboIndex=%d MontageNum=%d Current=%s"),
		*GetNameSafe(this),
		*GetNameSafe(OwnerCharacter),
		GetOwner() ? static_cast<int32>(GetOwner()->GetLocalRole()) : -1,
		HasAuthority() ? 1 : 0,
		OwnerCharacter && OwnerCharacter->IsLocallyControlled() ? 1 : 0,
		ComboIndex,
		basicAttackMontages.Num(),
		*GetNameSafe(CurrentMontage));

	if (basicAttackMontages.Num() == 0 || !OwnerCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PrimaryDebug][WeaponStartPrimaryBlocked] Reason=NoMontageOrOwner MontageNum=%d Owner=%s"),
			basicAttackMontages.Num(),
			*GetNameSafe(OwnerCharacter));
		return;
	}

	if (CurrentMontage)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PrimaryDebug][WeaponStartPrimaryClearCurrent] PreviousCurrent=%s ComboIndex=%d"),
			*GetNameSafe(CurrentMontage),
			ComboIndex);
		CurrentMontage = nullptr;
	}

	if (!basicAttackMontages.IsValidIndex(ComboIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("[PrimaryDebug][WeaponComboIndexInvalid] ComboIndex=%d MontageNum=%d -> 0"),
			ComboIndex,
			basicAttackMontages.Num());
		ComboIndex = 0;
	}

	const int32 AttackIndex = ComboIndex;
	UAnimMontage* Montage = basicAttackMontages[AttackIndex];
	if (!Montage)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PrimaryDebug][WeaponStartPrimaryBlocked] Reason=NullMontage AttackIndex=%d"), AttackIndex);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[PrimaryDebug][WeaponPlayPrimary] AttackIndex=%d Montage=%s ComboIndexBefore=%d"),
		AttackIndex,
		*GetNameSafe(Montage),
		ComboIndex);

	CurrentMontage = Montage;
	SetCurrentDamageMultiplierForPrimaryAttackIndex(AttackIndex);

	if (!HasAuthority())
	{
		if (AMainCharacterBase* MainCharacter = Cast<AMainCharacterBase>(OwnerCharacter))
		{
			if (MainCharacter->IsLocallyControlled())
			{
				MainCharacter->PlayPredictedWeaponMontageLocally(Montage);
			}
			else
			{
				PlayOwnerMontage(Montage);
			}
		}
		else
		{
			PlayOwnerMontage(Montage);
		}
	}
	else
	{
		PlayOwnerMontage(Montage);
	}

	ComboIndex++;
	if (ComboIndex >= basicAttackMontages.Num()) ComboIndex = 0;
	UE_LOG(LogTemp, Warning, TEXT("[PrimaryDebug][WeaponComboAdvanced] AttackIndex=%d ComboIndexAfter=%d"),
		AttackIndex,
		ComboIndex);
}

void ACharacterWeaponBase::StopPrimaryAction()
{
}

void ACharacterWeaponBase::StartSecondaryAction()
{
	if (!SecondaryActionMontage || !OwnerCharacter) return;

	CurrentMontage = SecondaryActionMontage;
	SetCurrentDamageMultiplier(SecondaryActionDamageMultiplier);
	PlayOwnerMontage(SecondaryActionMontage);
}

void ACharacterWeaponBase::StartAbilityAction()
{
	if (!AbilityActionMontage || !OwnerCharacter) return;

	CurrentMontage = AbilityActionMontage;
	SetCurrentDamageMultiplier(AbilityActionDamageMultiplier);
	PlayOwnerMontage(AbilityActionMontage);
}

bool ACharacterWeaponBase::IsAbilityMontage(const UAnimMontage* Montage) const
{
	return Montage && Montage == AbilityActionMontage;
}

void ACharacterWeaponBase::TriggerOwnerHitStop() const
{
	if (HitStopDuration <= 0.f)
	{
		return;
	}

	const ACharacter* ResolvedOwnerCharacter = ResolveOwnerCharacter();
	const AMainCharacterBase* MainCharacter = Cast<AMainCharacterBase>(ResolvedOwnerCharacter);
	if (!MainCharacter)
	{
		return;
	}

	UAnimMontage* HitStopMontage = CurrentMontage;
	if (!HitStopMontage && ResolvedOwnerCharacter && ResolvedOwnerCharacter->GetMesh())
	{
		if (UAnimInstance* AnimInstance = ResolvedOwnerCharacter->GetMesh()->GetAnimInstance())
		{
			HitStopMontage = AnimInstance->GetCurrentActiveMontage();
		}
	}

	if (!HitStopMontage)
	{
		return;
	}

	const_cast<AMainCharacterBase*>(MainCharacter)->RequestMontageHitStop(HitStopMontage, HitStopDuration, HitStopPlayRate);
}

void ACharacterWeaponBase::TriggerTargetHitStop(AActor* Target)
{
	if (HitStopDuration <= 0.f || !Target)
	{
		return;
	}

	AVL_AICharacterBase* TargetMonster = Cast<AVL_AICharacterBase>(Target);
	if (!TargetMonster || !TargetMonster->GetMesh())
	{
		if (!TargetMonster)
		{
			UE_LOG(LogTemp, Warning, TEXT("TargetMonster is nullptr (Cast failed)"));
			return;
		}

		if (!TargetMonster->GetMesh())
		{
			UE_LOG(LogTemp, Warning, TEXT("TargetMonster->GetMesh() is nullptr"));
			return;
		}
		return;
	}

	UAnimInstance* AnimInstance = TargetMonster->GetMesh()->GetAnimInstance();
	if (!AnimInstance)
	{
		return;
	}

	UAnimMontage* TargetMontage = AnimInstance->GetCurrentActiveMontage();
	if (!TargetMontage)
	{
		return;
	}

	TWeakObjectPtr<UAnimInstance> WeakAnimInst(AnimInstance);
	TWeakObjectPtr<UAnimMontage> WeakMontage(TargetMontage);
	const float RestorePlayRate = AnimInstance->Montage_GetPlayRate(TargetMontage) > KINDA_SMALL_NUMBER
		? AnimInstance->Montage_GetPlayRate(TargetMontage)
		: 1.f;
	constexpr float MinHitStopPlayRate = 0.05f;
	const float EffectiveHitStopPlayRate = HitStopPlayRate <= KINDA_SMALL_NUMBER
		? MinHitStopPlayRate
		: HitStopPlayRate;

	AnimInstance->Montage_SetPlayRate(TargetMontage, EffectiveHitStopPlayRate);

	FTimerHandle TimerHandle;
	GetWorld()->GetTimerManager().SetTimer(TimerHandle, FTimerDelegate::CreateLambda([WeakAnimInst, WeakMontage, RestorePlayRate]()
	{
		if (!WeakAnimInst.IsValid() || !WeakMontage.IsValid())
		{
			return;
		}
		WeakAnimInst->Montage_SetPlayRate(WeakMontage.Get(), RestorePlayRate);
	}), HitStopDuration, false);
}

void ACharacterWeaponBase::ApplyDamageToActor(AActor* OtherActor, UPrimitiveComponent* HitComponent, UPrimitiveComponent* SourceComponent)
{
	AActor* DamageCauser = GetOwner();
	if (!OtherActor || OtherActor == this || OtherActor == DamageCauser)
	{
		return;
	}

	AController* InstigatorController = nullptr;
	if (const APawn* OwnerPawn = Cast<APawn>(DamageCauser))
	{
		InstigatorController = OwnerPawn->GetController();
	}

	const float AppliedDamage = UGameplayStatics::ApplyDamage(
		OtherActor,
		GetCurrentActionDamage(),
		InstigatorController,
		this,
		DamageTypeClass ? DamageTypeClass.Get() : UDamageType::StaticClass());

	if (AppliedDamage > 0.f)
	{
		PlayHitSound(OtherActor);
		PlayHitEffect(OtherActor, HitComponent, SourceComponent);
		DragSwingSoundsForHitStop();
	}

	TriggerOwnerHitStop();
	TriggerTargetHitStop(OtherActor);
}

UAudioComponent* ACharacterWeaponBase::SpawnSoundAtWeapon(USoundBase* Sound, float VolumeMultiplier, float PitchMultiplier) const
{
	if (!Sound)
	{
		return nullptr;
	}

	const FVector SoundLocation = SKEquipmentMesh ? SKEquipmentMesh->GetComponentLocation() : GetActorLocation();
	return UGameplayStatics::SpawnSoundAtLocation(
		this,
		Sound,
		SoundLocation,
		FRotator::ZeroRotator,
		VolumeMultiplier,
		PitchMultiplier,
		0.f,
		SoundAttenuationSettings);
}

void ACharacterWeaponBase::PlayHitSound(AActor* HitActor)
{
	if (!HasAuthority())
	{
		return;
	}

	if (!HitSoundCue)
	{
		return;
	}

	const FVector SoundLocation = HitActor ? HitActor->GetActorLocation() : GetActorLocation();
	MulticastPlayHitSound(
		HitSoundCue,
		SoundLocation,
		GetSoundVolumeMultiplierForMontage(CurrentMontage),
		GetSoundPitchMultiplierForMontage(CurrentMontage));
}

void ACharacterWeaponBase::PlayHitEffect(AActor* HitActor, UPrimitiveComponent* HitComponent, UPrimitiveComponent* SourceComponent)
{
	if (!HasAuthority() || !HitEffectSystem)
	{
		return;
	}

	FVector EffectLocation;
	FRotator EffectRotation;
	FVector EffectNormal;
	ResolveHitEffectTransform(HitActor, HitComponent, SourceComponent, EffectLocation, EffectRotation, EffectNormal);

	FVector AngleDirection = FVector::ZeroVector;
	if (bHasPreviousTrailStartLocation)
	{
		AngleDirection = CurrentTrailStartLocation - PreviousTrailStartLocation;
	}
	if (AngleDirection.IsNearlyZero())
	{
		AngleDirection = SourceComponent ? SourceComponent->GetUpVector() : EffectNormal;
	}

	const FLinearColor SelectedCustomColor = IsAbilityMontage(CurrentMontage)
		? AbilityHitEffectCustomColor
		: HitEffectCustomColor;

	MulticastPlayHitEffect(HitEffectSystem, EffectLocation, EffectRotation, HitEffectScale, EffectNormal, AngleDirection, SelectedCustomColor);
}

void ACharacterWeaponBase::ResolveHitEffectTransform(AActor* HitActor, UPrimitiveComponent* HitComponent, UPrimitiveComponent* SourceComponent, FVector& OutLocation, FRotator& OutRotation, FVector& OutNormal) const
{
	const FVector SourceLocation = SourceComponent ? SourceComponent->GetComponentLocation() : (SKEquipmentMesh ? SKEquipmentMesh->GetComponentLocation() : GetActorLocation());
	if (!HitActor)
	{
		OutLocation = SourceLocation;
		OutNormal = GetActorForwardVector();
		OutRotation = GetActorRotation();
		return;
	}

	if (HitComponent)
	{
		FVector ClosestPoint = HitActor->GetActorLocation();
		const float ClosestDistance = HitComponent->GetClosestPointOnCollision(SourceLocation, ClosestPoint);
		if (ClosestDistance >= 0.f)
		{
			OutLocation = ClosestPoint;
			OutNormal = (SourceLocation - ClosestPoint).GetSafeNormal();
			if (OutNormal.IsNearlyZero())
			{
				OutNormal = (SourceLocation - HitActor->GetActorLocation()).GetSafeNormal();
			}
			if (OutNormal.IsNearlyZero())
			{
				OutNormal = GetActorForwardVector();
			}
			OutRotation = GetActorRotation();
			return;
		}
	}

	FVector Origin;
	FVector Extent;
	HitActor->GetActorBounds(false, Origin, Extent);
	if (Extent.IsNearlyZero())
	{
		OutLocation = HitActor->GetActorLocation();
		OutNormal = (SourceLocation - OutLocation).GetSafeNormal();
		if (OutNormal.IsNearlyZero())
		{
			OutNormal = GetActorForwardVector();
		}
		OutRotation = GetActorRotation();
		return;
	}

	const FVector BoxMin = Origin - Extent;
	const FVector BoxMax = Origin + Extent;
	OutLocation = FVector(
		FMath::Clamp(SourceLocation.X, BoxMin.X, BoxMax.X),
		FMath::Clamp(SourceLocation.Y, BoxMin.Y, BoxMax.Y),
		FMath::Clamp(SourceLocation.Z, BoxMin.Z, BoxMax.Z));
	OutNormal = (SourceLocation - OutLocation).GetSafeNormal();
	if (OutNormal.IsNearlyZero())
	{
		OutNormal = GetActorForwardVector();
	}
	OutRotation = GetActorRotation();
}

float ACharacterWeaponBase::GetSoundVolumeMultiplierForMontage(const UAnimMontage* Montage) const
{
	if (!Montage)
	{
		return DefaultSoundVolumeMultiplier;
	}

	for (int32 AttackIndex = 0; AttackIndex < basicAttackMontages.Num(); ++AttackIndex)
	{
		if (basicAttackMontages[AttackIndex] == Montage)
		{
			if (PrimaryAttackSoundVolumeMultipliers.IsValidIndex(AttackIndex))
			{
				return PrimaryAttackSoundVolumeMultipliers[AttackIndex];
			}

			if (PrimaryAttackSoundVolumeMultipliers.Num() > 0)
			{
				return PrimaryAttackSoundVolumeMultipliers.Last();
			}

			return DefaultSoundVolumeMultiplier;
		}
	}

	if (Montage == SecondaryActionMontage)
	{
		return SecondaryActionSoundVolumeMultiplier;
	}

	if (Montage == AbilityActionMontage)
	{
		return AbilityActionSoundVolumeMultiplier;
	}

	return DefaultSoundVolumeMultiplier;
}

float ACharacterWeaponBase::GetSoundPitchMultiplierForMontage(const UAnimMontage* Montage) const
{
	if (!Montage)
	{
		return 1.f;
	}

	for (int32 AttackIndex = 0; AttackIndex < basicAttackMontages.Num(); ++AttackIndex)
	{
		if (basicAttackMontages[AttackIndex] == Montage)
		{
			if (PrimaryAttackSoundPitchMultipliers.IsValidIndex(AttackIndex))
			{
				return PrimaryAttackSoundPitchMultipliers[AttackIndex];
			}

			if (PrimaryAttackSoundPitchMultipliers.Num() > 0)
			{
				return PrimaryAttackSoundPitchMultipliers.Last();
			}

			return 1.f;
		}
	}

	if (Montage == SecondaryActionMontage)
	{
		return SecondaryActionSoundPitchMultiplier;
	}

	if (Montage == AbilityActionMontage)
	{
		return AbilityActionSoundPitchMultiplier;
	}

	return 1.f;
}

void ACharacterWeaponBase::DragSwingSoundsForHitStop()
{
	if (!HasAuthority() || HitStopDuration <= 0.f)
	{
		return;
	}

	MulticastSetSwingSoundPitchMultiplier(SwingHitStopPitchMultiplier);

	GetWorld()->GetTimerManager().ClearTimer(SwingSoundHitStopTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(
		SwingSoundHitStopTimerHandle,
		FTimerDelegate::CreateWeakLambda(this, [this]()
		{
			MulticastSetSwingSoundPitchMultiplier(1.f);
		}),
		HitStopDuration,
		false);
}

void ACharacterWeaponBase::SetActiveSwingSoundPitchMultiplier(float PitchMultiplier)
{
	CleanupActiveSwingSounds();

	for (int32 Index = 0; Index < ActiveSwingSoundComponents.Num(); ++Index)
	{
		if (UAudioComponent* AudioComponent = ActiveSwingSoundComponents[Index].Get())
		{
			const float BasePitchMultiplier = ActiveSwingSoundBasePitchMultipliers.IsValidIndex(Index)
				? ActiveSwingSoundBasePitchMultipliers[Index]
				: 1.f;

			AudioComponent->SetPitchMultiplier(BasePitchMultiplier * PitchMultiplier);
		}
	}
}

void ACharacterWeaponBase::CleanupActiveSwingSounds()
{
	for (int32 Index = ActiveSwingSoundComponents.Num() - 1; Index >= 0; --Index)
	{
		if (!ActiveSwingSoundComponents[Index].IsValid())
		{
			ActiveSwingSoundComponents.RemoveAt(Index);

			if (ActiveSwingSoundBasePitchMultipliers.IsValidIndex(Index))
			{
				ActiveSwingSoundBasePitchMultipliers.RemoveAt(Index);
			}
		}
	}
}

void ACharacterWeaponBase::MulticastPlaySwingSound_Implementation(USoundBase* Sound, float VolumeMultiplier, float PitchMultiplier)
{
	if (UAudioComponent* AudioComponent = SpawnSoundAtWeapon(Sound, VolumeMultiplier, PitchMultiplier))
	{
		ActiveSwingSoundComponents.Add(AudioComponent);
		ActiveSwingSoundBasePitchMultipliers.Add(PitchMultiplier);
	}
}

void ACharacterWeaponBase::MulticastPlayHitSound_Implementation(USoundBase* Sound, FVector Location, float VolumeMultiplier, float PitchMultiplier)
{
	if (!Sound)
	{
		return;
	}

	UGameplayStatics::PlaySoundAtLocation(
		this,
		Sound,
		Location,
		VolumeMultiplier,
		PitchMultiplier,
		0.f,
		SoundAttenuationSettings);
}

void ACharacterWeaponBase::MulticastPlayHitEffect_Implementation(UNiagaraSystem* Effect, FVector Location, FRotator Rotation, float Scale, FVector Normal, FVector AngleDirection, FLinearColor CustomColor)
{
	if (!Effect)
	{
		return;
	}

	float ScreenAngleDegrees = 0.f;
	const FVector HitDirection = AngleDirection.IsNearlyZero() ? Rotation.Vector() : AngleDirection.GetSafeNormal();
	if (APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0))
	{
		FVector CameraLocation;
		FRotator CameraRotation;
		CameraManager->GetCameraViewPoint(CameraLocation, CameraRotation);

		const FRotationMatrix CameraMatrix(CameraRotation);
		const FVector CameraForward = CameraRotation.Vector();
		const FVector CameraRight = CameraMatrix.GetUnitAxis(EAxis::Y);
		const FVector CameraUp = CameraMatrix.GetUnitAxis(EAxis::Z);
		const FVector ProjectedDirection = FVector::VectorPlaneProject(HitDirection, CameraForward).GetSafeNormal();

		if (!ProjectedDirection.IsNearlyZero())
		{
			const float ScreenX = FVector::DotProduct(ProjectedDirection, CameraRight);
			const float ScreenY = FVector::DotProduct(ProjectedDirection, CameraUp);
			ScreenAngleDegrees = FMath::RadiansToDegrees(FMath::Atan2(ScreenY, ScreenX));
		}
	}

	UNiagaraComponent* NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		this,
		Effect,
		Location,
		Rotation,
		FVector(1.f),
		true,
		false);

	if (NiagaraComponent)
	{
		NiagaraComponent->SetVariableFloat(HitEffectScaleParameterName, Scale);
		NiagaraComponent->SetVariableFloat(HitEffectAngleParameterName, ScreenAngleDegrees);
		if (!HitEffectCustomColorParameterName.IsNone())
		{
			NiagaraComponent->SetVariableLinearColor(
				HitEffectCustomColorParameterName,
				CustomColor);
		}
		NiagaraComponent->Activate(true);
	}
}

void ACharacterWeaponBase::MulticastSetSwingSoundPitchMultiplier_Implementation(float PitchMultiplier)
{
	SetActiveSwingSoundPitchMultiplier(PitchMultiplier);
}

float ACharacterWeaponBase::GetPrimaryAttackDamageMultiplier(int32 AttackIndex) const
{
	if (PrimaryAttackDamageMultipliers.IsValidIndex(AttackIndex))
	{
		return PrimaryAttackDamageMultipliers[AttackIndex];
	}

	if (PrimaryAttackDamageMultipliers.Num() > 0)
	{
		return PrimaryAttackDamageMultipliers.Last();
	}

	return 1.f;
}

float ACharacterWeaponBase::GetDamageMultiplierForMontage(const UAnimMontage* Montage) const
{
	if (!Montage)
	{
		return 1.f;
	}

	for (int32 AttackIndex = 0; AttackIndex < basicAttackMontages.Num(); ++AttackIndex)
	{
		if (basicAttackMontages[AttackIndex] == Montage)
		{
			return GetPrimaryAttackDamageMultiplier(AttackIndex);
		}
	}

	if (Montage == SecondaryActionMontage)
	{
		return SecondaryActionDamageMultiplier;
	}

	if (Montage == AbilityActionMontage)
	{
		return AbilityActionDamageMultiplier;
	}

	return 1.f;
}

void ACharacterWeaponBase::SetCurrentDamageMultiplier(float NewMultiplier)
{
	CurrentDamageMultiplier = FMath::Max(0.f, NewMultiplier);
}

void ACharacterWeaponBase::SetCurrentDamageMultiplierForPrimaryAttackIndex(int32 AttackIndex)
{
	SetCurrentDamageMultiplier(GetPrimaryAttackDamageMultiplier(AttackIndex));
}

void ACharacterWeaponBase::SetCurrentDamageMultiplierForMontage(const UAnimMontage* Montage)
{
	SetCurrentDamageMultiplier(GetDamageMultiplierForMontage(Montage));
}

void ACharacterWeaponBase::ResetWeaponTrailState()
{
	WeaponTrailRequestCount = 0;
	HandleWeaponTrailStateChanged(false);
}

void ACharacterWeaponBase::RefreshAbilityNiagaraAttachment()
{
	if (!AbilityNiagaraComponent || !SKEquipmentMesh)
	{
		return;
	}

	const FName AttachSocketName =
		!AbilityNiagaraAttachSocket.IsNone() &&
		SKEquipmentMesh->DoesSocketExist(AbilityNiagaraAttachSocket)
			? AbilityNiagaraAttachSocket
			: NAME_None;

	AbilityNiagaraComponent->AttachToComponent(
		SKEquipmentMesh,
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		AttachSocketName);
}

void ACharacterWeaponBase::SetAbilityNiagaraEnabled(bool bEnabled)
{
	if (!AbilityNiagaraComponent)
	{
		return;
	}

	AbilityNiagaraComponent->SetAsset(AbilityNiagaraSystem);
	RefreshAbilityNiagaraAttachment();

	if (bEnabled && AbilityNiagaraSystem)
	{
		AbilityNiagaraComponent->Activate(true);
		return;
	}

	AbilityNiagaraComponent->DeactivateImmediate();
}

void ACharacterWeaponBase::HandleWeaponTrailStateChanged_Implementation(bool bEnabled)
{
	RefreshWeaponTrailAssets();
	RefreshWeaponTrailAttachments();

	const bool bHasTrailStartSocket = HasWeaponTrailSocket(WeaponTrailStartSocketName);
	const bool bHasTrailEndSocket = HasWeaponTrailSocket(WeaponTrailEndSocketName);
	const bool bHasCompleteTrailSocketPair = bHasTrailStartSocket && bHasTrailEndSocket;

	if (bEnabled)
	{
		if (!bHasCompleteTrailSocketPair)
		{
			if (WeaponTrailComponent)
			{
				WeaponTrailComponent->Deactivate();
			}

			if (WeaponCascadeTrailComponent)
			{
				WeaponCascadeTrailComponent->EndTrails();
				WeaponCascadeTrailComponent->Deactivate();
			}

			return;
		}

		if (WeaponTrailComponent && WeaponTrailSystem)
		{
			WeaponTrailComponent->Activate(true);
		}

		if (WeaponCascadeTrailComponent && WeaponCascadeTrailSystem)
		{
			WeaponCascadeTrailComponent->Activate(true);
			WeaponCascadeTrailComponent->BeginTrails(
				WeaponTrailStartSocketName,
				WeaponTrailEndSocketName,
				ETrailWidthMode_FromCentre,
				DefaultWeaponTrailWidth);
		}

		return;
	}

	if (WeaponTrailComponent)
	{
		WeaponTrailComponent->Deactivate();
	}

	if (WeaponCascadeTrailComponent)
	{
		WeaponCascadeTrailComponent->EndTrails();
		WeaponCascadeTrailComponent->Deactivate();
	}
}

void ACharacterWeaponBase::ForceResetState()
{
	ResetWeaponTrailState();
	SetAbilityNiagaraEnabled(false);

	if (CurrentMontage && OwnerCharacter)
	{
		StopOwnerMontage(CurrentMontage, 0.25f);
	}

	CurrentMontage = nullptr;
	ComboIndex = 0;
	SetCurrentDamageMultiplier(1.f);
}

void ACharacterWeaponBase::CancelAction()
{
	ForceResetState();
}
// Called every frame
void ACharacterWeaponBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (SKEquipmentMesh && SKEquipmentMesh->DoesSocketExist(WeaponTrailStartSocketName))
	{
		const FVector NewTrailStartLocation = SKEquipmentMesh->GetSocketLocation(WeaponTrailStartSocketName);
		if (!bHasPreviousTrailStartLocation)
		{
			PreviousTrailStartLocation = NewTrailStartLocation;
			CurrentTrailStartLocation = NewTrailStartLocation;
			bHasPreviousTrailStartLocation = true;
			return;
		}

		PreviousTrailStartLocation = CurrentTrailStartLocation;
		CurrentTrailStartLocation = NewTrailStartLocation;
		return;
	}

	bHasPreviousTrailStartLocation = false;
}

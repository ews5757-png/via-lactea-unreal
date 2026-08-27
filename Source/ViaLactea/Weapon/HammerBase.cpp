#include "HammerBase.h"
#include "Player/MainCharacterBase.h"
#include "Components/AudioComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInterface.h"
#include "Net/UnrealNetwork.h"
#include "NiagaraComponent.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"

namespace
{
	const FName HammerIgnoreStickTag(TEXT("HammerIgnoreStick"));

	FString BuildHammerHitLogString(const FHitResult& Hit)
	{
		const AActor* HitActor = Hit.GetActor();
		const UPrimitiveComponent* HitComponent = Hit.GetComponent();

		const FString ActorName = GetNameSafe(HitActor);
		const FString ActorClassName = HitActor ? GetNameSafe(HitActor->GetClass()) : TEXT("None");
		const FString ComponentName = GetNameSafe(HitComponent);
		const FString ComponentClassName = HitComponent ? GetNameSafe(HitComponent->GetClass()) : TEXT("None");
		const FString CollisionProfileName = HitComponent ? HitComponent->GetCollisionProfileName().ToString() : TEXT("None");
		const int32 ObjectType = HitComponent ? static_cast<int32>(HitComponent->GetCollisionObjectType()) : INDEX_NONE;

		return FString::Printf(
			TEXT("Actor=%s Class=%s Component=%s ComponentClass=%s Profile=%s ObjectType=%d Bone=%s Location=%s ImpactPoint=%s ImpactNormal=%s Time=%.3f Blocking=%d StartPenetrating=%d"),
			*ActorName,
			*ActorClassName,
			*ComponentName,
			*ComponentClassName,
			*CollisionProfileName,
			ObjectType,
			*Hit.BoneName.ToString(),
			*Hit.Location.ToCompactString(),
			*Hit.ImpactPoint.ToCompactString(),
			*Hit.ImpactNormal.ToCompactString(),
			Hit.Time,
			Hit.bBlockingHit,
			Hit.bStartPenetrating);
	}

	bool ShouldIgnoreHammerStickHit(const FHitResult& Hit)
	{
		const AActor* HitActor = Hit.GetActor();
		const UPrimitiveComponent* HitComponent = Hit.GetComponent();
		if (!HitActor)
		{
			return true;
		}

		if (Cast<APawn>(HitActor))
		{
			return true;
		}

		if (HitActor->ActorHasTag(HammerIgnoreStickTag) ||
			(HitComponent && HitComponent->ComponentHasTag(HammerIgnoreStickTag)))
		{
			return true;
		}

		const UClass* HitActorClass = HitActor->GetClass();
		return HitActorClass && HitActorClass->GetFName() == TEXT("PCGVolume");
	}
}

AHammerBase::AHammerBase()
{
	WeaponType = EWeaponAnimType::OneHandedMelee;
	Damage = 30.f;
	PrimaryAttackDamageMultipliers = { 1.0f, 1.2f };
	DefaultSoundVolumeMultiplier = 1.f;
	PrimaryAttackSoundVolumeMultipliers = { 1.f, 1.f };
	SecondaryActionDamageMultiplier = 1.15f;
	SecondaryActionSoundVolumeMultiplier = 1.f;
	AbilityActionDamageMultiplier = 1.3f;
	AbilityActionSoundVolumeMultiplier = 1.f;

	HammerThrowNiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("HammerThrowNiagaraComponent"));
	HammerThrowNiagaraComponent->SetupAttachment(SKEquipmentMesh);
	HammerThrowNiagaraComponent->SetAutoActivate(false);
	HammerThrowNiagaraComponent->bAutoActivate = false;

	HammerFlightLoopAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("HammerFlightLoopAudioComponent"));
	HammerFlightLoopAudioComponent->SetupAttachment(SKEquipmentMesh);
	HammerFlightLoopAudioComponent->SetAutoActivate(false);
	HammerFlightLoopAudioComponent->bAutoActivate = false;

	HammerEffectLoopAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("HammerEffectLoopAudioComponent"));
	HammerEffectLoopAudioComponent->SetupAttachment(SKEquipmentMesh);
	HammerEffectLoopAudioComponent->SetAutoActivate(false);
	HammerEffectLoopAudioComponent->bAutoActivate = false;
}

void AHammerBase::BeginPlay()
{
	Super::BeginPlay();

	SetHammerThrowVisualsEnabled(false);
}

void AHammerBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AHammerBase, bHammerThrown);
	DOREPLIFETIME(AHammerBase, bHammerReturning);
	DOREPLIFETIME(AHammerBase, bHammerStuck);
	DOREPLIFETIME(AHammerBase, ThrowVelocity);
}

void AHammerBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	SyncHammerFlightTrailState();
	UpdateHammerThrowLoopSounds();

	if (!bHammerThrown || bHammerStuck)
	{
		return;
	}

	const bool bShouldSimulateHammer = HasAuthority() || (OwnerCharacter && OwnerCharacter->IsLocallyControlled());
	if (!bShouldSimulateHammer)
	{
		return;
	}

	if (!bHammerReturning)
	{
		const FVector CurrentLocation = GetActorLocation();
		const FVector NextLocation = CurrentLocation + ThrowVelocity * DeltaTime;
		UpdateHammerSpin(ThrowVelocity.Size(), DeltaTime, false);
		ApplyHammerTravelRotation(ThrowVelocity);

		if (TryStickToWorld(CurrentLocation, NextLocation))
		{
			return;
		}

		if (HasAuthority())
		{
			const float SweepRadius = WeaponCollision ? WeaponCollision->GetScaledCapsuleRadius() : 20.f;
			TArray<FHitResult> SweepHits;
			FCollisionShape SphereShape = FCollisionShape::MakeSphere(SweepRadius);
			FCollisionQueryParams DamageParams;
			DamageParams.AddIgnoredActor(this);
			DamageParams.AddIgnoredActor(OwnerCharacter);

			GetWorld()->SweepMultiByChannel(
				SweepHits,
				CurrentLocation,
				NextLocation,
				FQuat::Identity,
				ECC_Pawn,
				SphereShape,
				DamageParams);

			bool bHitActorThisFrame = false;
			for (const FHitResult& SweepHit : SweepHits)
			{
				AActor* HitActor = SweepHit.GetActor();
				if (HitActor && !HasActorBeenHitThisSwing(HitActor))
				{
					UE_LOG(LogTemp, Warning, TEXT("[HammerThrowHit][Pawn] %s"), *BuildHammerHitLogString(SweepHit));
					MarkHammerHitSomething();
					MarkActorHitThisSwing(HitActor);
					ApplyDamageToActor(HitActor);
					bHitActorThisFrame = true;
				}
			}

			if (bHitActorThisFrame && !bHammerReturning)
			{
				BeginHammerReturn();
			}
		}

		AddActorWorldOffset(ThrowVelocity * DeltaTime);
	}
	else
	{
		const FName SocketName = ResolveEquipSocketName();
		USkeletalMeshComponent* AttachMesh = ResolveAttachMeshComponent(SocketName);

		FVector TargetLocation = GetActorLocation();
		if (AttachMesh && !SocketName.IsNone())
		{
			TargetLocation = AttachMesh->GetSocketLocation(SocketName);
		}

		const FVector ToTarget = TargetLocation - GetActorLocation();
		const float DistanceToTarget = ToTarget.Size();
		const float MaxReturnSpeed = FMath::Max(0.f, ThrowSpeed);
		if (DistanceToTarget <= ReturnArrivalDistance || MaxReturnSpeed <= KINDA_SMALL_NUMBER)
		{
			ReattachHammer();
			return;
		}

		const FVector ReturnDirection = ToTarget.GetSafeNormal();
		const FVector DesiredVelocity = ReturnDirection * MaxReturnSpeed;
		const float RecallInterpSpeed = RecallBrakeDuration > KINDA_SMALL_NUMBER ? 1.f / RecallBrakeDuration : MaxReturnSpeed;
		ThrowVelocity = FMath::VInterpTo(ThrowVelocity, DesiredVelocity, DeltaTime, RecallInterpSpeed);

		const float ReturnStepDistance = FMath::Min(ThrowVelocity.Size() * DeltaTime, DistanceToTarget);
		if (DistanceToTarget <= FMath::Max(ReturnArrivalDistance, ReturnStepDistance))
		{
			ReattachHammer();
			return;
		}

		const float SignedReturnSpeed = FVector::DotProduct(ThrowVelocity, ReturnDirection);
		const bool bReverseSpin = SignedReturnSpeed > 0.f;
		UpdateHammerSpin(FMath::Abs(SignedReturnSpeed), DeltaTime, bReverseSpin);
		ApplyHammerTravelRotation(ReturnDirection);

		const FVector TravelDirection = bReverseSpin ? ReturnDirection : ThrowVelocity.GetSafeNormal();
		AddActorWorldOffset(TravelDirection * ReturnStepDistance);
	}
}

void AHammerBase::SetOwnerCollisionIgnore(bool bIgnore)
{
	if (!OwnerCharacter)
	{
		return;
	}

	if (UCapsuleComponent* OwnerCapsule = OwnerCharacter->GetCapsuleComponent())
	{
		OwnerCapsule->IgnoreActorWhenMoving(this, bIgnore);
	}

	if (WeaponCollision)
	{
		WeaponCollision->IgnoreActorWhenMoving(OwnerCharacter, bIgnore);
	}

	SKEquipmentMesh->IgnoreActorWhenMoving(OwnerCharacter, bIgnore);
}

void AHammerBase::SyncHammerFlightTrailState()
{
	const bool bShouldEnableFlightTrail = bHammerThrown && !bHammerStuck;
	if (bHammerFlightTrailActive == bShouldEnableFlightTrail)
	{
		return;
	}

	SetWeaponTrailEnabled(bShouldEnableFlightTrail);
	SetHammerThrowVisualsEnabled(bShouldEnableFlightTrail);
	bHammerFlightTrailActive = bShouldEnableFlightTrail;
}

void AHammerBase::SetHammerThrowVisualsEnabled(bool bEnabled)
{
	if (HammerThrowNiagaraComponent)
	{
		HammerThrowNiagaraComponent->SetAsset(HammerThrowNiagaraSystem);

		const FName AttachSocketName =
			SKEquipmentMesh &&
			!HammerThrowNiagaraAttachSocket.IsNone() &&
			SKEquipmentMesh->DoesSocketExist(HammerThrowNiagaraAttachSocket)
				? HammerThrowNiagaraAttachSocket
				: NAME_None;

		HammerThrowNiagaraComponent->AttachToComponent(
			SKEquipmentMesh,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			AttachSocketName);

		if (bEnabled && HammerThrowNiagaraSystem)
		{
			HammerThrowNiagaraComponent->Activate(true);
		}
		else
		{
			HammerThrowNiagaraComponent->Deactivate();
		}
	}

	if (SKEquipmentMesh)
	{
		SKEquipmentMesh->SetOverlayMaterial(bEnabled ? HammerThrowOverlayMaterial : nullptr);
	}

	SetHammerThrowLoopSoundsEnabled(bEnabled);
}

void AHammerBase::SetHammerThrowLoopSoundsEnabled(bool bEnabled)
{
	const FName AttachSocketName =
		SKEquipmentMesh &&
		!HammerThrowNiagaraAttachSocket.IsNone() &&
		SKEquipmentMesh->DoesSocketExist(HammerThrowNiagaraAttachSocket)
			? HammerThrowNiagaraAttachSocket
			: NAME_None;

	if (HammerFlightLoopAudioComponent)
	{
		if (SKEquipmentMesh)
		{
			HammerFlightLoopAudioComponent->AttachToComponent(
				SKEquipmentMesh,
				FAttachmentTransformRules::SnapToTargetNotIncludingScale,
				AttachSocketName);
		}
		HammerFlightLoopAudioComponent->SetSound(HammerFlightLoopSound);
		HammerFlightLoopAudioComponent->SetVolumeMultiplier(HammerFlightLoopVolumeMultiplier);
		HammerFlightLoopAudioComponent->SetPitchMultiplier(HammerFlightLoopPitchMultiplier);
		HammerFlightLoopAudioComponent->AttenuationSettings = HammerThrowLoopAttenuationSettings ? HammerThrowLoopAttenuationSettings : SoundAttenuationSettings;

		if (bEnabled && HammerFlightLoopSound)
		{
			PlayHammerThrowLoopAudio(HammerFlightLoopAudioComponent, HammerFlightLoopSound, HammerFlightLoopVolumeMultiplier, HammerFlightLoopPitchMultiplier);
		}
		else
		{
			StopHammerThrowLoopAudio(HammerFlightLoopAudioComponent);
		}
	}

	if (HammerEffectLoopAudioComponent)
	{
		if (SKEquipmentMesh)
		{
			HammerEffectLoopAudioComponent->AttachToComponent(
				SKEquipmentMesh,
				FAttachmentTransformRules::SnapToTargetNotIncludingScale,
				AttachSocketName);
		}
		HammerEffectLoopAudioComponent->SetSound(HammerEffectLoopSound);
		HammerEffectLoopAudioComponent->SetVolumeMultiplier(HammerEffectLoopVolumeMultiplier);
		HammerEffectLoopAudioComponent->SetPitchMultiplier(HammerEffectLoopPitchMultiplier);
		HammerEffectLoopAudioComponent->AttenuationSettings = HammerThrowLoopAttenuationSettings ? HammerThrowLoopAttenuationSettings : SoundAttenuationSettings;

		if (bEnabled && HammerEffectLoopSound)
		{
			PlayHammerThrowLoopAudio(HammerEffectLoopAudioComponent, HammerEffectLoopSound, HammerEffectLoopVolumeMultiplier, HammerEffectLoopPitchMultiplier);
		}
		else
		{
			StopHammerThrowLoopAudio(HammerEffectLoopAudioComponent);
		}
	}
}

void AHammerBase::UpdateHammerThrowLoopSounds()
{
	if (!bHammerFlightTrailActive)
	{
		return;
	}

	PlayHammerThrowLoopAudio(HammerFlightLoopAudioComponent, HammerFlightLoopSound, HammerFlightLoopVolumeMultiplier, HammerFlightLoopPitchMultiplier);
	PlayHammerThrowLoopAudio(HammerEffectLoopAudioComponent, HammerEffectLoopSound, HammerEffectLoopVolumeMultiplier, HammerEffectLoopPitchMultiplier);
}

void AHammerBase::PlayHammerThrowLoopAudio(UAudioComponent* AudioComponent, USoundBase* Sound, float VolumeMultiplier, float PitchMultiplier) const
{
	if (!AudioComponent || !Sound || AudioComponent->IsPlaying())
	{
		return;
	}

	AudioComponent->SetSound(Sound);
	AudioComponent->SetVolumeMultiplier(VolumeMultiplier);
	AudioComponent->SetPitchMultiplier(PitchMultiplier);
	AudioComponent->AttenuationSettings = HammerThrowLoopAttenuationSettings ? HammerThrowLoopAttenuationSettings : SoundAttenuationSettings;
	AudioComponent->FadeIn(HammerThrowLoopFadeInTime, VolumeMultiplier);
}

void AHammerBase::StopHammerThrowLoopAudio(UAudioComponent* AudioComponent) const
{
	if (!AudioComponent || !AudioComponent->IsPlaying())
	{
		return;
	}

	AudioComponent->FadeOut(HammerThrowLoopFadeOutTime, 0.f);
}

void AHammerBase::OnEquip(ACharacter* NewOwner)
{
	Super::OnEquip(NewOwner);
	SetOwnerCollisionIgnore(true);
}

bool AHammerBase::OnUnequip()
{
	SetOwnerCollisionIgnore(false);
	return Super::OnUnequip();
}

void AHammerBase::CompleteUnequip()
{
	SetOwnerCollisionIgnore(false);
	Super::CompleteUnequip();
}

bool AHammerBase::CanStartAction(EEquipmentActionType ActionType) const
{
	if ((ActionType == EEquipmentActionType::Primary || ActionType == EEquipmentActionType::Ability) && bHammerThrown)
	{
		return true;
	}

	if (ActionType == EEquipmentActionType::Ability)
	{
		return OwnerCharacter != nullptr;
	}

	return Super::CanStartAction(ActionType);
}

bool AHammerBase::ShouldConsumeStaminaForAction(EEquipmentActionType ActionType) const
{
	return CanStartAction(ActionType) && GetStaminaCostForAction(ActionType) > KINDA_SMALL_NUMBER;
}

float AHammerBase::GetStaminaCostForAction(EEquipmentActionType ActionType) const
{
	if ((ActionType == EEquipmentActionType::Primary || ActionType == EEquipmentActionType::Ability) && bHammerThrown)
	{
		return 0.f;
	}

	return Super::GetStaminaCostForAction(ActionType);
}

void AHammerBase::StartAbilityAction()
{
	if (!bHammerThrown)
	{
		if (AMainCharacterBase* MainCharacter = Cast<AMainCharacterBase>(OwnerCharacter))
		{
			MainCharacter->RotateToControlYaw();
		}

		if (AbilityActionMontage && OwnerCharacter)
		{
			bHammerThrowPending = true;
			CurrentMontage = AbilityActionMontage;
			SetCurrentDamageMultiplier(AbilityActionDamageMultiplier);
			PlayOwnerMontage(AbilityActionMontage);
			return;
		}

		ThrowHammer(false);
	}
	else
	{
		RecallHammer();
	}

	if (!HasAuthority())
	{
		ServerStartHammerAbilityAction();
	}
}

void AHammerBase::StartPrimaryAction()
{
	if (bHammerThrown)
	{
		RecallHammer();
		if (!HasAuthority())
		{
			ServerStartHammerAbilityAction();
		}
		return;
	}

	Super::StartPrimaryAction();
}

void AHammerBase::OnMontageStarted(UAnimMontage* Montage)
{
	if (bHammerThrown && CurrentMontage && Montage != CurrentMontage)
	{
		return;
	}

	Super::OnMontageStarted(Montage);

	if (Montage == AbilityActionMontage && !bHammerThrown)
	{
		bHammerThrowPending = true;
		CurrentMontage = Montage;
		SetCurrentDamageMultiplier(AbilityActionDamageMultiplier);
	}
}

void AHammerBase::CancelAction()
{
	if (bHammerThrown)
	{
		if (CurrentMontage && OwnerCharacter)
		{
			StopOwnerMontage(CurrentMontage, 0.25f);
		}

		CurrentMontage = nullptr;
		ComboIndex = 0;
		return;
	}

	bHammerThrowPending = false;
	Super::CancelAction();
}

void AHammerBase::ForceResetState()
{
	if (bHammerThrown)
	{
		ReattachHammer();
	}

	bHammerThrowPending = false;
	Super::ForceResetState();
}

void AHammerBase::OnNaturalMontageEnd()
{
	bHammerThrowPending = false;

	if (!bHammerThrown)
	{
		Super::OnNaturalMontageEnd();
	}
}

void AHammerBase::OnInterruptedMontageEnd()
{
	bHammerThrowPending = false;

	if (!bHammerThrown)
	{
		Super::OnInterruptedMontageEnd();
	}
}

void AHammerBase::ServerStartHammerAbilityAction_Implementation()
{
	if (!bHammerThrown)
	{
		ThrowHammer(false);
	}
	else
	{
		RecallHammer();
	}
}

void AHammerBase::ServerHandleHammerThrowNotify_Implementation()
{
	HandleThrowHammer();
}

void AHammerBase::OnRep_HammerThrown()
{
	if (bHammerThrown)
	{
		DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

		if (SKEquipmentMesh)
		{
			SavedMeshCollisionEnabled = SKEquipmentMesh->GetCollisionEnabled();
			SKEquipmentMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}

		if (WeaponCollision)
		{
			WeaponCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		}
	}
	else
	{
		ReattachHammer();
	}

	SyncHammerFlightTrailState();
}

void AHammerBase::HandleThrowHammer()
{
	if (bHammerThrown || !OwnerCharacter)
	{
		return;
	}

	const bool bShouldExecuteThrow = HasAuthority() || OwnerCharacter->IsLocallyControlled();
	if (!bShouldExecuteThrow)
	{
		return;
	}

	if (!bHammerThrowPending && AbilityActionMontage)
	{
		return;
	}

	bHammerThrowPending = false;
	ThrowHammer(false);

	if (!HasAuthority())
	{
		ServerHandleHammerThrowNotify();
	}
}

void AHammerBase::ThrowHammer(bool bPlayMontage)
{
	if (!OwnerCharacter)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HammerThrowState][ThrowBlocked] Reason=NoOwner Weapon=%s"), *GetNameSafe(this));
		return;
	}

	SetCurrentDamageMultiplier(AbilityActionDamageMultiplier);

	FVector ThrowDir = OwnerCharacter->GetActorForwardVector();
	if (AController* C = OwnerCharacter->GetController())
	{
		const FRotator ControlRotation = C->GetControlRotation();
		// Use the aim direction for the throw without snapping the whole character,
		// which causes the spring arm to visibly jerk.
		ThrowDir = ControlRotation.Vector();
	}

	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
	SetReplicateMovement(true);
	HammerSpinAngleDegrees = 0.f;
	ApplyHammerTravelRotation(ThrowDir);

	if (SKEquipmentMesh)
	{
		SavedMeshCollisionEnabled = SKEquipmentMesh->GetCollisionEnabled();
		SKEquipmentMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	if (WeaponCollision)
	{
		WeaponCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	ResetSwingHitActors();

	ThrowVelocity = ThrowDir.GetSafeNormal() * FMath::Max(0.f, ThrowSpeed);
	bHammerThrown = true;
	bHammerReturning = false;
	bHammerStuck = false;
	bHitSomethingDuringThrow = false;
	bHammerThrowPending = false;
	SyncHammerFlightTrailState();

	if (bPlayMontage && AbilityActionMontage)
	{
		CurrentMontage = AbilityActionMontage;
		PlayOwnerMontage(AbilityActionMontage);
	}

	ScheduleMissAutoReturn();
	UE_LOG(LogTemp, Warning, TEXT("[HammerThrowState][Thrown] Weapon=%s Owner=%s Location=%s Direction=%s Speed=%.2f PlayMontage=%d HasAuthority=%d LocalOwner=%d"),
		*GetNameSafe(this),
		*GetNameSafe(OwnerCharacter),
		*GetActorLocation().ToCompactString(),
		*ThrowDir.GetSafeNormal().ToCompactString(),
		ThrowVelocity.Size(),
		bPlayMontage,
		HasAuthority(),
		OwnerCharacter->IsLocallyControlled());
	ForceNetUpdate();
}

void AHammerBase::RecallHammer()
{
	if (!bHammerThrown)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HammerThrowState][RecallBlocked] Reason=NotThrown Weapon=%s"), *GetNameSafe(this));
		return;
	}

	ClearMissAutoReturn();

	if (bHammerReturning)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HammerThrowState][RecallBlocked] Reason=AlreadyReturning Weapon=%s"), *GetNameSafe(this));
		return;
	}

	BeginHammerReturn();
}

bool AHammerBase::TryStickToWorld(const FVector& CurrentLocation, const FVector& NextLocation)
{
	const float SweepRadius = WeaponCollision ? WeaponCollision->GetScaledCapsuleRadius() : 20.f;
	FCollisionShape WorldShape = FCollisionShape::MakeSphere(SweepRadius);
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(this);
	Params.AddIgnoredActor(OwnerCharacter);

	FHitResult BestHit;
	bool bFoundHit = false;
	const auto KeepNearestWorldHit = [&BestHit, &bFoundHit](const FHitResult& Hit)
	{
		if (ShouldIgnoreHammerStickHit(Hit))
		{
			UE_LOG(LogTemp, Warning, TEXT("[HammerThrowHit][IgnoredStickHit] %s"), *BuildHammerHitLogString(Hit));
			return;
		}

		if (!bFoundHit || Hit.Time < BestHit.Time)
		{
			BestHit = Hit;
			bFoundHit = true;
		}
	};

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);

	FHitResult ObjectHit;
	if (GetWorld()->SweepSingleByObjectType(ObjectHit, CurrentLocation, NextLocation, FQuat::Identity, ObjectParams, WorldShape, Params))
	{
		KeepNearestWorldHit(ObjectHit);
	}

	TArray<FHitResult> VisibilityHits;
	if (GetWorld()->SweepMultiByChannel(VisibilityHits, CurrentLocation, NextLocation, FQuat::Identity, ECC_Visibility, WorldShape, Params))
	{
		for (const FHitResult& VisibilityHit : VisibilityHits)
		{
			KeepNearestWorldHit(VisibilityHit);
		}
	}

	if (!bFoundHit)
	{
		UE_LOG(LogTemp, VeryVerbose, TEXT("[HammerThrowHit][NoWorldHit] Weapon=%s From=%s To=%s Radius=%.2f"),
			*GetNameSafe(this),
			*CurrentLocation.ToCompactString(),
			*NextLocation.ToCompactString(),
			SweepRadius);
		return false;
	}

	MarkHammerHitSomething();
	SetActorLocation(BestHit.Location);
	bHammerStuck = true;
	UE_LOG(LogTemp, Warning, TEXT("[HammerThrowHit][StuckWorld] %s"), *BuildHammerHitLogString(BestHit));
	SyncHammerFlightTrailState();
	ForceNetUpdate();

	return true;
}

void AHammerBase::UpdateHammerSpin(float TravelSpeed, float DeltaTime, bool bReverseSpin)
{
	if (DeltaTime <= 0.f || TravelSpeed <= KINDA_SMALL_NUMBER || MeshSpinSpeed <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	const float SpeedScale = FMath::Max(0.f, ThrowSpeed) > KINDA_SMALL_NUMBER
		? TravelSpeed / FMath::Max(0.f, ThrowSpeed)
		: 0.f;
	const float SpinDirection = bReverseSpin ? -1.f : 1.f;
	const float SpinDegrees = MeshSpinSpeed * SpeedScale * DeltaTime * SpinDirection;
	if (FMath::Abs(SpinDegrees) <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	HammerSpinAngleDegrees = FRotator::ClampAxis(HammerSpinAngleDegrees + SpinDegrees);
}

void AHammerBase::ApplyHammerTravelRotation(const FVector& TravelDirection)
{
	const FVector NormalizedTravelDirection = TravelDirection.GetSafeNormal();
	if (NormalizedTravelDirection.IsNearlyZero())
	{
		return;
	}

	const FQuat TravelRotation = NormalizedTravelDirection.ToOrientationQuat();
	const FQuat OffsetRotation = ThrowRotationOffset.Quaternion();
	const FQuat SpinRotation = FRotator(HammerSpinAngleDegrees, 0.f, 0.f).Quaternion();

	SetActorRotation((TravelRotation * OffsetRotation * SpinRotation).Rotator());
}

void AHammerBase::ScheduleMissAutoReturn()
{
	ClearMissAutoReturn();

	if (!HasAuthority() || MissAutoReturnDelay <= KINDA_SMALL_NUMBER)
	{
		return;
	}

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			MissAutoReturnTimerHandle,
			this,
			&AHammerBase::HandleMissAutoReturn,
			MissAutoReturnDelay,
			false);
	}
}

void AHammerBase::ClearMissAutoReturn()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(MissAutoReturnTimerHandle);
	}
}

void AHammerBase::HandleMissAutoReturn()
{
	if (!bHammerThrown || bHammerReturning || bHammerStuck || bHitSomethingDuringThrow)
	{
		UE_LOG(LogTemp, Warning, TEXT("[HammerThrowState][MissAutoReturnSkipped] Weapon=%s Thrown=%d Returning=%d Stuck=%d HitSomething=%d"),
			*GetNameSafe(this),
			bHammerThrown,
			bHammerReturning,
			bHammerStuck,
			bHitSomethingDuringThrow);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[HammerThrowState][MissAutoReturn] Weapon=%s Location=%s Velocity=%s"),
		*GetNameSafe(this),
		*GetActorLocation().ToCompactString(),
		*ThrowVelocity.ToCompactString());
	BeginHammerReturn();
}

void AHammerBase::MarkHammerHitSomething()
{
	bHitSomethingDuringThrow = true;
	ClearMissAutoReturn();
}

void AHammerBase::BeginHammerReturn()
{
	const bool bWasStuck = bHammerStuck;

	ClearMissAutoReturn();
	bHammerStuck = false;
	bHammerReturning = true;
	if (bWasStuck || ThrowVelocity.IsNearlyZero())
	{
		ThrowVelocity = FVector::ZeroVector;
	}

	ResetSwingHitActors();
	HandleEnableCollision();
	SyncHammerFlightTrailState();
	UE_LOG(LogTemp, Warning, TEXT("[HammerThrowState][BeginReturn] Weapon=%s WasStuck=%d Location=%s Velocity=%s"),
		*GetNameSafe(this),
		bWasStuck,
		*GetActorLocation().ToCompactString(),
		*ThrowVelocity.ToCompactString());
	ForceNetUpdate();
}

void AHammerBase::ReattachHammer()
{
	ClearMissAutoReturn();
	bHammerThrown = false;
	bHammerReturning = false;
	bHammerStuck = false;
	ThrowVelocity = FVector::ZeroVector;
	bHitSomethingDuringThrow = false;
	HammerSpinAngleDegrees = 0.f;
	SetReplicateMovement(false);
	SyncHammerFlightTrailState();

	if (SKEquipmentMesh)
	{
		SKEquipmentMesh->SetCollisionEnabled(SavedMeshCollisionEnabled);
	}

	AttachEquipmentToOwner();
	HandleDisableCollision();
	UE_LOG(LogTemp, Warning, TEXT("[HammerThrowState][Reattached] Weapon=%s Owner=%s Location=%s"),
		*GetNameSafe(this),
		*GetNameSafe(OwnerCharacter),
		*GetActorLocation().ToCompactString());
	ForceNetUpdate();
}

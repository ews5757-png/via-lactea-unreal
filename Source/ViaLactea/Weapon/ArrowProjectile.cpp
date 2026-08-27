// Fill out your copyright notice in the Description page of Project Settings.

#include "ArrowProjectile.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/DamageType.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Materials/MaterialInterface.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "UObject/ConstructorHelpers.h"
#include "Net/UnrealNetwork.h"

AArrowProjectile::AArrowProjectile()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = true;
	SetReplicateMovement(true);

	// 작은 구체 콜리전을 루트로 설정 (기본 OFF — Fire() 호출 시 활성화)
	ArrowRoot = CreateDefaultSubobject<USphereComponent>(TEXT("ArrowRoot"));
	ArrowRoot->InitSphereRadius(5.f);
	ArrowRoot->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ArrowRoot->SetCollisionObjectType(ECC_WorldDynamic);
	ArrowRoot->SetCollisionResponseToAllChannels(ECR_Block);
	ArrowRoot->SetCollisionResponseToChannel(ECC_Camera, ECR_Ignore);
	ArrowRoot->SetNotifyRigidBodyCollision(true);
	ArrowRoot->OnComponentHit.AddDynamic(this, &AArrowProjectile::OnHit);
	SetRootComponent(ArrowRoot);

	// 화살 메시는 루트 자식으로 붙임 (콜리전 없음, 상대 회전만 담당)
	ArrowMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ArrowMesh"));
	ArrowMesh->SetupAttachment(ArrowRoot);
	ArrowMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ArrowMesh->SetRelativeRotation(MeshRotationOffset);
	ArrowMesh->SetRelativeLocation(MeshLocationOffset);

	ArrowTrail = CreateDefaultSubobject<UNiagaraComponent>(TEXT("ArrowTrail"));
	ArrowTrail->SetupAttachment(ArrowRoot);
	ArrowTrail->SetAutoActivate(false);
	ArrowTrail->SetUsingAbsoluteRotation(true);

	ArrowTrailMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ArrowTrailMesh"));
	ArrowTrailMesh->SetupAttachment(ArrowRoot);
	ArrowTrailMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ArrowTrailMesh->SetCastShadow(false);
	ArrowTrailMesh->SetHiddenInGame(true);
	ArrowTrailMesh->SetVisibility(false);
	ArrowTrailMesh->SetRelativeScale3D(FVector(1.5F, 1.5f, 1.5f));

	ArrowFlightLoopAudioComponent = CreateDefaultSubobject<UAudioComponent>(TEXT("ArrowFlightLoopAudioComponent"));
	ArrowFlightLoopAudioComponent->SetupAttachment(ArrowRoot);
	ArrowFlightLoopAudioComponent->SetAutoActivate(false);
	ArrowFlightLoopAudioComponent->bAutoActivate = false;

	ArrowFlightLoopAudioComponentB = CreateDefaultSubobject<UAudioComponent>(TEXT("ArrowFlightLoopAudioComponentB"));
	ArrowFlightLoopAudioComponentB->SetupAttachment(ArrowRoot);
	ArrowFlightLoopAudioComponentB->SetAutoActivate(false);
	ArrowFlightLoopAudioComponentB->bAutoActivate = false;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> DefaultArrowTrailMesh(
		TEXT("/Game/Asset/Monster/RPGEffects/Meshes/SM_Arrow_Trail.SM_Arrow_Trail"));
	if (DefaultArrowTrailMesh.Succeeded())
	{
		ArrowTrailMesh->SetStaticMesh(DefaultArrowTrailMesh.Object);
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> DefaultArrowTrailMaterial(
		TEXT("/Game/Asset/Monster/RPGEffects/Materials/M_Arrow_Trail.M_Arrow_Trail"));
	if (DefaultArrowTrailMaterial.Succeeded())
	{
		ArrowTrailMesh->SetMaterial(0, DefaultArrowTrailMaterial.Object);
	}

	// 투사체 이동 컴포넌트
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
	ProjectileMovement->bAutoActivate = true;
	ProjectileMovement->InitialSpeed = 0.f;
	ProjectileMovement->MaxSpeed = 5000.f;
	ProjectileMovement->bRotationFollowsVelocity = true;
	ProjectileMovement->bShouldBounce = false;
	ProjectileMovement->ProjectileGravityScale = 0.9f;
}

void AArrowProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AArrowProjectile, bHidePreviewFromOwner);
	DOREPLIFETIME(AArrowProjectile, bArrowInFlight);
}

void AArrowProjectile::BeginPlay()
{
	Super::BeginPlay();

	if (ArrowTrailSystem)
	{
		ArrowTrail->SetAsset(ArrowTrailSystem);
	}

	// UpdatedComponent 정상 세팅 후 즉시 정지 및 비활성화
	ProjectileMovement->SetUpdatedComponent(ArrowRoot);
	ProjectileMovement->StopMovementImmediately();
	ProjectileMovement->SetActive(false);
	SetActorTickEnabled(false);
	ArrowTrail->DeactivateImmediate();
	ArrowTrailMesh->SetHiddenInGame(true);
	ArrowTrailMesh->SetVisibility(false);
	StopArrowFlightLoopSound();
	ApplyPreviewOwnerVisibility();
}

void AArrowProjectile::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	StopArrowFlightLoopSound();
	Super::EndPlay(EndPlayReason);
}

void AArrowProjectile::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	UpdateArrowFlightLoopSound();
}

void AArrowProjectile::Fire(const FVector& Direction, float Speed)
{
	SetHidePreviewFromOwner(false);
	bHasHitSomething = false;
	bArrowInFlight = true;
	NextArrowFlightLoopRestartTime = 0.f;

	// 소켓에서 분리
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);

	// 콜리전 활성화
	ArrowRoot->SetCollisionEnabled(ECollisionEnabled::QueryOnly);

	ApplyArrowFlightState();

	// 발사자(캐릭터) 와 충돌 무시
	if (AActor* MyOwner = GetOwner())
	{
		ArrowRoot->IgnoreActorWhenMoving(MyOwner, true);

		if (UPrimitiveComponent* OwnerRoot = Cast<UPrimitiveComponent>(MyOwner->GetRootComponent()))
		{
			OwnerRoot->IgnoreActorWhenMoving(this, true);
		}

		if (ACharacter* OwnerCharacter = Cast<ACharacter>(MyOwner))
		{
			if (UCapsuleComponent* Capsule = OwnerCharacter->GetCapsuleComponent())
			{
				Capsule->IgnoreActorWhenMoving(this, true);
			}

			TArray<USkeletalMeshComponent*> SkeletalMeshComponents;
			OwnerCharacter->GetComponents(SkeletalMeshComponents);
			for (USkeletalMeshComponent* SkeletalMeshComponent : SkeletalMeshComponents)
			{
				if (!SkeletalMeshComponent)
				{
					continue;
				}

				SkeletalMeshComponent->IgnoreActorWhenMoving(this, true);
			}
		}
	}

	// 속도 먼저 설정 후 활성화
	ProjectileMovement->InitialSpeed = Speed;
	ProjectileMovement->MaxSpeed = Speed;
	ProjectileMovement->Velocity = Direction.GetSafeNormal() * Speed;
	ProjectileMovement->SetActive(true);
}

void AArrowProjectile::SetHidePreviewFromOwner(bool bNewHidePreviewFromOwner)
{
	if (bHidePreviewFromOwner == bNewHidePreviewFromOwner)
	{
		return;
	}

	bHidePreviewFromOwner = bNewHidePreviewFromOwner;
	ApplyPreviewOwnerVisibility();
	ForceNetUpdate();
}

void AArrowProjectile::OnRep_HidePreviewFromOwner()
{
	ApplyPreviewOwnerVisibility();
}

void AArrowProjectile::OnRep_ArrowInFlight()
{
	ApplyArrowFlightState();
}

void AArrowProjectile::ApplyPreviewOwnerVisibility()
{
	if (ArrowMesh)
	{
		ArrowMesh->SetOwnerNoSee(bHidePreviewFromOwner);
	}

	if (ArrowTrail)
	{
		ArrowTrail->SetOwnerNoSee(bHidePreviewFromOwner);
	}

	if (ArrowTrailMesh)
	{
		ArrowTrailMesh->SetOwnerNoSee(bHidePreviewFromOwner);
	}
}

void AArrowProjectile::OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit)
{
	if (bHasHitSomething)
	{
		return;
	}

	AActor* MyOwner = GetOwner();

	if (!OtherActor || OtherActor == this || OtherActor == MyOwner)
	{
		return;
	}

	bHasHitSomething = true;
	bArrowInFlight = false;

	if (HasAuthority() && MyOwner)
	{
		AController* InstigatorController = MyOwner->GetInstigatorController();
		UGameplayStatics::ApplyPointDamage(
			OtherActor,
			Damage,
			Hit.ImpactNormal,
			Hit,
			InstigatorController,
			this,
			UDamageType::StaticClass()
		);
	}

	const FVector ImpactDirection = ProjectileMovement && !ProjectileMovement->Velocity.IsNearlyZero()
		? ProjectileMovement->Velocity.GetSafeNormal()
		: GetActorForwardVector();

	// 명중 후 그 자리에 정지하고, 일정 시간 뒤 삭제
	ProjectileMovement->StopMovementImmediately();
	ProjectileMovement->SetActive(false);
	ApplyArrowFlightState();
	ArrowRoot->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	StickToHitTarget(OtherComp, Hit, ImpactDirection);
	SetLifeSpan(DestroyDelayAfterHit);
}

void AArrowProjectile::UpdateArrowFlightLoopSound()
{
	if (!bArrowInFlight || bHasHitSomething)
	{
		return;
	}

	const UWorld* World = GetWorld();
	if (!World || NextArrowFlightLoopRestartTime <= 0.f || World->GetTimeSeconds() >= NextArrowFlightLoopRestartTime)
	{
		StartArrowFlightLoopSound();
	}
}

void AArrowProjectile::ApplyArrowFlightState()
{
	if (bArrowInFlight)
	{
		if (ArrowTrailSystem)
		{
			ArrowTrail->SetAsset(ArrowTrailSystem);
		}
		else
		{
			ArrowTrail->DeactivateImmediate();
		}

		ArrowTrailMesh->SetHiddenInGame(false);
		ArrowTrailMesh->SetVisibility(true);
		if (ArrowTrailSystem)
		{
			ArrowTrail->Activate(true);
		}

		StartArrowFlightLoopSound();
		SetActorTickEnabled(ArrowFlightLoopSoundCue != nullptr);
		return;
	}

	SetActorTickEnabled(false);
	if (ArrowTrail)
	{
		ArrowTrail->Deactivate();
	}
	if (ArrowTrailMesh)
	{
		ArrowTrailMesh->SetHiddenInGame(true);
		ArrowTrailMesh->SetVisibility(false);
	}
	StopArrowFlightLoopSound();
}

void AArrowProjectile::StartArrowFlightLoopSound()
{
	if (!ArrowFlightLoopAudioComponent || !ArrowFlightLoopAudioComponentB || !ArrowFlightLoopSoundCue)
	{
		return;
	}

	UAudioComponent* NewComponent = bUseFirstArrowFlightLoopAudioComponent ? ArrowFlightLoopAudioComponent : ArrowFlightLoopAudioComponentB;
	UAudioComponent* OldComponent = bUseFirstArrowFlightLoopAudioComponent ? ArrowFlightLoopAudioComponentB : ArrowFlightLoopAudioComponent;

	StartArrowFlightLoopOnComponent(NewComponent);

	if (OldComponent && OldComponent->IsPlaying())
	{
		OldComponent->FadeOut(ArrowFlightLoopFadeOutTime, 0.f);
	}

	bUseFirstArrowFlightLoopAudioComponent = !bUseFirstArrowFlightLoopAudioComponent;

	const float SoundDuration = ArrowFlightLoopSoundCue->GetDuration();
	if (FMath::IsFinite(SoundDuration) && SoundDuration > KINDA_SMALL_NUMBER && SoundDuration < 1000.f)
	{
		const float NextDelay = FMath::Max(0.01f, SoundDuration * FMath::Clamp(ArrowFlightLoopRestartAlpha, 0.01f, 0.99f));
		NextArrowFlightLoopRestartTime = GetWorld() ? GetWorld()->GetTimeSeconds() + NextDelay : 0.f;
	}
	else
	{
		NextArrowFlightLoopRestartTime = 0.f;
	}
}

void AArrowProjectile::StartArrowFlightLoopOnComponent(UAudioComponent* AudioComponent)
{
	if (!AudioComponent || !ArrowFlightLoopSoundCue)
	{
		return;
	}

	if (AudioComponent->IsPlaying())
	{
		AudioComponent->Stop();
	}

	AudioComponent->SetSound(ArrowFlightLoopSoundCue);
	AudioComponent->SetPitchMultiplier(ArrowFlightLoopPitchMultiplier);
	AudioComponent->AttenuationSettings = ArrowFlightLoopAttenuationSettings;
	AudioComponent->FadeIn(ArrowFlightLoopFadeInTime, ArrowFlightLoopVolumeMultiplier);
}

void AArrowProjectile::StopArrowFlightLoopSound()
{
	NextArrowFlightLoopRestartTime = 0.f;

	if (ArrowFlightLoopAudioComponent && ArrowFlightLoopAudioComponent->IsPlaying())
	{
		ArrowFlightLoopAudioComponent->FadeOut(ArrowFlightLoopFadeOutTime, 0.f);
	}

	if (ArrowFlightLoopAudioComponentB && ArrowFlightLoopAudioComponentB->IsPlaying())
	{
		ArrowFlightLoopAudioComponentB->FadeOut(ArrowFlightLoopFadeOutTime, 0.f);
	}
}

void AArrowProjectile::StickToHitTarget(UPrimitiveComponent* HitComponent, const FHitResult& Hit, const FVector& ImpactDirection)
{
	const FVector StickDirection = ImpactDirection.IsNearlyZero() ? GetActorForwardVector() : ImpactDirection;
	const FVector StickLocation = Hit.ImpactPoint - (StickDirection * StickDepth);
	const FRotator StickRotation = StickDirection.Rotation();

	SetActorLocationAndRotation(
		StickLocation,
		StickRotation,
		false,
		nullptr,
		ETeleportType::TeleportPhysics);

	if (!HitComponent)
	{
		return;
	}

	UPrimitiveComponent* AttachComponent = HitComponent;
	FName AttachPoint = NAME_None;

	if (USkinnedMeshComponent* SkinnedMeshComponent = Cast<USkinnedMeshComponent>(HitComponent))
	{
		AttachPoint = Hit.BoneName != NAME_None
			? Hit.BoneName
			: SkinnedMeshComponent->FindClosestBone(Hit.ImpactPoint);
	}
	else if (AActor* HitActor = Hit.GetActor())
	{
		TArray<USkinnedMeshComponent*> SkinnedMeshComponents;
		HitActor->GetComponents(SkinnedMeshComponents);

		float BestDistanceSq = TNumericLimits<float>::Max();
		for (USkinnedMeshComponent* CandidateMeshComponent : SkinnedMeshComponents)
		{
			if (!CandidateMeshComponent)
			{
				continue;
			}

			FVector BoneLocation = FVector::ZeroVector;
			const FName ClosestBone = CandidateMeshComponent->FindClosestBone(Hit.ImpactPoint, &BoneLocation);
			if (ClosestBone == NAME_None)
			{
				continue;
			}

			const float DistanceSq = FVector::DistSquared(Hit.ImpactPoint, BoneLocation);
			if (DistanceSq < BestDistanceSq)
			{
				BestDistanceSq = DistanceSq;
				AttachComponent = CandidateMeshComponent;
				AttachPoint = ClosestBone;
			}
		}
	}

	AttachToComponent(AttachComponent, FAttachmentTransformRules::KeepWorldTransform, AttachPoint);
}

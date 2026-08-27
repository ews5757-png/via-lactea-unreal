// Fill out your copyright notice in the Description page of Project Settings.


#include "CharacterEquipmentBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "Animation/AnimInstance.h"
#include "Player/MainCharacterBase.h"
#include "Net/UnrealNetwork.h"

// Sets default values
ACharacterEquipmentBase::ACharacterEquipmentBase()
{
 	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
	bReplicates = true;
	bNetUseOwnerRelevancy = true;
	SetReplicateMovement(false);


	SKEquipmentMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("SKEquipmentMesh"));
	SetRootComponent(SKEquipmentMesh);
	//SKEquipmentMesh->SetupAttachment(GetMesh(), TEXT("pelvis"));
	SKEquipmentMesh->SetRelativeLocation(EquipmentLocation);
	SKEquipmentMesh->SetRelativeRotation(EquipmentRotation);

}

void ACharacterEquipmentBase::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ACharacterEquipmentBase, bEquipmentVisible);
}

// Called when the game starts or when spawned
void ACharacterEquipmentBase::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = ResolveOwnerCharacter();
	ApplyEquipmentVisibility();
}

void ACharacterEquipmentBase::OnRep_Owner()
{
	Super::OnRep_Owner();

	OwnerCharacter = ResolveOwnerCharacter();

	if (OwnerCharacter)
	{
		AttachEquipmentToOwner();
	}

	ApplyEquipmentVisibility();
}
// Called every frame
void ACharacterEquipmentBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

ACharacter* ACharacterEquipmentBase::ResolveOwnerCharacter() const
{
	if (OwnerCharacter)
	{
		return OwnerCharacter;
	}

	return Cast<ACharacter>(GetOwner());
}

FName ACharacterEquipmentBase::ResolveEquipSocketName() const
{
	switch (EquipmentType)
	{
	case EEquipmentSlotType::RightHand:
	case EEquipmentSlotType::TwoHanded:
		return TEXT("hand_r_Socket");
	case EEquipmentSlotType::LeftHand:
	case EEquipmentSlotType::Bow:
		return TEXT("weapon_l_Socket");
	case EEquipmentSlotType::Armor:
		return TEXT("lowerarm_l_Socket");
	case EEquipmentSlotType::Shield:
		return TEXT("lowerarm_l_Socket");
	default:
		return NAME_None;
	}
}

USkeletalMeshComponent* ACharacterEquipmentBase::ResolveAttachMeshComponent(FName SocketName) const
{
	ACharacter* ResolvedOwnerCharacter = ResolveOwnerCharacter();
	if (!ResolvedOwnerCharacter)
	{
		return nullptr;
	}

	USkeletalMeshComponent* OwnerMesh = ResolvedOwnerCharacter->GetMesh();
	if (!OwnerMesh)
	{
		return nullptr;
	}

	USkeletalMeshComponent* AttachTargetMesh = OwnerMesh;
	TArray<USkeletalMeshComponent*> SkeletalMeshComponents;
	ResolvedOwnerCharacter->GetComponents(SkeletalMeshComponents);

	for (USkeletalMeshComponent* SkeletalMeshComponent : SkeletalMeshComponents)
	{
		if (!SkeletalMeshComponent || SkeletalMeshComponent == OwnerMesh)
		{
			continue;
		}

		if (!SkeletalMeshComponent->IsAttachedTo(OwnerMesh))
		{
			continue;
		}

		if (!SocketName.IsNone() && !SkeletalMeshComponent->DoesSocketExist(SocketName))
		{
			continue;
		}

		AttachTargetMesh = SkeletalMeshComponent;
		break;
	}

	return AttachTargetMesh;
}

void ACharacterEquipmentBase::ApplyEquipmentVisibility()
{
	SetActorHiddenInGame(!bEquipmentVisible);
	SetActorEnableCollision(bEquipmentVisible);

	if (SKEquipmentMesh)
	{
		SKEquipmentMesh->SetVisibility(bEquipmentVisible, true);
	}
}

void ACharacterEquipmentBase::OnRep_EquipmentVisible()
{
	ApplyEquipmentVisibility();
}

float ACharacterEquipmentBase::PlayOwnerMontage(UAnimMontage* Montage, float PlayRate, FName StartSection, bool bFreezeAtEnd, bool bStopAllMontages) const
{
	ACharacter* ResolvedOwnerCharacter = ResolveOwnerCharacter();
	if (!ResolvedOwnerCharacter || !Montage)
	{
		return 0.f;
	}

	if (const AMainCharacterBase* MainCharacter = Cast<AMainCharacterBase>(ResolvedOwnerCharacter))
	{
		return const_cast<AMainCharacterBase*>(MainCharacter)->PlayNetworkedMontage(Montage, PlayRate, StartSection, bFreezeAtEnd, true, bStopAllMontages);
	}

	UAnimInstance* AnimInstance = ResolvedOwnerCharacter->GetMesh() ? ResolvedOwnerCharacter->GetMesh()->GetAnimInstance() : nullptr;
	if (!AnimInstance)
	{
		return 0.f;
	}

	const float Duration = AnimInstance->Montage_Play(
		Montage,
		PlayRate,
		EMontagePlayReturnType::MontageLength,
		0.f,
		bStopAllMontages);

	if (Duration > 0.f && StartSection != NAME_None)
	{
		AnimInstance->Montage_JumpToSection(StartSection, Montage);
	}

	return Duration;
}

void ACharacterEquipmentBase::StopOwnerMontage(UAnimMontage* Montage, float BlendOutTime) const
{
	ACharacter* ResolvedOwnerCharacter = ResolveOwnerCharacter();
	if (!ResolvedOwnerCharacter || !Montage)
	{
		return;
	}

	if (const AMainCharacterBase* MainCharacter = Cast<AMainCharacterBase>(ResolvedOwnerCharacter))
	{
		const_cast<AMainCharacterBase*>(MainCharacter)->StopNetworkedMontage(Montage, BlendOutTime);
		return;
	}

	if (UAnimInstance* AnimInstance = ResolvedOwnerCharacter->GetMesh() ? ResolvedOwnerCharacter->GetMesh()->GetAnimInstance() : nullptr)
	{
		AnimInstance->Montage_Stop(BlendOutTime, Montage);
	}
}

void ACharacterEquipmentBase::OnEquip(ACharacter* NewOwner)
{
	SetOwner(NewOwner);
	OwnerCharacter = NewOwner;

	if (OwnerCharacter && EquipMontage)
	{
		PlayOwnerMontage(EquipMontage);
	}
}

bool ACharacterEquipmentBase::OnUnequip()
{
	if (OwnerCharacter && UnequipMontage)
	{
		PlayOwnerMontage(UnequipMontage);
		return false; // 몽타주 있음 → 노티파이 대기
	}
	return true; // 몽타주 없음 → 즉시 완료
}

void ACharacterEquipmentBase::CompleteUnequip()
{
	EquippedItemID = NAME_None;
	OwnerCharacter = nullptr;
	Destroy();
}

void ACharacterEquipmentBase::AttachEquipmentToOwner()
{
	OwnerCharacter = ResolveOwnerCharacter();
	if (!OwnerCharacter)
	{
		return;
	}

	FName SocketName = ResolveEquipSocketName();
	USkeletalMeshComponent* AttachTargetMesh = ResolveAttachMeshComponent(SocketName);
	if (!AttachTargetMesh)
	{
		return;
	}

	AttachToComponent(
		AttachTargetMesh,
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		SocketName);

	if (SKEquipmentMesh)
	{
		SKEquipmentMesh->SetRelativeLocation(EquipmentLocation);
		SKEquipmentMesh->SetRelativeRotation(EquipmentRotation);
	}
}

void ACharacterEquipmentBase::DetachEquipmentFromOwner()
{
	DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
}

void ACharacterEquipmentBase::ShowEquipment()
{
	bEquipmentVisible = true;
	ApplyEquipmentVisibility();
}

void ACharacterEquipmentBase::HideEquipment()
{
	bEquipmentVisible = false;
	ApplyEquipmentVisibility();
}

bool ACharacterEquipmentBase::CanStartAction(EEquipmentActionType ActionType) const
{
	return false;
}

bool ACharacterEquipmentBase::ShouldConsumeStaminaForAction(EEquipmentActionType ActionType) const
{
	return GetStaminaCostForAction(ActionType) > KINDA_SMALL_NUMBER;
}

float ACharacterEquipmentBase::GetStaminaCostForAction(EEquipmentActionType ActionType) const
{
	return 0.f;
}

void ACharacterEquipmentBase::StartAction(EEquipmentActionType ActionType)
{
	switch (ActionType)
	{
	case EEquipmentActionType::Primary:
		StartPrimaryAction();
		break;
	case EEquipmentActionType::Secondary:
		StartSecondaryAction();
		break;
	case EEquipmentActionType::Ability:
		StartAbilityAction();
		break;
	default:
		break;
	}
}

void ACharacterEquipmentBase::StopAction(EEquipmentActionType ActionType)
{
	switch (ActionType)
	{
	case EEquipmentActionType::Primary:
		StopPrimaryAction();
		break;
	case EEquipmentActionType::Secondary:
		StopSecondaryAction();
		break;
	case EEquipmentActionType::Ability:
		StopAbilityAction();
		break;
	default:
		break;
	}
}


void ACharacterEquipmentBase::HandleAction(EEquipMentHandleAction Action)
{
	switch (Action)
	{
	case EEquipMentHandleAction::EnableCollision:
		HandleEnableCollision();
		break;
	case EEquipMentHandleAction::DisableCollision:
		HandleDisableCollision();
		break;
	case EEquipMentHandleAction::CanAction:
		HandleCanAction();
		break;
	case EEquipMentHandleAction::ResetHitActors:
		HandleResetHitActors();
		break;
	case EEquipMentHandleAction::FireBowArrow:
		HandleFireBowArrow();
		break;
	case EEquipMentHandleAction::ThrowHammer:
		HandleThrowHammer();
		break;
	case EEquipMentHandleAction::PlaySwingSound:
		HandlePlaySwingSound();
		break;
	case EEquipMentHandleAction::PlayEquipSound:
		HandlePlayEquipSound();
		break;
	case EEquipMentHandleAction::PlayUnequipSound:
		HandlePlayUnequipSound();
		break;
	case EEquipMentHandleAction::AttachPendingEquipment:
		AttachEquipmentToOwner();
		ShowEquipment();
		break;
	default:
		break;
	}
}

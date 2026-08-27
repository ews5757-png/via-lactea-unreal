// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Player/CharacterEnums.h"
#include "CharacterEquipmentBase.generated.h"

UCLASS()
class VIALACTEA_API ACharacterEquipmentBase : public AActor
{
	GENERATED_BODY()
	
public:

	ACharacterEquipmentBase();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void OnRep_Owner() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Equipment|State")
	class ACharacter* OwnerCharacter;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Equipment|State")
	FName EquippedItemID = NAME_None;

	virtual void OnEquip(ACharacter* NewOwner);

	virtual bool OnUnequip();

	virtual void CompleteUnequip();

	virtual bool CanStartAction(EEquipmentActionType ActionType) const;

	virtual bool ShouldConsumeStaminaForAction(EEquipmentActionType ActionType) const;

	virtual float GetStaminaCostForAction(EEquipmentActionType ActionType) const;

	virtual void StartAction(EEquipmentActionType ActionType);

	virtual void StopAction(EEquipmentActionType ActionType);

	virtual void HandleAction(EEquipMentHandleAction Action);



	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Animation")
	class UAnimMontage* EquipMontage;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Animation")
	class UAnimMontage* UnequipMontage;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Equipment|Mesh")
	class USkeletalMeshComponent* SKEquipmentMesh;

	void AttachEquipmentToOwner();
	void DetachEquipmentFromOwner();
	void ShowEquipment();
	void HideEquipment();


protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

	class ACharacter* ResolveOwnerCharacter() const;
	FName ResolveEquipSocketName() const;
	class USkeletalMeshComponent* ResolveAttachMeshComponent(FName SocketName) const;
	float PlayOwnerMontage(class UAnimMontage* Montage, float PlayRate = 1.f, FName StartSection = NAME_None, bool bFreezeAtEnd = false, bool bStopAllMontages = true) const;
	void StopOwnerMontage(class UAnimMontage* Montage, float BlendOutTime = 0.25f) const;
	void ApplyEquipmentVisibility();

	UFUNCTION()
	void OnRep_EquipmentVisible();

	virtual void StartPrimaryAction() {};

	virtual void StartSecondaryAction() {};

	virtual void StartAbilityAction() {};

	virtual void StopPrimaryAction() {};

	virtual void StopSecondaryAction() {};

	virtual void StopAbilityAction() {};

	virtual void HandleEnableCollision() {};

	virtual void HandleDisableCollision() {};

	virtual void HandleCanAction() {};

	virtual void HandleResetHitActors() {};

	virtual void HandleFireBowArrow() {};

	virtual void HandleThrowHammer() {};

	virtual void HandlePlaySwingSound() {};

	virtual void HandlePlayEquipSound() {};

	virtual void HandlePlayUnequipSound() {};

public:
	// 이동으로 인한 강제 중단 (각 무기가 오버라이드해서 처리)
	virtual void CancelAction() {};	
	// Called every frame
	virtual void Tick(float DeltaTime) override;


	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|State", meta = (DisplayName = "Equipment Slot Type"))
	EEquipmentSlotType EquipmentType = EEquipmentSlotType::RightHand;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|State")
	FVector EquipmentLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|State")
	FRotator EquipmentRotation = FRotator::ZeroRotator;

	UPROPERTY(ReplicatedUsing = OnRep_EquipmentVisible, VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|State")
	bool bEquipmentVisible = true;

};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "CharacterEnums.h"

#include "AnimNotifyWeaponAction.generated.h"

UCLASS()
class VIALACTEA_API UAnimNotifyWeaponAction : public UAnimNotify
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Notify")
	EEquipMentHandleAction Action;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Notify")
	EWeaponTarget Target = EWeaponTarget::Both;

	virtual void Notify(
		USkeletalMeshComponent* MeshComp,
		UAnimSequenceBase* Animation,
		const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override;
};

// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "CharacterWeaponBase.h"
#include "MeleeWeaponBase.generated.h"

/**
 * 
 */
UCLASS()
class VIALACTEA_API AMeleeWeaponBase : public ACharacterWeaponBase
{
	GENERATED_BODY()

public:
	AMeleeWeaponBase();
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void Tick(float DeltaTime) override;
	virtual void StartPrimaryAction() override;
	virtual void StartSecondaryAction() override;
	virtual void StartAbilityAction() override;
	virtual void ForceResetState() override;
	virtual void OnMontageStarted(UAnimMontage* Montage) override;
	virtual void OnNaturalMontageEnd() override;
	virtual void OnInterruptedMontageEnd() override;

protected:
	virtual void HandleEnableCollision() override;
	virtual void HandleDisableCollision() override;
	virtual void HandleCanAction() override;
	virtual void HandleResetHitActors() override;

	UFUNCTION()
	void OnWeaponBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult);

	void ResetSwingHitActors();
	bool HasActorBeenHitThisSwing(AActor* OtherActor) const;
	void MarkActorHitThisSwing(AActor* OtherActor);
	void UpdateWeaponCollisionShape();
	void CheckOverlapCandidatesDuringDamageWindow();
	void TryApplyDamageToActor(AActor* OtherActor, UPrimitiveComponent* HitComponent = nullptr, UPrimitiveComponent* SourceComponent = nullptr);


public:

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|Collision")
	class UCapsuleComponent* WeaponCollision;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|Collision")
	float WeaponCollisionRadius;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|Collision")
	float WeaponCollisionHalfHeight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|Collision")
	FVector WeaponCollisionOffset = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|Collision")
	FRotator WeaponCollisionRotation = FRotator::ZeroRotator;

private:
	bool bDamageWindowOpen = false;
	TArray<TWeakObjectPtr<AActor>> HitActorsThisSwing;
};

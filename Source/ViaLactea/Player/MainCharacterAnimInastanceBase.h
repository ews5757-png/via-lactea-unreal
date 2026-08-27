// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "CharacterEnums.h"

#include "PoseSearch/PoseSearchTrajectoryLibrary.h"
#include "Animation/TrajectoryTypes.h"

#include "MainCharacterAnimInastanceBase.generated.h"

enum class EPoseSearchInterruptMode : uint8;

/**
 * 
 */
UCLASS()
class VIALACTEA_API UMainCharacterAnimInastanceBase : public UAnimInstance
{
	GENERATED_BODY()
	
public:
    virtual void NativeInitializeAnimation() override;

	virtual void NativeUpdateAnimation(float DeltaSeconds) override;


public:
	//UFUNCTION(BlueprintCallable, Category = "Animation")
	void UpdateState(float DeltaTime);
	//UFUNCTION(BlueprintCallable, Category = "Animation")
	void Update_EssentialValues(float DeltaTime);

	bool IsMoving();

	//선택기에서 바인딩 하기 위한 UFUNTION 매크로
	UFUNCTION(/*BlueprintCallable,*/ BlueprintPure, Category = "Chooser", meta = (BlueprintThreadSafe))
	bool ShouldTurnInPlace() const;

	UFUNCTION(/*BlueprintCallable,*/ BlueprintPure, Category = "Chooser", meta = (BlueprintThreadSafe))
	bool IsPivoting() const;

	UFUNCTION(/*BlueprintCallable,*/ BlueprintPure, Category = "Chooser", meta = (BlueprintThreadSafe))
	bool IsStarting() const;

	UFUNCTION(/*BlueprintCallable,*/ BlueprintPure, Category = "Chooser", meta = (BlueprintThreadSafe))
	bool JustLanded_Light() const;

	UFUNCTION(/*BlueprintCallable,*/ BlueprintPure, Category = "Chooser", meta = (BlueprintThreadSafe))
	bool JustLanded_Heavy() const;


	//블루 프린트 모션 매칭 노드 에서 만들것
	//UFUNCTION(BlueprintImplementableEvent, Category = "Animation")
	//void Update_MotionMatching(float DeltaTime);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MotionMatching", meta = (BlueprintThreadSafe))
	EPoseSearchInterruptMode Get_MMInterruptMode() const;


	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "MotionMatching", meta = (BlueprintThreadSafe))
	float Get_MMBlendTime() const;

public:

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation")
	float RightLegBlend;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation")
	float LeftLegBlend;

protected:
    UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation")
    class AMainCharacterBase* CachedCharacter = nullptr;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation")
    class UCharacterMovementComponent* CachedMovementComp = nullptr;

	UPROPERTY(Transient, BlueprintReadWrite, Category = "Animation")
	float Speed;
	UPROPERTY(Transient, BlueprintReadWrite, Category = "Animation")
	FVector Acceleration;
	UPROPERTY(Transient, BlueprintReadWrite, Category = "Animation")
	FVector Acceleration_LastFrame;

	UPROPERTY(Transient, BlueprintReadWrite, Category = "Animation")
	float AccelerationAmount;

	UPROPERTY(Transient, BlueprintReadWrite, Category = "Animation")
	bool HasAcceleration;
	
	UPROPERTY(Transient, BlueprintReadWrite, Category = "Animation")
	float RotationSpeed;

	UPROPERTY(Transient, BlueprintReadWrite, Category = "Animation")
	FVector Velocity;

	UPROPERTY(Transient, BlueprintReadWrite, Category = "Animation")
	FVector Velocity_LastFrame;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation")
	float ForwardSpeed;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation")
	float RightSpeed;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation")
	bool bIsInAir;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation")
	bool bIsWeaponEquip;

	UPROPERTY(Transient, BlueprintReadWrite, Category = "Animation")
	EMovementContext MovementMode;

	UPROPERTY(Transient, BlueprintReadWrite, Category = "Animation")
	EMovementContext MovementMode_LastFrame;

	UPROPERTY(Transient, BlueprintReadWrite, Category = "Animation")
	ERotationMode RotationMode;

	UPROPERTY(Transient, BlueprintReadWrite, Category = "Animation")
	ERotationMode RotationMode_LastFrame;

	UPROPERTY(Transient, BlueprintReadWrite, Category = "Animation")
	EMovementState MovementState_LastFrame;

	UPROPERTY(Transient, BlueprintReadWrite, Category = "Animation")
	EMovementState MovementState;


	UPROPERTY(Transient, BlueprintReadWrite, Category = "Animation")
	EWeaponAnimType WeaponType;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|IK")
	bool bUseLeftHandIK = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|IK")
	bool bUseRightHandIK = false;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|IK")
	FTransform LeftHandIKTransformCS = FTransform::Identity;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|IK")
	FTransform RightHandIKTransformCS = FTransform::Identity;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|AimOffset")
	float AOYaw = 0.f;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|AimOffset")
	float AOPitch = 0.f;

	UPROPERTY(Transient, BlueprintReadOnly, Category = "Animation|AimOffset")
	bool bIsAiming = false;

	UPROPERTY(Transient, BlueprintReadWrite, Category = "Animation")
	EGait Gait;

	UPROPERTY(Transient, BlueprintReadWrite, Category = "Animation")
	EGait Gait_LastFrame;

	UPROPERTY(Transient, BlueprintReadWrite, Category = "Animation")
	EStance Stance_LastFrame;

	UPROPERTY(Transient, BlueprintReadWrite, Category = "Animation")
	EStance Stance;

	UPROPERTY(Transient, BlueprintReadWrite, Category = "Animation")
	FTransform CharacterTransform;

	UPROPERTY(Transient, BlueprintReadWrite, Category = "Animation")
	FTransform CharacterTransform_LastFrame;

	UPROPERTY( BlueprintReadWrite, Category = "Trajectory")
	FPoseSearchTrajectoryData TrajectoryGenerationData_Idle;

	UPROPERTY( BlueprintReadWrite, Category = "Trajectory")
	FPoseSearchTrajectoryData TrajectoryGenerationData_Moving;
	
	UPROPERTY( BlueprintReadWrite, Category = "Trajectory")
	FTransformTrajectory InOutTrajectory;

	UPROPERTY(BlueprintReadWrite, Category = "Trajectory")
	float PreviousDesiredControllerYaw;

	UPROPERTY(BlueprintReadWrite, Category = "Trajectory")
	FVector Trj_PastVelocity;

	UPROPERTY(BlueprintReadWrite, Category = "Trajectory")
	FVector Trj_CurrentVelocity;

	UPROPERTY(BlueprintReadWrite, Category = "Trajectory")
	FVector Trj_FutureVelocity;

	UPROPERTY(Transient, BlueprintReadWrite, Category = "Animation")
	float HeavyLandSpeedThreshold = 700.f;

};

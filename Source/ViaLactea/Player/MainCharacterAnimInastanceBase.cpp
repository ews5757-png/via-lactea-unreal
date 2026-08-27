// Fill out your copyright notice in the Description page of Project Settings.


#include "MainCharacterAnimInastanceBase.h"

//#include "GameFramework/Pawn.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "MainCharacterBase.h" 

#include "Kismet/KismetMathLibrary.h"

#include "PoseSearch/PoseSearchTrajectoryLibrary.h"
#include "PoseSearch/PoseSearchLibrary.h"

void UMainCharacterAnimInastanceBase::NativeInitializeAnimation()
{
    Super::NativeInitializeAnimation();

    APawn* PawnOwner = TryGetPawnOwner();

    CachedCharacter = Cast<AMainCharacterBase>(PawnOwner);
    CachedMovementComp = CachedCharacter ? CachedCharacter->GetCharacterMovement() : nullptr;
}

void UMainCharacterAnimInastanceBase::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
    //Super::BlueprintThreadSafeUpdateAnimation();

    APawn* PawnOwner = TryGetPawnOwner();
    if (!PawnOwner)
    {
        // 폰이 없으면 그 아래 단계(캐릭터/무브먼트)는 의미 없음
        CachedCharacter = nullptr;
        CachedMovementComp = nullptr;
        LeftHandIKTransformCS = FTransform::Identity;
        RightHandIKTransformCS = FTransform::Identity;
        bUseLeftHandIK = false;
        bUseRightHandIK = false;
        return;
    }

    // Pawn이 바뀌었으면: 캐릭터 캐시부터 다시
    if (CachedCharacter == nullptr || CachedCharacter != PawnOwner)
    {
        CachedCharacter = Cast<AMainCharacterBase>(PawnOwner);
        CachedMovementComp = nullptr; // 아래에서 다시 채움
    }

    if (!CachedCharacter)
    {
        // 폰은 있는데 내가 원하는 타입이 아님
        CachedMovementComp = nullptr;
        return;
    }

    // 캐릭터는 있는데 무브먼트만 없으면: 무브먼트만 재획득
    if (!CachedMovementComp)
    {
        CachedMovementComp = CachedCharacter->GetCharacterMovement();
        if (!CachedMovementComp)
        {
            return;
        }
    }

    Update_EssentialValues(DeltaSeconds);
    UpdateState(DeltaSeconds);
}

void UMainCharacterAnimInastanceBase::UpdateState(float DeltaSeconds)
{
    MovementMode_LastFrame = MovementMode;
    RotationMode_LastFrame = RotationMode;
    MovementState_LastFrame = MovementState;
    Gait_LastFrame = Gait;
    Stance_LastFrame = Stance;

    switch (CachedMovementComp->MovementMode)
    {
    case EMovementMode::MOVE_None:
    case EMovementMode::MOVE_Walking:
    case EMovementMode::MOVE_NavWalking:
        MovementMode = EMovementContext::OnGround;
        break;
    case EMovementMode::MOVE_Falling:
    case EMovementMode::MOVE_Flying:
        MovementMode = EMovementContext::InAir;
        break;
    case EMovementMode::MOVE_Swimming:
        break;
    case EMovementMode::MOVE_Custom:
        break;
    default:
        MovementMode = EMovementContext::OnGround;
        break;
    }

    RotationMode = CachedMovementComp->bOrientRotationToMovement ? ERotationMode::OrientToMovement : ERotationMode::Strafe;

    // 회전 모드 전환 시 모션매칭 궤적 히스토리 리셋
    if (RotationMode != RotationMode_LastFrame)
    {
        InOutTrajectory = FTransformTrajectory();
    }

    MovementState = IsMoving() ? EMovementState::Moving : EMovementState::Idle;


    Gait = CachedCharacter->GetGait();

    Stance = CachedCharacter->GetStance();

    WeaponType = CachedCharacter->GetWeaponAnimType();

    bIsWeaponEquip = CachedCharacter->GetisEquip();

}

void UMainCharacterAnimInastanceBase::Update_EssentialValues(float DeltaTime)
{
    Velocity_LastFrame = Velocity;
    Velocity = CachedCharacter ? CachedCharacter->GetVelocity() : FVector::ZeroVector;


    Speed = CachedCharacter ? Velocity.Size() : 0.f;

    ForwardSpeed = CachedCharacter ? FVector::DotProduct(Velocity, CachedCharacter->GetActorForwardVector()) : 0.f;
    RightSpeed = CachedCharacter ? FVector::DotProduct(Velocity, CachedCharacter->GetActorRightVector()) : 0.f;
    AOYaw = CachedCharacter ? CachedCharacter->GetAOYaw() : 0.f;
    AOPitch = CachedCharacter ? CachedCharacter->GetAOPitch() : 0.f;
    bIsAiming = CachedCharacter ? CachedCharacter->IsAiming() : false;
    bUseLeftHandIK = CachedCharacter ? CachedCharacter->ShouldUseLeftHandIK() : false;
    bUseRightHandIK = CachedCharacter ? CachedCharacter->ShouldUseRightHandIK() : false;

    USkeletalMeshComponent* OwningMeshComponent = GetOwningComponent();
    LeftHandIKTransformCS = (CachedCharacter && OwningMeshComponent)
        ? CachedCharacter->GetLeftHandIKTransformCSForMesh(OwningMeshComponent)
        : FTransform::Identity;
    RightHandIKTransformCS = (CachedCharacter && OwningMeshComponent)
        ? CachedCharacter->GetRightHandIKTransformCSForMesh(OwningMeshComponent)
        : FTransform::Identity;

    bIsInAir = CachedMovementComp ? CachedMovementComp->IsFalling() : false;

    
    CharacterTransform_LastFrame = CharacterTransform;
    CharacterTransform = CachedCharacter ? CachedCharacter->GetActorTransform() : FTransform::Identity;

    //const float DeltaYaw =
    //    CharacterTransform.GetRotation().Z - CharacterTransform_LastFrame.GetRotation().Z;

    //const float NormalizedDeltaYaw =
    //    UKismetMathLibrary::NormalizeAxis(DeltaYaw);

    //RotationSpeed = NormalizedDeltaYaw / DeltaTime;

    const float Yaw = CharacterTransform.GetRotation().Rotator().Yaw;
    const float YawPrev = CharacterTransform_LastFrame.GetRotation().Rotator().Yaw;
    const float NormalizedDeltaYaw = UKismetMathLibrary::NormalizeAxis(Yaw - YawPrev);
    RotationSpeed = NormalizedDeltaYaw / FMath::Max(DeltaTime, KINDA_SMALL_NUMBER);


    // <- 여기부터 방어 코드 안씀
    Acceleration_LastFrame = Acceleration;
    Acceleration = CachedMovementComp->GetCurrentAcceleration();
    AccelerationAmount = Acceleration.Length() / CachedMovementComp->GetMaxAcceleration();
    HasAcceleration = AccelerationAmount > 0;



    FPoseSearchTrajectoryData SelectData = Speed > 0 ? TrajectoryGenerationData_Moving : TrajectoryGenerationData_Idle;
    UPoseSearchTrajectoryLibrary::PoseSearchGenerateTransformTrajectory(this, SelectData, DeltaTime, InOutTrajectory, PreviousDesiredControllerYaw, InOutTrajectory, -1, 30, 0.1, 15);

    UPoseSearchTrajectoryLibrary::GetTransformTrajectoryVelocity(InOutTrajectory, -0.3f, 0.1f, Trj_PastVelocity);

    UPoseSearchTrajectoryLibrary::GetTransformTrajectoryVelocity(InOutTrajectory, 0.f, 0.2f, Trj_CurrentVelocity);

    UPoseSearchTrajectoryLibrary::GetTransformTrajectoryVelocity(InOutTrajectory, 0.4f, 0.5f, Trj_FutureVelocity);


}



bool UMainCharacterAnimInastanceBase::IsMoving()
{
    if (Trj_FutureVelocity != FVector::ZeroVector && Acceleration != FVector::ZeroVector)
    {
        return true;
    }
    return false;
}

bool UMainCharacterAnimInastanceBase::ShouldTurnInPlace() const
{
    return (FMath::Abs(RotationSpeed) >= 50.0)
        && (
            (RotationMode == ERotationMode::OrientToMovement)
            || ((MovementState == EMovementState::Idle) && (MovementState_LastFrame == EMovementState::Moving))
            );
}

bool UMainCharacterAnimInastanceBase::IsPivoting() const
{
    const float FutureYaw = Trj_FutureVelocity.Rotation().Yaw;
    const float CurrYaw = Velocity.Rotation().Yaw;

    const float DeltaYaw = FMath::FindDeltaAngleDegrees(CurrYaw, FutureYaw);
    const float AbsDeltaYaw = FMath::Abs(DeltaYaw);

    float ThresholdDeg;

    switch (RotationMode) 
    {
    case ERotationMode::OrientToMovement:
        ThresholdDeg = 45.f;
        break;
    case ERotationMode::Strafe:
        ThresholdDeg = 30.f;
        break;
    default:
        ThresholdDeg = 45.f;
        break;
    }

    return AbsDeltaYaw >= ThresholdDeg;
}

bool UMainCharacterAnimInastanceBase::JustLanded_Light() const
{
    if (!CachedCharacter) return false;

    bool bLightLanding = FMath::Abs(CachedCharacter->GetVelocity().Z) < HeavyLandSpeedThreshold;
    return  CachedCharacter->IsJustLanded() && (bLightLanding);
}

bool UMainCharacterAnimInastanceBase::JustLanded_Heavy() const
{
    if (!CachedCharacter) return false;
    bool bHeavyLanding = FMath::Abs(CachedCharacter->GetVelocity().Z) >= HeavyLandSpeedThreshold;
    return  CachedCharacter->IsJustLanded() && (bHeavyLanding);
}

bool UMainCharacterAnimInastanceBase::IsStarting() const
{
    return Trj_FutureVelocity.Size2D() >= Velocity.Size2D() + 100.f;
}

EPoseSearchInterruptMode UMainCharacterAnimInastanceBase::Get_MMInterruptMode() const
{
    const bool bMovementModeChanged = (MovementMode != MovementMode_LastFrame);
    const bool bMovementStateChanged = (MovementState != MovementState_LastFrame);
    const bool bStanceChanged = (Stance != Stance_LastFrame);

    const bool bIsMovingState = (MovementState == EMovementState::Moving);
    const bool bGaitChangedWhileMoving = bIsMovingState && (Gait != Gait_LastFrame);

    // Walking일 때만 세부 변화 체크
    const bool bAnyRelevantChangeWhileWalking =
        bMovementStateChanged ||
        bGaitChangedWhileMoving ||
        bStanceChanged;


    const bool bIsOnGroundWithRelevantChange = MovementMode == EMovementContext::OnGround && bAnyRelevantChangeWhileWalking;

    const bool bShouldInterrupt = bIsOnGroundWithRelevantChange || bMovementModeChanged;

    return bShouldInterrupt
        ? EPoseSearchInterruptMode::InterruptOnDatabaseChange
        : EPoseSearchInterruptMode::DoNotInterrupt;

}

float UMainCharacterAnimInastanceBase::Get_MMBlendTime() const
{
    switch (MovementMode)
    {
    case EMovementContext::OnGround:
        switch (MovementState_LastFrame)
        {
        case EMovementState::Idle:
            return 0.5f;
        case EMovementState::Moving:
            return 0.2f;
        default:
            return 0.2f;
        }
    case EMovementContext::InAir:
    {
        return (Velocity.Z > 100.f) ? 0.15f : 0.5f;
    }
    default:

        return 0.2f;
    }
}



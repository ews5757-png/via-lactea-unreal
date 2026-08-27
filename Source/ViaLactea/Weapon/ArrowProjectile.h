// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ArrowProjectile.generated.h"

/**
 * 화살 투사체 클래스
 * ProjectileMovement 컴포넌트로 포물선 비행 처리
 */
UCLASS()
class VIALACTEA_API AArrowProjectile : public AActor
{
	GENERATED_BODY()

public:
	AArrowProjectile();
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void Tick(float DeltaTime) override;

public:
	// 콜리전 루트 (작은 구체, ProjectileMovement 기준)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arrow")
	class USphereComponent* ArrowRoot;

	// 화살 메시 (ArrowRoot 자식 — 상대 회전 자유롭게 설정 가능)
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arrow")
	class UStaticMeshComponent* ArrowMesh;

	// 발사 시 활성화되는 궤적 트레일
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arrow|VFX")
	class UNiagaraComponent* ArrowTrail;

	// 기본 화살 궤적 메시
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arrow|VFX")
	class UStaticMeshComponent* ArrowTrailMesh;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arrow|Sound")
	class UAudioComponent* ArrowFlightLoopAudioComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arrow|Sound")
	class UAudioComponent* ArrowFlightLoopAudioComponentB;

	// 투사체 이동 컴포넌트
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Arrow")
	class UProjectileMovementComponent* ProjectileMovement;

	// 화살 데미지
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Arrow|Combat")
	float Damage = 30.f;

	// 명중 후 월드에 남아있는 시간
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Arrow|Combat")
	float DestroyDelayAfterHit = 5.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Arrow|Combat", meta = (ClampMin = "0.0"))
	float StickDepth = 8.0f;

	// 화살 메시 오프셋 회전 (메시 import 방향에 따라 BP에서 조정)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Arrow|Visual")
	FRotator MeshRotationOffset = FRotator(90.f, 0.f, 0.f);

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Arrow|Visual")
	FVector MeshLocationOffset = FVector(30.f, 0.f, 0.f );

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Arrow|VFX")
	class UNiagaraSystem* ArrowTrailSystem = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Arrow|Sound")
	TObjectPtr<class USoundBase> ArrowFlightLoopSoundCue = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Arrow|Sound")
	class USoundAttenuation* ArrowFlightLoopAttenuationSettings = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Arrow|Sound", meta = (ClampMin = "0.0"))
	float ArrowFlightLoopVolumeMultiplier = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Arrow|Sound", meta = (ClampMin = "0.01"))
	float ArrowFlightLoopPitchMultiplier = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Arrow|Sound", meta = (ClampMin = "0.0"))
	float ArrowFlightLoopFadeInTime = 0.25f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Arrow|Sound", meta = (ClampMin = "0.0"))
	float ArrowFlightLoopFadeOutTime = 0.25f;

	// 사운드 길이 중 이 지점부터 다음 재생을 겹쳐서 크로스페이드한다. 0.5 = 중간 지점.
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Arrow|Sound", meta = (ClampMin = "0.01", ClampMax = "0.99"))
	float ArrowFlightLoopRestartAlpha = 0.25f;

	// 소켓 분리 + 콜리전/이동 활성화 후 발사
	void Fire(const FVector& Direction, float Speed);
	void SetHidePreviewFromOwner(bool bNewHidePreviewFromOwner);

private:
	// 구체 콜리전 히트 처리
	UFUNCTION()
	void OnHit(UPrimitiveComponent* HitComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, FVector NormalImpulse, const FHitResult& Hit);

	void StickToHitTarget(UPrimitiveComponent* HitComponent, const FHitResult& Hit, const FVector& ImpactDirection);
	void ApplyPreviewOwnerVisibility();
	void UpdateArrowFlightLoopSound();
	void StartArrowFlightLoopSound();
	void StopArrowFlightLoopSound();
	void StartArrowFlightLoopOnComponent(class UAudioComponent* AudioComponent);
	void ApplyArrowFlightState();

	UFUNCTION()
	void OnRep_HidePreviewFromOwner();

	UFUNCTION()
	void OnRep_ArrowInFlight();

	UPROPERTY(ReplicatedUsing = OnRep_HidePreviewFromOwner)
	bool bHidePreviewFromOwner = false;

	UPROPERTY(ReplicatedUsing = OnRep_ArrowInFlight)
	bool bArrowInFlight = false;

	bool bHasHitSomething = false;
	bool bUseFirstArrowFlightLoopAudioComponent = true;
	float NextArrowFlightLoopRestartTime = 0.f;
};

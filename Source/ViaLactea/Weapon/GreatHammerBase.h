#pragma once

#include "CoreMinimal.h"
#include "MeleeWeaponBase.h"
#include "GreatHammerBase.generated.h"

class UAnimMontage;
class UAnimInstance;

UENUM(BlueprintType)
enum class EGreatHammerWhirlwindPhase : uint8
{
	None,
	Starting,
	Looping,
	Finishing,
	Canceling
};

UCLASS(Blueprintable)
class VIALACTEA_API AGreatHammerBase : public AMeleeWeaponBase
{
	GENERATED_BODY()

public:
	AGreatHammerBase();

	virtual void Tick(float DeltaTime) override;
	virtual void StartAbilityAction() override;
	virtual void StopAbilityAction() override;
	virtual float GetStaminaCostForAction(EEquipmentActionType ActionType) const override;
	virtual bool CanStartAction(EEquipmentActionType ActionType) const override;
	virtual void ForceResetState() override;
	virtual void OnNaturalMontageEnd() override;
	virtual void OnInterruptedMontageEnd() override;
	virtual bool IsAbilityMontage(const UAnimMontage* Montage) const override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|GreatHammer|Ability")
	UAnimMontage* WhirlwindMontage = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|GreatHammer|Ability")
	FName WhirlwindStartSection = TEXT("Default");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|GreatHammer|Ability")
	FName WhirlwindLoopSection = TEXT("LoopStart");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|GreatHammer|Ability")
	FName WhirlwindCancelSection = TEXT("CancelStart");

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|GreatHammer|Ability")
	TArray<FName> WhirlwindFinishSections = { TEXT("End1Start"), TEXT("End2Start") };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|GreatHammer|Ability", meta = (ClampMin = "0.0"))
	float WhirlwindCancelDuration = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|GreatHammer|Ability", meta = (ClampMin = "0.0"))
	float WhirlwindFinishStageDuration = 2.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|GreatHammer|Ability", meta = (ClampMin = "0.0"))
	float WhirlwindTerminalBlendOutTime = 0.12f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|GreatHammer|Ability")
	bool bRotateWhirlwindToInput = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|GreatHammer|Ability", meta = (ClampMin = "0.0"))
	float WhirlwindLoopStaminaDrainPerSecond = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|GreatHammer|Ability", meta = (ClampMin = "0.0"))
	float WhirlwindFinishStaminaCost = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment|Weapon|GreatHammer|Ability", meta = (ClampMin = "0.0"))
	float WhirlwindInputRotationInterpSpeed = 1.f;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Weapon|GreatHammer|Ability")
	bool bIsWhirlwindActive = false;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Equipment|Weapon|GreatHammer|Ability")
	int32 CurrentWhirlwindLevel = 0;

private:
	bool BeginWhirlwindAbilityLocal();
	float PlayWhirlwindMontageLocal(FName StartSection) const;
	void StopWhirlwindMontageLocal(float BlendOutTime) const;
	void AdvanceWhirlwindStage();
	void ScheduleNextWhirlwindStage();
	void ConfigureWhirlwindSectionFlow() const;
	void SetWhirlwindNextSection(FName FromSection, FName ToSection) const;
	void ScheduleWhirlwindLoopEntry();
	void EnterWhirlwindLoopSection();
	void RouteWhirlwindToSection(FName TargetSection, EGreatHammerWhirlwindPhase NewPhase);
	void ScheduleTerminalSectionCompletion(FName TargetSection);
	void CompleteWhirlwindTerminalSection();
	void UpdateWhirlwindInputRotation();
	void DrainWhirlwindLoopStamina(float DeltaTime);
	void CheckTerminalSectionComplete();
	void UpdateWhirlwindLevelFromElapsed();
	void UpdateWhirlwindLevelFromElapsed(float ElapsedTime);
	void ResetWhirlwindState();
	float GetWhirlwindHeldDuration() const;
	int32 GetWhirlwindLevelForElapsed(float ElapsedTime) const;
	bool GetWhirlwindStopRoute(float HeldDuration, FName& OutTargetSection, EGreatHammerWhirlwindPhase& OutPhase) const;
	bool ApplyWhirlwindStopRoute(FName TargetSection, EGreatHammerWhirlwindPhase NewPhase, float HeldDuration);
	FName GetFinishSectionNameForCurrentWhirlwindLevel() const;
	float GetWhirlwindFinishThresholdTime(int32 FinishSectionIndex) const;
	float GetSectionLength(FName SectionName) const;
	float GetTimeRemainingInCurrentSection() const;
	UAnimInstance* GetOwnerAnimInstance() const;

	UFUNCTION(Server, Reliable)
	void ServerStartWhirlwindAbility();

	UFUNCTION(Server, Reliable)
	void ServerStopWhirlwindAbility(float ClientHeldDuration);

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStartWhirlwindAbility();

	UFUNCTION(NetMulticast, Reliable)
	void MulticastStopWhirlwindAbility(FName TargetSection, EGreatHammerWhirlwindPhase NewPhase, float HeldDuration);

	double WhirlwindStartTime = 0.0;
	FName ActiveTerminalSection = NAME_None;
	bool bIsAbilityInputHeld = false;
	EGreatHammerWhirlwindPhase WhirlwindPhase = EGreatHammerWhirlwindPhase::None;
};

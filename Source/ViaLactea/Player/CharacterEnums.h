#pragma once


#include "CoreMinimal.h"

#include "CharacterEnums.generated.h"

UENUM(BlueprintType)
enum class EGait : uint8 
{
	Walk,
	Run,
	Sprint
};

UENUM(BlueprintType)
enum class EStance : uint8
{
	Standing,
	Crouching
};

UENUM(BlueprintType)
enum class EMovementState : uint8
{
	Idle,
	Moving
};

UENUM(BlueprintType)
enum class ERotationMode : uint8
{
	OrientToMovement,
	Strafe
};

UENUM(BlueprintType)
enum class EFacingSource : uint8
{
	None,
	ControllerRotation,
	InputDirection,
	RollDirection,
	RollRecovery
};

UENUM(BlueprintType)
enum class EMovementContext : uint8
{
	OnGround,
	InAir
};

UENUM(BlueprintType)
enum class EWeaponAnimType : uint8
{
	None = 0,
	OneHandedMelee = 1,
	Bow = 2,
	TwoHandedMelee = 3
};

// 애니메이션 노티파이를 위한 Enum
UENUM(BlueprintType)
enum class EEquipMentHandleAction : uint8
{
	EnableCollision,
	DisableCollision,
	CanAction,
	FireBowArrow,
	AttachPendingEquipment,
	UnequipComplete,
	LockMove,
	UnlockMove,
	ThrowHammer,
	DeathRagdoll,
	PlaySwingSound,
	PlayEquipSound,
	PlayUnequipSound,
	ResetHitActors
};

UENUM(BlueprintType)
enum class EWeaponTarget : uint8
{
	Right,
	Left,
	Both
};

// 장비 슬롯 타입 Enum
UENUM(BlueprintType)
enum class EEquipmentSlotType : uint8
{
	RightHand,
	LeftHand,
	TwoHanded,
	Bow,
	Armor,
	Shield
};

UENUM(BlueprintType)
enum class EEquipmentActionType : uint8
{
	Primary,
	Secondary,
	Ability
};

UENUM(BlueprintType)
enum class EDefenseWindowType : uint8
{
	Invincible,
	Parry,
	Block
};

UENUM(BlueprintType)
enum class EPendingInputAction : uint8
{
	None,
	Run,
	Secondary,
	Jump,
	Roll,
	Primary,
	Ability
};

// 캐릭터 전역 상태 - 전신 차단 상태만 관리
UENUM(BlueprintType)
enum class ECharacterActionState : uint8
{
	Idle,		// 기본 상태 - 모든 액션 허용
	Rolling,	// 구르기 중
	InAir,		// 공중 (점프/낙하)
	Stunned,	// 피격/스턴
	Dead		// 사망
};


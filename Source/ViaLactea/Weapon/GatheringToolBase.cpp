#include "GatheringToolBase.h"

AGatheringToolBase::AGatheringToolBase()
{
	WeaponType = EWeaponAnimType::OneHandedMelee;
	Damage = 12.f;
	PrimaryAttackDamageMultipliers = { 0.75f, 0.85f };
	SecondaryActionDamageMultiplier = 0.8f;
	AbilityActionDamageMultiplier = 1.0f;
}

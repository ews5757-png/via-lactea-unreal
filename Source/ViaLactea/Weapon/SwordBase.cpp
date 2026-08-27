// Fill out your copyright notice in the Description page of Project Settings.

#include "SwordBase.h"

ASwordBase::ASwordBase()
{
	WeaponType = EWeaponAnimType::OneHandedMelee;
	Damage = 24.f;
	PrimaryAttackDamageMultipliers = { 0.95f, 1.0f, 1.25f };
	PrimaryAttackSoundPitchMultipliers = { 1.f, 1.f, 0.75f };
	SecondaryActionDamageMultiplier = 1.15f;
	AbilityActionDamageMultiplier = 1.6f;
}

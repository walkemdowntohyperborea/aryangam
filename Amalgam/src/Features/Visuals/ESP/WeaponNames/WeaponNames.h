#pragma once
#include "../../../../SDK/SDK.h"

class CWeaponNames
{
public:
	std::string GetWeaponName(CTFWeaponBase* pWeapon);
	std::string GetWeaponNameUpper(CTFWeaponBase* pWeapon);
	std::string GetWeaponNameLower(CTFWeaponBase* pWeapon);
};

ADD_FEATURE(CWeaponNames, WeaponNames);
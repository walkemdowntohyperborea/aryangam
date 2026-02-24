#include "WeaponNames.h"

std::string CWeaponNames::GetWeaponName(CTFWeaponBase* pWeapon)
{
	if (!pWeapon)
		return "Unknown Weapon";

	const int iWeaponID = pWeapon->GetWeaponID();
	const int iWeaponIndex = pWeapon->m_iItemDefinitionIndex();

	const char* sReturnValue = "Unknown Weapon";
	switch (iWeaponIndex)
	{ // most multi/all class weapons host the same index for each class' weapon
	case Heavy_m_Deflector_mvm:
		sReturnValue = "Deflector"; break;
	case Demoman_t_TheHalfZatoichi:
		sReturnValue = "Half-Zatoichi"; break;
	case Engi_m_PanicAttack:
		sReturnValue = "Panic Attack"; break;
	case Misc_t_Saxxy:
		sReturnValue = "Saxxy"; break;
	case Misc_t_FryingPan:
		sReturnValue = "Frying Pan"; break;
	case Misc_t_GoldFryingPan:
		sReturnValue = "Golden Frying Pan"; break;
	case Demoman_t_TheConscientiousObjector:
		sReturnValue = "Conscientious Objector"; break;
	case Demoman_t_PrinnyMachete:
		sReturnValue = "Prinny Machete"; break;
	case Demoman_t_TheMemoryMaker:
		sReturnValue = "Memory Maker"; break;
	case Demoman_t_TheBatOuttaHell:
		sReturnValue = "Bat Outta Hell"; break;
	case Demoman_t_TheHamShank:
		sReturnValue = "Ham Shank";
	}

	if (FNV1A::Hash32(sReturnValue) == FNV1A::Hash32Const("Unknown Weapon"))
	{
		switch (iWeaponID)
		{
		case TF_WEAPON_BAT:
			switch (iWeaponIndex)
			{
			case Scout_t_TheCandyCane:
				sReturnValue = "Candy Cane"; break;
			case Scout_t_TheBostonBasher:
			case Scout_t_ThreeRuneBlade:
				sReturnValue = "Boston Basher"; break;
			case Scout_t_SunonaStick:
				sReturnValue = "Sun-on-a-Stick"; break;
			case Scout_t_TheFanOWar:
				sReturnValue = "Fan O War"; break;
			case Scout_t_TheAtomizer:
				sReturnValue = "Atomizer"; break;
			default: sReturnValue = "Bat";
			}
			break;
		case TF_WEAPON_BAT_FISH:
			sReturnValue = "Holy Mackerel"; break;
		case TF_WEAPON_BAT_GIFTWRAP:
			sReturnValue = "Wrap Assassin"; break;
		case TF_WEAPON_BAT_WOOD:
			sReturnValue = "Sandman"; break;
		case TF_WEAPON_BONESAW:
			switch (iWeaponIndex)
			{
			case Medic_t_Amputator:
				sReturnValue = "Amputator"; break;
			case Medic_t_TheUbersaw:
			case Medic_t_FestiveUbersaw:
				sReturnValue = "Ubersaw"; break;
			case Medic_t_TheVitaSaw:
				sReturnValue = "Vita-Saw"; break;
			case Medic_t_TheSolemnVow:
				sReturnValue = "Solemn Vow"; break;
			default: sReturnValue = "Bonesaw";
			}
			break;
		case TF_WEAPON_BOTTLE:
			sReturnValue = "Bottle"; break;
		case TF_WEAPON_BREAKABLE_SIGN:
			sReturnValue = "Neon Annihilator"; break;
		case TF_WEAPON_BUFF_ITEM:
			switch (iWeaponIndex)
			{
			case Soldier_s_TheBuffBanner:
			case Soldier_s_FestiveBuffBanner:
				sReturnValue = "Buff Banner"; break;
			case Soldier_s_TheBattalionsBackup:
				sReturnValue = "Battalions Backup"; break;
			case Soldier_s_TheConcheror:
				sReturnValue = "Concheror";
			}
			break;
		case TF_WEAPON_BUILDER:
			switch (iWeaponIndex)
			{
			case Engi_p_PDA:
				sReturnValue = "Toolbox"; break;
			case Spy_s_Sapper:
			case Spy_s_SapperR:
				sReturnValue = "Sapper";
			}
			break;
		case TF_WEAPON_CANNON:
			sReturnValue = "Loose Cannon"; break;
		case TF_WEAPON_CHARGED_SMG:
			sReturnValue = "Cleaners Carbine"; break;
		case TF_WEAPON_CLEAVER:
			sReturnValue = "Flying Guillotine"; break;
		case TF_WEAPON_CLUB:
			switch (iWeaponIndex)
			{
			case Sniper_t_TheTribalmansShiv:
				sReturnValue = "Tribalmans Shiv"; break;
			case Sniper_t_TheBushwacka:
				sReturnValue = "Bushwacka"; break;
			case Sniper_t_TheShahanshah:
				sReturnValue = "Shahanshah"; break;
			default: sReturnValue = "Kukri";
			}
			break;
		case TF_WEAPON_COMPOUND_BOW:
			sReturnValue = "Huntsman"; break;
		case TF_WEAPON_CROSSBOW:
			sReturnValue = "Crossbow"; break;
		case TF_WEAPON_DRG_POMSON:
			sReturnValue = "Pomson"; break;
		case TF_WEAPON_FIREAXE:
			switch (iWeaponIndex)
			{
			case Pyro_t_TheAxtinguisher:
			case Pyro_t_TheFestiveAxtinguisher:
			case Pyro_t_ThePostalPummeler:
				sReturnValue = "Axtinguisher"; break;
			case Pyro_t_Homewrecker:
			case Pyro_t_TheMaul:
				sReturnValue = "Homewrecker"; break;
			case Pyro_t_TheBackScratcher:
				sReturnValue = "Back Scratcher"; break;
			case Pyro_t_ThePowerjack:
				sReturnValue = "Powerjack"; break;
			case Pyro_t_SharpenedVolcanoFragment:
				sReturnValue = "Volcano Fragment"; break;
			case Pyro_t_TheThirdDegree:
				sReturnValue = "Third Degree"; break;
			default: sReturnValue = "Fire Axe";
			}
			break;
		case TF_WEAPON_FISTS:
			switch (iWeaponIndex)
			{
			case Heavy_t_TheKillingGlovesofBoxing:
				sReturnValue = "Fists-KGB"; break;
			case Heavy_t_GlovesofRunningUrgently:
			case Heavy_t_GlovesofRunningUrgentlyMvM:
			case Heavy_t_FestiveGlovesofRunningUrgently:
			case Heavy_t_TheBreadBite:
				sReturnValue = "GRU"; break;
			case Heavy_t_WarriorsSpirit:
				sReturnValue = "Warriors Spirit"; break;
			case Heavy_t_FistsofSteel:
				sReturnValue = "Fists Of Steel"; break;
			case Heavy_t_TheEvictionNotice:
				sReturnValue = "Eviction Notice"; break;
			case Heavy_t_TheHolidayPunch:
				sReturnValue = "Holiday Punch"; break;
			default: sReturnValue = "Fists";
			}
			break;
		case TF_WEAPON_FLAMETHROWER:
			switch (iWeaponIndex)
			{
			case Pyro_m_TheBackburner:
			case Pyro_m_FestiveBackburner:
				sReturnValue = "Backburner"; break;
			case Pyro_m_TheDegreaser:
				sReturnValue = "Degreaser"; break;
			case Pyro_m_ThePhlogistinator:
				sReturnValue = "Phlogistinator"; break;
			case Pyro_m_NostromoNapalmer:
				sReturnValue = "Nostromo Napalmer"; break;
			default: sReturnValue = "Flame Thrower";
			}
			break;
		case TF_WEAPON_FLAME_BALL:
			sReturnValue = "Dragons Fury"; break;
		case TF_WEAPON_FLAREGUN:
			switch (iWeaponIndex)
			{
			case Pyro_s_TheDetonator:
				sReturnValue = "Detonator"; break;
			case Pyro_s_TheScorchShot:
				sReturnValue = "Scorch Shot"; break;
			default: sReturnValue = "Flare Gun";
			}
			break;
		case TF_WEAPON_FLAREGUN_REVENGE:
			sReturnValue = "Manmelter"; break;
		case TF_WEAPON_GRAPPLINGHOOK:
			sReturnValue = "Grappling Hook"; break;
		case TF_WEAPON_GRENADELAUNCHER:
			switch (iWeaponIndex)
			{
			case Demoman_m_TheLochnLoad:
				sReturnValue = "Loch-n-Load"; break;
			case Demoman_m_TheIronBomber:
				sReturnValue = "Iron Bomber"; break;
			default: sReturnValue = "Grenade Launcher";
			}
			break;
		case TF_WEAPON_HANDGUN_SCOUT_PRIMARY:
			sReturnValue = "Shortstop"; break;
		case TF_WEAPON_HANDGUN_SCOUT_SECONDARY:
			switch (iWeaponIndex)
			{
			case Scout_s_TheWinger:
				sReturnValue = "Winger"; break;
			case Scout_s_PrettyBoysPocketPistol:
				sReturnValue = "Pocket Pistol";
			}
			break;
		case TF_WEAPON_JAR:
			sReturnValue = "Jarate"; break;
		case TF_WEAPON_JAR_GAS:
			sReturnValue = "Gas Passer"; break;
		case TF_WEAPON_JAR_MILK:
			sReturnValue = "Mad Milk"; break;
		case TF_WEAPON_KNIFE:
			switch (iWeaponIndex)
			{
			case Spy_t_YourEternalReward:
			case Spy_t_TheWangaPrick:
				sReturnValue = "Eternal Reward"; break;
			case Spy_t_ConniversKunai:
				sReturnValue = "Kunai"; break;
			case Spy_t_TheBigEarner:
				sReturnValue = "Big Earner"; break;
			case Spy_t_TheSpycicle:
				sReturnValue = "Spycicle"; break;
			default: sReturnValue = "Knife";
			}
			break;
		case TF_WEAPON_LASER_POINTER:
			sReturnValue = "Wrangler"; break;
		case TF_WEAPON_LUNCHBOX:
			switch (iWeaponIndex)
			{
			case Heavy_s_TheDalokohsBar:
			case Heavy_s_Fishcake:
				sReturnValue = "Dalokohs Bar"; break;
			case Heavy_s_TheBuffaloSteakSandvich:
				sReturnValue = "Steak"; break;
			case Heavy_s_SecondBanana:
				sReturnValue = "Banana"; break;
			case Scout_s_BonkAtomicPunch:
			case Scout_s_FestiveBonk:
				sReturnValue = "Bonk"; break;
			case Scout_s_CritaCola:
				sReturnValue = "Crit-a-Cola"; break;
			default: sReturnValue = "Sandwich";
			}
			break;
		case TF_WEAPON_MECHANICAL_ARM:
			sReturnValue = "Short Circuit"; break;
		case TF_WEAPON_MEDIGUN:
			switch (iWeaponIndex)
			{
			case Medic_s_TheKritzkrieg:
				sReturnValue = "Kritzkrieg"; break;
			case Medic_s_TheQuickFix:
				sReturnValue = "Quick Fix"; break;
			case Medic_s_TheVaccinator:
				sReturnValue = "Vaccinator"; break;
			default: sReturnValue = "Medigun";
			}
			break;
		case TF_WEAPON_MINIGUN:
			switch (iWeaponIndex)
			{
			case Heavy_m_Natascha:
				sReturnValue = "Natascha"; break;
			case Heavy_m_TheBrassBeast:
				sReturnValue = "Brass Beast"; break;
			case Heavy_m_Tomislav:
				sReturnValue = "Tomislav"; break;
			case Heavy_m_TheHuoLongHeater:
			case Heavy_m_TheHuoLongHeaterG:
				sReturnValue = "Huo-Long Heater"; break;
			default: sReturnValue = "Minigun";
			}
			break;
		case TF_WEAPON_PARACHUTE:
			sReturnValue = "Base Jumper"; break;
		case TF_WEAPON_PARTICLE_CANNON:
			sReturnValue = "Cow Mangler"; break;
		case TF_WEAPON_PDA_ENGINEER_BUILD:
		case TF_WEAPON_PDA_ENGINEER_DESTROY:
			sReturnValue = "PDA"; break;
		case TF_WEAPON_PDA_SPY:
			sReturnValue = "Disguise Kit"; break;
		case TF_WEAPON_PEP_BRAWLER_BLASTER:
			sReturnValue = "Baby Faces Blaster"; break;
		case TF_WEAPON_PIPEBOMBLAUNCHER:
			switch (iWeaponIndex)
			{
			case Demoman_s_TheScottishResistance:
				sReturnValue = "Scottish Resistance"; break;
			case Demoman_s_StickyJumper:
				sReturnValue = "Sticky Jumper"; break;
			case Demoman_s_TheQuickiebombLauncher:
				sReturnValue = "Quickiebomb Launcher"; break;
			default: sReturnValue = "Stickybomb Launcher";
			}
			break;
		case TF_WEAPON_PISTOL:
		case TF_WEAPON_PISTOL_SCOUT:
			sReturnValue = "Pistol"; break;
		case TF_WEAPON_RAYGUN:
			sReturnValue = "Bison"; break;
		case TF_WEAPON_REVOLVER:
			switch (iWeaponIndex)
			{
			case Spy_m_TheAmbassador:
			case Spy_m_FestiveAmbassador:
				sReturnValue = "Ambassador"; break;
			case Spy_m_LEtranger:
				sReturnValue = "Letranger"; break;
			case Spy_m_TheEnforcer:
				sReturnValue = "Enforcer"; break;
			case Spy_m_TheDiamondback:
				sReturnValue = "Diamondback"; break;
			default: sReturnValue = "Revolver";
			}
			break;
		case TF_WEAPON_ROCKETLAUNCHER:
			switch (iWeaponIndex)
			{
			case Soldier_m_TheBlackBox:
			case Soldier_m_FestiveBlackBox:
				sReturnValue = "Black Box"; break;
			case Soldier_m_RocketJumper:
				sReturnValue = "Rocket Jumper"; break;
			case Soldier_m_TheLibertyLauncher:
				sReturnValue = "Liberty Launcher"; break;
			case Soldier_m_TheOriginal:
				sReturnValue = "Original"; break;
			case Soldier_m_TheBeggarsBazooka:
				sReturnValue = "Beggars Bazooka"; break;
			case Soldier_m_TheAirStrike:
				sReturnValue = "Air Strike"; break;
			default: sReturnValue = "Rocket Launcher";
			}
			break;
		case TF_WEAPON_ROCKETLAUNCHER_DIRECTHIT:
			sReturnValue = "Direct Hit"; break;
		case TF_WEAPON_ROCKETPACK:
			sReturnValue = "Thermal Thruster"; break;
		case TF_WEAPON_SCATTERGUN:
			switch (iWeaponIndex)
			{
			case Scout_m_ForceANature:
			case Scout_m_FestiveForceANature:
				sReturnValue = "Force-A-Nature"; break;
			default: sReturnValue = "Scattergun";
			}
			break;
		case TF_WEAPON_SENTRY_REVENGE:
			sReturnValue = "Frontier Justice"; break;
		case TF_WEAPON_SHOTGUN_BUILDING_RESCUE:
			sReturnValue = "Rescue Ranger"; break;
		case TF_WEAPON_SHOTGUN_PYRO:
		case TF_WEAPON_SHOTGUN_SOLDIER:
			sReturnValue = "Shotgun"; break;
		case TF_WEAPON_SHOTGUN_HWG:
			if (iWeaponIndex == Heavy_s_TheFamilyBusiness)
				sReturnValue = "Family Business";
			else
				sReturnValue = "Shotgun";
			break;
		case TF_WEAPON_SHOTGUN_PRIMARY:
			if (iWeaponIndex == Engi_m_TheWidowmaker)
				sReturnValue = "Widowmaker";
			else
				sReturnValue = "Shotgun";
			break;
		case TF_WEAPON_SHOVEL:
			switch (iWeaponIndex)
			{
			case Soldier_t_TheEqualizer:
				sReturnValue = "Equalizer"; break;
			case Soldier_t_ThePainTrain:
				sReturnValue = "Pain Train"; break;
			case Soldier_t_TheMarketGardener:
				sReturnValue = "Market Gardener"; break;
			case Soldier_t_TheDisciplinaryAction:
				sReturnValue = "Whip"; break;
			case Soldier_t_TheEscapePlan:
				sReturnValue = "Escape Plan"; break;
			default: sReturnValue = "Shovel";
			}
			break;
		case TF_WEAPON_SLAP:
			sReturnValue = "Hot Hand"; break;
		case TF_WEAPON_SMG:
			sReturnValue = "SMG"; break;
		case TF_WEAPON_SNIPERRIFLE:
			switch (iWeaponIndex)
			{
			case Sniper_m_TheSydneySleeper:
				sReturnValue = "Sydney Sleeper"; break;
			case Sniper_m_TheMachina:
			case Sniper_m_ShootingStar:
				sReturnValue = "Machina"; break;
			case Sniper_m_TheHitmansHeatmaker:
				sReturnValue = "Hitmans Heatmaker"; break;
			default: sReturnValue = "Sniper Rifle";
			}
			break;
		case TF_WEAPON_SNIPERRIFLE_CLASSIC:
			sReturnValue = "Classic"; break;
		case TF_WEAPON_SNIPERRIFLE_DECAP:
			sReturnValue = "Bazaar Bargain"; break;
		case TF_WEAPON_SODA_POPPER:
			sReturnValue = "Soda Popper"; break;
		case TF_WEAPON_SPELLBOOK:
			sReturnValue = "Spellbook"; break;
		case TF_WEAPON_STICKBOMB:
			sReturnValue = "Caber"; break;
		case TF_WEAPON_SWORD:
			switch (iWeaponIndex)
			{
			case Demoman_t_TheScotsmansSkullcutter:
				sReturnValue = "Skullcutter"; break;
			case Demoman_t_TheClaidheamhMor:
				sReturnValue = "Claidheamh Mor"; break;
			case Demoman_t_ThePersianPersuader:
				sReturnValue = "Persian Persuader"; break;
			default: sReturnValue = "Eyelander";
			}
			break;
		case TF_WEAPON_SYRINGEGUN_MEDIC:
			switch (iWeaponIndex)
			{
			case Medic_m_TheBlutsauger:
				sReturnValue = "Blutsauger"; break;
			case Medic_m_TheOverdose:
				sReturnValue = "Overdose"; break;
			default: sReturnValue = "Syringe Gun";
			}
			break;
		case TF_WEAPON_WRENCH:
			switch (iWeaponIndex)
			{
			case Engi_t_TheSouthernHospitality:
				sReturnValue = "Southern Hospitality"; break;
			case Engi_t_TheJag:
				sReturnValue = "Jag"; break;
			case Engi_t_TheEurekaEffect:
				sReturnValue = "Eureka Effect"; break;
			case Engi_t_TheGunslinger:
				sReturnValue = "Gunslinger"; break;
			default: sReturnValue = "Wrench";
			}
			break;
		}
	}
	// if we havent found it yet just return a basic understanding of what it is
	if (FNV1A::Hash32(sReturnValue) == FNV1A::Hash32Const("Unknown Weapon"))
	{
		switch (SDK::GetWeaponType(pWeapon))
		{
		case EWeaponType::HITSCAN:
			sReturnValue = "Hitscan Weapon";
			break;
		case EWeaponType::PROJECTILE:
			sReturnValue = "Projectile Weapon";
			break;
		case EWeaponType::MELEE:
			sReturnValue = "Melee";
		}
	}
	
	return sReturnValue;
}

std::string CWeaponNames::GetWeaponNameUpper(CTFWeaponBase* pWeapon)
{
	auto sWeaponName = GetWeaponName(pWeapon);
	std::transform(sWeaponName.begin(), sWeaponName.end(), sWeaponName.begin(), ::toupper);
	return sWeaponName;
}

std::string CWeaponNames::GetWeaponNameLower(CTFWeaponBase* pWeapon)
{
	auto sWeaponName = GetWeaponName(pWeapon);
	std::transform(sWeaponName.begin(), sWeaponName.end(), sWeaponName.begin(), ::tolower);
	return sWeaponName;
}
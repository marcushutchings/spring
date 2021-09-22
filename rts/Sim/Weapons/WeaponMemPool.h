/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef WEAPON_MEMPOOL_H
#define WEAPON_MEMPOOL_H

#include "Sim/Misc/GlobalConstants.h"
#include "System/MemPoolTypes.h"
#include "System/SpringMath.h"

#include "Sim/Weapons/BeamLaser.h"
#include "Sim/Weapons/BombDropper.h"
#include "Sim/Weapons/Cannon.h"
#include "Sim/Weapons/DGunWeapon.h"
#include "Sim/Weapons/EmgCannon.h"
#include "Sim/Weapons/FlameThrower.h"
#include "Sim/Weapons/LaserCannon.h"
#include "Sim/Weapons/LightningCannon.h"
#include "Sim/Weapons/MeleeWeapon.h"
#include "Sim/Weapons/MissileLauncher.h"
#include "Sim/Weapons/NoWeapon.h"
#include "Sim/Weapons/PlasmaRepulser.h"
#include "Sim/Weapons/Rifle.h"
#include "Sim/Weapons/StarburstLauncher.h"
#include "Sim/Weapons/TorpedoLauncher.h"

static constexpr size_t largestWeaponSize = ConstexprMax
    ( sizeof(CBeamLaser)
    , sizeof(CBombDropper)
    , sizeof(CCannon)
    , sizeof(CDGunWeapon)
    , sizeof(CEmgCannon)
    , sizeof(CFlameThrower)
    , sizeof(CLaserCannon)
    , sizeof(CLightningCannon)
    , sizeof(CMeleeWeapon)
    , sizeof(CMissileLauncher)
    , sizeof(CNoWeapon)
    , sizeof(CPlasmaRepulser)
    , sizeof(CRifle)
    , sizeof(CStarburstLauncher)
    , sizeof(CTorpedoLauncher)
    );

static constexpr size_t WMP_S = AlignUp(largestWeaponSize, 4);

#if (defined(__x86_64) || defined(__x86_64__))
// NOTE: ~742MB, way too big for 32-bit builds
typedef StaticMemPool<MAX_UNITS * MAX_WEAPONS_PER_UNIT, WMP_S> WeaponMemPool;
#else
typedef FixedDynMemPool<WMP_S, (MAX_UNITS * MAX_WEAPONS_PER_UNIT) / 4000, (MAX_UNITS * MAX_WEAPONS_PER_UNIT) / 256> WeaponMemPool;
#endif

extern WeaponMemPool weaponMemPool;

#endif


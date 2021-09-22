/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef PROJECTILE_MEMPOOL_H
#define PROJECTILE_MEMPOOL_H

#include "Sim/Misc/GlobalConstants.h"
#include "System/MemPoolTypes.h"
#include "System/SpringMath.h"

#include "Sim/Projectiles/WeaponProjectiles/BeamLaserProjectile.h"
#include "Sim/Projectiles/WeaponProjectiles/EmgProjectile.h"
#include "Sim/Projectiles/WeaponProjectiles/ExplosiveProjectile.h"
#include "Sim/Projectiles/WeaponProjectiles/FireBallProjectile.h"
#include "Sim/Projectiles/WeaponProjectiles/FlameProjectile.h"
#include "Sim/Projectiles/WeaponProjectiles/LargeBeamLaserProjectile.h"
#include "Sim/Projectiles/WeaponProjectiles/LaserProjectile.h"
#include "Sim/Projectiles/WeaponProjectiles/LightningProjectile.h"
#include "Sim/Projectiles/WeaponProjectiles/MissileProjectile.h"
#include "Sim/Projectiles/WeaponProjectiles/StarburstProjectile.h"
#include "Sim/Projectiles/WeaponProjectiles/TorpedoProjectile.h"

static constexpr size_t largestProjectileSize = ConstexprMax
    ( sizeof(CBeamLaserProjectile)
    , sizeof(CEmgProjectile)
    , sizeof(CExplosiveProjectile)
    , sizeof(CFireBallProjectile)
    , sizeof(CFlameProjectile)
    , sizeof(CLargeBeamLaserProjectile)
    , sizeof(CLaserProjectile)
    , sizeof(CLightningProjectile)
    , sizeof(CMissileProjectile)
    , sizeof(CStarburstProjectile)
    , sizeof(CTorpedoProjectile)
    );

static constexpr size_t PMP_S = AlignUp(largestProjectileSize, 4);

#if (defined(__x86_64) || defined(__x86_64__))
typedef StaticMemPool<MAX_PROJECTILES, PMP_S> ProjMemPool;
#else
typedef FixedDynMemPool<PMP_S, MAX_PROJECTILES / 2000, MAX_PROJECTILES / 64> ProjMemPool;
#endif

extern ProjMemPool projMemPool;

#endif


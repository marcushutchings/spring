/* This file is part of the Spring engine (GPL v2 or later), see LICENSE.html */

#ifndef FLOAT3_H
#define FLOAT3_H

#ifdef USESSEFLOAT3
#include "System/float3sse.h"
#else
#include "System/float3nosse.h"
#endif


inline float3 operator*(float f, const float3& v) {
	return v * f;
}

/**
 * @brief upwards vector
 *
 * Defines constant upwards vector
 * (0, 1, 0)
 */
static constexpr float3  UpVector(0.0f, 1.0f, 0.0f);
static constexpr float3 FwdVector(0.0f, 0.0f, 1.0f);
static constexpr float3 RgtVector(1.0f, 0.0f, 0.0f);

/**
 * @brief zero vector
 *
 * Defines constant zero vector
 * (0, 0, 0)
 */
static constexpr float3 ZeroVector(0.0f, 0.0f, 0.0f);
static constexpr float3 OnesVector(1.0f, 1.0f, 1.0f);

static constexpr float3 XYVector(1.0f, 1.0f, 0.0f);
static constexpr float3 XZVector(1.0f, 0.0f, 1.0f);
static constexpr float3 YZVector(0.0f, 1.0f, 1.0f);

#endif /* FLOAT3_H */


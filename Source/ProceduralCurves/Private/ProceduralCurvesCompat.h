// Copyright PsinaDev. All Rights Reserved.

#pragma once

#include "Math/UnrealMathUtility.h"

// The UE_-prefixed math constants replaced PI and friends during UE 5; older 5.x releases only have the unprefixed ones.
#ifndef UE_PI
	#define UE_PI PI
#endif
#ifndef UE_TWO_PI
	#define UE_TWO_PI (2.0f * PI)
#endif
#ifndef UE_KINDA_SMALL_NUMBER
	#define UE_KINDA_SMALL_NUMBER KINDA_SMALL_NUMBER
#endif
#ifndef UE_EULERS_NUMBER
	#define UE_EULERS_NUMBER EULERS_NUMBER
#endif

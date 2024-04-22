// Copyright 2023, Dakota Dawe, All rights reserved


#include "Statics/SKGOpticLibrary.h"

float USKGOpticLibrary::MagnificationToFOVAngle(const float Magnification)
{
	return FMath::RadiansToDegrees(FMath::Atan(2.0f / Magnification) * 0.5f);
}

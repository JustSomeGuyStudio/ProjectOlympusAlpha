// Copyright 2023, Dakota Dawe, All rights reserved


#include "Statics/SKGMuzzleHelpers.h"

#include "Components/SKGMuzzleComponent.h"

FTransform USKGMuzzleHelpers::Conv_MuzzleTransformToTransform(const FSKGMuzzleTransform& MuzzleTransform)
{
	return MuzzleTransform.ConvertToTransform();
}

TArray<FTransform> USKGMuzzleHelpers::Conv_MuzzleTransformsToTransforms(const TArray<FSKGMuzzleTransform>& MuzzleTransforms)
{
	TArray<FTransform> Transforms;
	Transforms.Reserve(MuzzleTransforms.Num());
	for (const FSKGMuzzleTransform& MuzzleTransform : MuzzleTransforms)
	{
		Transforms.Add(MuzzleTransform.ConvertToTransform());
	}
	return Transforms;
}

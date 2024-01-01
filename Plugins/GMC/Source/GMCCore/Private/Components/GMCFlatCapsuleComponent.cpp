// Copyright 2022-2023 Dominik Lips. All Rights Reserved.

#include "GMCFlatCapsuleComponent.h"

UGMC_FlatCapsuleCmp::UGMC_FlatCapsuleCmp()
{
  ShapeColor = FColor(223, 149, 157, 255);
  CapsuleRotation = FLAT_ROTATION;
  CapsuleRadius = 22.f;
  CapsuleHalfHeight = 44.f;
  bUseEditorCompositing = true;
}

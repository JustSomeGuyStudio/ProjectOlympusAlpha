// Copyright 2022-2023 Dominik Lips. All Rights Reserved.
#pragma once

#include "Modules/ModuleManager.h"

class FGMCCoreModule : public IModuleInterface
{
public:

  virtual void StartupModule() override;
  virtual void ShutdownModule() override;
};

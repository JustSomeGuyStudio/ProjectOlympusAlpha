// Copyright 2023, Erik Hedberg. All Rights Reserved.

#include "TerminalBallistics.h"

DEFINE_LOG_CATEGORY(TerminalBallistics);

#define LOCTEXT_NAMESPACE "FTerminalBallistics"

void FTerminalBallistics::StartupModule()
{
	//UE_LOG(TerminalBallistics, Warning, TEXT("TerminalBallistics module has been loaded"));
}

void FTerminalBallistics::ShutdownModule()
{
	//UE_LOG(TerminalBallistics, Warning, TEXT("TerminalBallistics module has been unloaded"));
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FTerminalBallistics, TerminalBallistics)
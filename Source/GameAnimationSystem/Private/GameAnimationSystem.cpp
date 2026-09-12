// Copyright Epic Games, Inc. All Rights Reserved.

#include "GameAnimationSystem.h"

DEFINE_LOG_CATEGORY(LogGASP);
DEFINE_LOG_CATEGORY(LogGASPAnim);
DEFINE_LOG_CATEGORY(LogGASPMover);

#define LOCTEXT_NAMESPACE "FGameAnimationSystemModule"

void FGameAnimationSystemModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FGameAnimationSystemModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FGameAnimationSystemModule, GameAnimationSystem)

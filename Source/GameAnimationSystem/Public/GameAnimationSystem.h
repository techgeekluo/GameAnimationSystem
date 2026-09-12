// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Modules/ModuleManager.h"

DECLARE_LOG_CATEGORY_EXTERN(LogGASP, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogGASPAnim, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogGASPMover, Log, All);

class FGameAnimationSystemModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};

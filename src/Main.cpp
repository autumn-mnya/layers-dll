#include <windows.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include "mod_loader.h"
#include "cave_story.h"
#include "Layers.h"
#include "AutPI.h"

void RegisterHooks()
{
	// all of this just to load animated tiles lol
	RegisterPreModeElement(InitHook);
	RegisterOpeningInitElement(InitHook);
	RegisterOpeningActionElement(ActHook);
	RegisterInitElement(InitHook);
	RegisterActionElement(ActHook);
	RegisterTransferStageInitElement(TransferStageHook);
}

void InitMod(void)
{
	// Same as periwinkle dll patches, but now its a mod loader dll at heart
	LoadAutPiDll(); // load autpi
	layers_mode::applyLayersPatch();
	layers_mode::applyTSCPatch();
	layers_mode::fixGraphicsEnhancementCompatibility();
	RegisterHooks(); // register all of our jank ass hooks

	pause_animated_tiles_on_pri = ModLoader_GetSettingBool("Pause Animated Tiles on PRI", true);
	MapBufferSize = ModLoader_GetSettingInt("Map Buffer Size", 614400);
}

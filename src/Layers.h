#pragma once

#include "patch_utils.h"
#include "doukutsu/npc.h"
#include "doukutsu/tsc.h"

extern bool pause_animated_tiles_on_pri;
extern int MapBufferSize;

void InitHook();
void TransferStageHook();
void ActHook();

namespace layers_mode
{

	extern int screenTileWidth;
	extern int screenTileHeight;

	// Replacing gMap entirely is too much hassle, so let's just make a new struct with the added data
	struct LAYERSDATA
	{
		unsigned short* data;
		unsigned short* farBackData;
		unsigned short* backData;
		unsigned short* frontData;
		unsigned char atrb[0x10000]; // Expanded gMap.atrb to support 16-bit PXMs
	};

	extern LAYERSDATA gLayers;

	void applyLayersPatch();
	void applyTSCPatch();

	// Call this from applyPostInitPatches() if using the modloader with graphics_enhancement
	void fixGraphicsEnhancementCompatibility();

	inline void applyPatch()
	{
		applyLayersPatch();
		applyTSCPatch();
	}

	extern "C" __declspec(dllexport) csvanilla::BOOL ChangeMapLayerNoSmoke(int x, int y, unsigned short no, int layer);
	

	extern "C" __declspec(dllexport) void ShiftMapLayer(int x, int y, int layer);
	extern "C" __declspec(dllexport) csvanilla::BOOL ChangeMapLayer(int x, int y, unsigned short no, int layer);
}

extern "C" __declspec(dllexport) int GetLayerBuffer();
extern "C" __declspec(dllexport) unsigned short* GetLayerFront();
extern "C" __declspec(dllexport) unsigned short* GetLayerFarFront();
extern "C" __declspec(dllexport) unsigned short* GetLayerBack();
extern "C" __declspec(dllexport) unsigned short* GetLayerFarBack();
extern "C" __declspec(dllexport) void SetLayerFront(unsigned short* data, size_t size);
extern "C" __declspec(dllexport) void SetLayerFarFront(unsigned short* data, size_t size);
extern "C" __declspec(dllexport) void SetLayerBack(unsigned short* data, size_t size);
extern "C" __declspec(dllexport) void SetLayerFarBack(unsigned short* data, size_t size);
extern "C" __declspec(dllexport) void SetLayersAttribute(int index, int input);
extern "C" __declspec(dllexport) unsigned char* GetLayersAttribute();
extern "C" __declspec(dllexport) unsigned char GetLayersAttributeIndex(int index);
extern "C" __declspec(dllexport) int GetLayerMapWidth();
extern "C" __declspec(dllexport) int GetLayerMapHeight();

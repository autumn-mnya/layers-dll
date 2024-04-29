#pragma once

extern bool pause_animated_tiles_on_pri;

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

}
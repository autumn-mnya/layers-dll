// Layers mode implementation
// idk how this even works, I'm just copy/pasting code (mostly)

#include "layers.h"
#include "patch_utils.h"
#include "doukutsu/cstdlib.h"
#include "doukutsu/map.h"
#include "doukutsu/draw.h"
#include "doukutsu/misc.h"
#include "doukutsu/npc.h"
#include "doukutsu/window.h"

#include <iostream>
#include <fstream>
#include <map>
#include <yaml-cpp/yaml.h>

bool pause_animated_tiles_on_pri = true;


typedef struct STAGE_TABLE
{
	char parts[0x20];
	char map[0x20];
	int bkType;
	char back[0x20];
	char npc[0x20];
	char boss[0x20];
	signed char boss_no;
	char name[0x20];
} STAGE_TABLE;

static STAGE_TABLE* mTMT = (STAGE_TABLE*)(*(unsigned*)0x420C2F); // This is a pointer to where it gets used, instead of the actual table, so that it has compatibility with mods.

int put_front_ran = 0;

int MapBufferSize = 0x96000;

// Define your struct for animated tiles
struct AnimatedTile {
	int id;
	int frames;
	int frame_timer;
};

#define TILE_REP_MAX 0x10000 // same as 
AnimatedTile tile_replacements[TILE_REP_MAX];

// Function to register animated tile data
void RegisterAnimatedTile(int id, int frames, int frame_timer) {
	if (id < TILE_REP_MAX) {
		tile_replacements[id].id = id;
		tile_replacements[id].frames = frames;
		tile_replacements[id].frame_timer = frame_timer;
	}
	else {
		fprintf(stderr, "Error: Tile ID exceeds maximum allowed value.\n");
	}
}

// Function to reset animated tile data
void ResetAnimatedTiles() {
	for (int i = 0; i < TILE_REP_MAX; ++i) {
		tile_replacements[i].id = 0;
	}
}

// Function to load animated tile data from YAML file
void LoadAnimatedTileData(const char* yamlPath) {
	// Parse the YAML file
	YAML::Node data;
	try {
		data = YAML::LoadFile(yamlPath);
	}
	catch (const std::exception& e) {
		// fprintf(stderr, "Error: Failed to open or parse YAML file: %s\n", e.what());
		return; // Return early if file isn't found or can't be parsed
	}

	// Iterate over each tile ID in the YAML data
	for (const auto& tile : data["tiles"]) {
		uint16_t tileID = tile.first.as<int>();
		const YAML::Node& tileNode = tile.second;

		// Register the animated tile data
		RegisterAnimatedTile(tileID, tileNode["frames"].as<int>(), tileNode["delay"].as<int>());
	}
}


void LoadStageAnimatedTile(int no)
{
	char path[MAX_PATH];
	sprintf(path, "%s\\%s\\%s%s", csvanilla::gDataPath, "Stage", mTMT[no].parts, ".ani.yaml");
	LoadAnimatedTileData(path);
}

bool has_transferred_stage = false;

// Ran during the games "init" elements in ModeOpening and ModeAction
void InitHook()
{
	put_front_ran = 0;
	ResetAnimatedTiles();
	has_transferred_stage = false;
}

// Ran at the end of the TransferStage() function
void TransferStageHook()
{
	put_front_ran = 0;
	ResetAnimatedTiles();
	has_transferred_stage = true;
}

// Ran during the games "action" elements in ModeOpening and ModeAction
void ActHook()
{
	if (has_transferred_stage)
	{
		LoadStageAnimatedTile(csvanilla::gStageNo); // load animated tiles file for stage id we're currently in (if it exists)
		has_transferred_stage = false;
	}
}

namespace layers_mode
{
	int screenTileWidth = 21;
	int screenTileHeight = 16;

	LAYERSDATA gLayers;

	csvanilla::BOOL InitMapData2()
	{
		using csvanilla::malloc;
		gLayers.data = (unsigned short*)malloc(MapBufferSize);
		gLayers.farBackData = (unsigned short*)malloc(MapBufferSize);
		gLayers.backData = (unsigned short*)malloc(MapBufferSize);
		gLayers.frontData = (unsigned short*)malloc(MapBufferSize);
		return 1;
	}

	void ReadLayerData(unsigned short* dest, int size, csvanilla::FILE* fp)
	{
		csvanilla::fread(dest, 2, size, fp);
	}

	// Replaces the fread(gMap.data, 1, gMap.width * gMap.length, fp); call at the end of LoadMapData2()
	void LoadMapData2_hook(void*, unsigned pxmLayersCheck, unsigned mapSize, csvanilla::FILE* fp)
	{
		// Check if this is a layers PXM
		if ((pxmLayersCheck & 0xFF000000) == 0x21000000)
		{
			ReadLayerData(gLayers.farBackData, mapSize, fp);
			ReadLayerData(gLayers.backData, mapSize, fp);
			ReadLayerData(gLayers.data, mapSize, fp);
			ReadLayerData(gLayers.frontData, mapSize, fp);
		}
		else
		{
			// Otherwise, load this like a normal map
			csvanilla::memset(gLayers.farBackData, 0, mapSize * 2);
			csvanilla::memset(gLayers.backData, 0, mapSize * 2);
			csvanilla::memset(gLayers.frontData, 0, mapSize * 2);

			unsigned char* tmp = (unsigned char*)csvanilla::malloc(mapSize);
			csvanilla::fread(tmp, 1, mapSize, fp);
			for (unsigned i = 0; i < mapSize; ++i)
			{
				gLayers.data[i] = tmp[i];
			}
			csvanilla::free(tmp);
		}
	}

	void EndMapData()
	{
		using csvanilla::free;
		free(gLayers.data);
		free(gLayers.farBackData);
		free(gLayers.backData);
		free(gLayers.frontData);
	}

	// Since we're making gMap.data an unsigned short* instead of unsigned char*, that changes all of the
	// offsets for the array indexes, so I guess I have to replace every function that uses gMap.data... :/
	unsigned char GetAttribute(int x, int y)
	{
		using csvanilla::gMap;
		if (x < 0 || y < 0 || x >= gMap.width || y >= gMap.length)
			return 0;
		return gLayers.atrb[gLayers.data[x + y * gMap.width]];
	}
	void DeleteMapParts(int x, int y)
	{
		gLayers.data[x + y * csvanilla::gMap.width] = 0;
	}
	void ShiftMapParts(int x, int y)
	{
		--gLayers.data[x + y * csvanilla::gMap.width];
	}
	csvanilla::BOOL ChangeMapParts(int x, int y, unsigned short no)
	{
		unsigned short& tile = gLayers.data[x + y * csvanilla::gMap.width];
		if (tile == no)
			return 0;
		tile = no;
		for (int i = 0; i < 3; ++i)
			csvanilla::SetNpChar(4, x * 0x2000, y * 0x2000, 0, 0, 0, nullptr, 0);
		return 1;
	}

	template <typename Func1, typename Func2>
	void PutStage_Layer(unsigned short* data, int fx, int fy, Func1 skipCond, Func2 drawExtra)
	{
		using namespace csvanilla;

		// Get range to draw
		const int put_x = ((fx / 0x200) + 8) / 16;
		const int put_y = ((fy / 0x200) + 8) / 16;

		for (int j = put_y; j < put_y + screenTileHeight; ++j)
		{
			for (int i = put_x; i < put_x + screenTileWidth; ++i)
			{
				// Get attribute
				int offset = (j * gMap.width) + i;

				if (data[offset] == 0)
					continue;

				int atrb = GetAttribute(i, j);

				if (skipCond(atrb))
					continue;

				// Draw tile
				::RECT rect;
				int dataAni = data[offset];
				AnimatedTile aTile = tile_replacements[dataAni];
				if (aTile.id != 0) {
					dataAni += (put_front_ran / aTile.frame_timer) % aTile.frames;
				}
				rect.left = (dataAni % 16) * 16;
				rect.top = (dataAni / 16) * 16;
				rect.right = rect.left + 16;
				rect.bottom = rect.top + 16;

				int x = ((i * 16) - 8) - (fx / 0x200), y = ((j * 16) - 8) - (fy / 0x200);
				PutBitmap3(&grcGame, x, y, &rect, 2);

				// For PutStage_Front
				if (atrb == 0x43)
					drawExtra(x, y);
			}
		}
	}

	// Separating this out is assured to have virtually no performance impact, but
	// people have been having lag issues with this hack, so I may as well try anyways
	void PutStage_Layer(unsigned short* data, int fx, int fy)
	{
		using namespace csvanilla;

		// Get range to draw
		const int put_x = ((fx / 0x200) + 8) / 16;
		const int put_y = ((fy / 0x200) + 8) / 16;

		for (int j = put_y; j < put_y + screenTileHeight; ++j)
		{
			for (int i = put_x; i < put_x + screenTileWidth; ++i)
			{
				int offset = (j * gMap.width) + i;

				if (data[offset] == 0)
					continue;

				// Draw tile
				::RECT rect;
				int dataAni = data[offset];
				AnimatedTile aTile = tile_replacements[dataAni];
				if (aTile.id != 0) {
					dataAni += (put_front_ran / aTile.frame_timer) % aTile.frames;
				}
				rect.left = (dataAni % 16) * 16;
				rect.top = (dataAni / 16) * 16;
				rect.right = rect.left + 16;
				rect.bottom = rect.top + 16;

				PutBitmap3(&grcGame, ((i * 16) - 8) - (fx / 0x200), ((j * 16) - 8) - (fy / 0x200), &rect, 2);
			}
		}
	}

	void PutStage_Back(int fx, int fy)
	{
		// Draw back layers
		PutStage_Layer(gLayers.farBackData, fx, fy);
		PutStage_Layer(gLayers.backData, fx, fy);

		// This is vanilla behavior
		PutStage_Layer(gLayers.data, fx, fy, [](int atrb) { return atrb >= 0x20; }, [](int, int) {});
	}

	void PutStage_Front(int fx, int fy)
	{
		// This is vanilla behavior
		if (!pause_animated_tiles_on_pri || (csvanilla::g_GameFlags & 3))
			put_front_ran++;

		PutStage_Layer(gLayers.data, fx, fy, [](int atrb) { return atrb < 0x40 || atrb >= 0x80; },
			[](int x, int y)
		{
			const ::RECT rcSnack = { 256, 48, 272, 64 };
			csvanilla::PutBitmap3(&csvanilla::grcGame, x, y, &rcSnack, 20);
		});

		// Draw the far-front layer
		PutStage_Layer(gLayers.frontData, fx, fy);
	}

	csvanilla::BOOL RecreateTilesetSurface(const char* name, int surf_no)
	{
		csvanilla::ReleasePartsImage();
		return csvanilla::MakeSurface_File(name, surf_no);
	}


	void applyLayersPatch()
	{
		using patcher::byte;
		patcher::replaceFunction(csvanilla::InitMapData2, InitMapData2);
		// if (gMap.data == NULL) --> if (gLayers.data == NULL) in LoadMapData2
		const unsigned short* const* const dataPtr = &gLayers.data;
		patcher::patchBytes(0x413842, reinterpret_cast<const byte* const>(&dataPtr), 4);
		// Pass PXM header dummy byte into LoadMapData2_hook to check for layers PXM
		const byte patchArgs[] = { 0xFF, 0x75, 0xF4, 0xFF, 0x35, 0x80, 0xE4, 0x49, 0x00 };
		patcher::patchBytes(0x41386F, patchArgs, sizeof patchArgs);
		// Load layers
		patcher::writeCall(0x413878, LoadMapData2_hook);

		// Increase size of atrb array
		patcher::dword atrbSize = sizeof gLayers.atrb;
		patcher::patchBytes(0x4138F4, reinterpret_cast<byte*>(&atrbSize), 4); // New size to memset
		// Load PXA data into gLayers.atrb instead of gMap.atrb
		void* atrbPtr = &gLayers.atrb;
		patcher::patchBytes(0x4138FB, reinterpret_cast<byte*>(&atrbPtr), 4);

		patcher::replaceFunction(csvanilla::EndMapData, EndMapData);
		patcher::replaceFunction(csvanilla::GetAttribute, GetAttribute);
		patcher::replaceFunction(csvanilla::DeleteMapParts, DeleteMapParts);
		patcher::replaceFunction(csvanilla::ShiftMapParts, ShiftMapParts);
		patcher::replaceFunction(csvanilla::ChangeMapParts, ChangeMapParts);
		// Don't truncate <CMP argument to 1 byte
		byte pushEAX = 0x50;
		patcher::patchBytes(0x424B30, &pushEAX, 1);

		patcher::replaceFunction(csvanilla::PutStage_Back, PutStage_Back);
		patcher::replaceFunction(csvanilla::PutStage_Front, PutStage_Front);

		// Recreate tileset surface when loading a new stage
		patcher::writeCall(0x420C55, RecreateTilesetSurface); // Replaces the call to ReloadBitmap_File()
	}

	void fixGraphicsEnhancementCompatibility()
	{
		// In case of graphics_enhancement with widescreen, reapply the GetAttribute() patch
		patcher::replaceFunction(csvanilla::GetAttribute, GetAttribute);
		// Get screen dimensions (in case of widescreen)
		ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<void*>(0x413AF9), &screenTileWidth, 4, NULL);
		// The modloader hack doesn't write to this one?
		//ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<void*>(0x413B00), &screenTileHeight, 4, NULL);
	}

}

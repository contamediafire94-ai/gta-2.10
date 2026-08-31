#include "../main.h"
#include "../game/game.h"
#include "../vendor/armhook/patch.h"
#include "vehicleColoursTable.h"
#include "../settings.h"
extern CSettings* pSettings;

VehicleAudioPropertiesStruct VehicleAudioProperties[20000];
#include "game.h"
#include "World.h"
#include "net/netgame.h"

extern CGame* pGame;
void readVehiclesAudioSettings()
{

	char vehicleModel[50];
	int16_t pIndex = 0;

	FILE* pFile;

	char line[300];

	// Zero VehicleAudioProperties
	memset(VehicleAudioProperties, 0x00, sizeof(VehicleAudioProperties));

	VehicleAudioPropertiesStruct CurrentVehicleAudioProperties;

	memset(&CurrentVehicleAudioProperties, 0x0, sizeof(VehicleAudioPropertiesStruct));

	char buffer[0xFF];
	sprintf(buffer, "%sSAMP/vehicleAudioSettings.cfg", g_pszStorage);
	pFile = fopen(buffer, "r");
	if (!pFile)
	{
		//Log("Cannot read vehicleAudioSettings.cfg");
		return;
	}

	// File exists
	while (fgets(line, sizeof(line), pFile))
	{
		if (strncmp(line, ";the end", 8) == 0)
			break;

		if (line[0] == ';')
			continue;

		sscanf(line, "%s %d %d %d %d %f %f %d %f %d %d %d %d %f",
			   vehicleModel,
			   &CurrentVehicleAudioProperties.VehicleType,
			   &CurrentVehicleAudioProperties.EngineOnSound,
			   &CurrentVehicleAudioProperties.EngineOffSound,
			   &CurrentVehicleAudioProperties.field_4,
			   &CurrentVehicleAudioProperties.field_5,
			   &CurrentVehicleAudioProperties.field_6,
			   &CurrentVehicleAudioProperties.HornTon,
			   &CurrentVehicleAudioProperties.HornHigh,
			   &CurrentVehicleAudioProperties.DoorSound,
			   &CurrentVehicleAudioProperties.RadioNum,
			   &CurrentVehicleAudioProperties.RadioType,
			   &CurrentVehicleAudioProperties.field_14,
			   &CurrentVehicleAudioProperties.field_16);

		((void (*)(const char* thiz, int16_t* a2))(g_libGTASA + 0x385E38 + 1))(vehicleModel, &pIndex);
		memcpy(&VehicleAudioProperties[pIndex-400], &CurrentVehicleAudioProperties, sizeof(VehicleAudioPropertiesStruct));


	}

	fclose(pFile);
}

void ApplyFPSPatch(uint8_t fps)
{
#if VER_x32
    CHook::WriteMemory(g_libGTASA + 0x005E49E0, (uintptr_t)& fps, 1);
	CHook::WriteMemory(g_libGTASA + 0x005E492E, (uintptr_t)& fps, 1);
#else
    CHook::WriteMemory(g_libGTASA + 0x70A38C, "\xE9\x0F\x1E\x32", 4);
    CHook::WriteMemory(g_libGTASA + 0x70A43C, "\xE8\x0F\x1E\x32", 4);
    CHook::WriteMemory(g_libGTASA + 0x70A458, "\xE9\x0F\x1E\x32", 4);
#endif

    FLog("New fps limit = %d", fps);
}

void DisableAutoAim()
{
    CHook::RET("_ZN10CPlayerPed22FindWeaponLockOnTargetEv"); // CPed::FindWeaponLockOnTarget
    CHook::RET("_ZN10CPlayerPed26FindNextWeaponLockOnTargetEP7CEntityb"); // CPed::FindNextWeaponLockOnTarget
    CHook::RET("_ZN4CPed21SetWeaponLockOnTargetEP7CEntity"); // CPed::SetWeaponLockOnTarget
}

void ApplySAMPPatchesInGame()
{
    // TEMP TEST:
    // Disable all in-game patches to verify whether one of the hardcoded
    // libGTASA offsets is causing the SIGBUS during main.scm loading.
    FLog("ApplySAMPPatchesInGame: temporarily disabled for crash test");
    return;
}

int32_t CWorld__FindPlayerSlotWithPedPointer(CPedGTA* pPlayersPed)
{
    for(int i = 0; i < MAX_PLAYERS; ++i)
    {
        if(CWorld::Players[i].m_pPed == pPlayersPed)
            return i;
    }
    return -1;
}

void ApplyPatches_level0()
{
    FLog("ApplyPatches_level0");

    CHook::Write(g_libGTASA + (VER_x32 ? 0x006783C0 : 0x84E7A8), &CWorld::Players);
    CHook::Write(g_libGTASA + (VER_x32 ? 0x00679B5C : 0x8516D8), &CWorld::PlayerInFocus);

    CHook::Redirect("_ZN6CWorld28FindPlayerSlotWithPedPointerEPv", &CWorld__FindPlayerSlotWithPedPointer);

// fix aplha raster
#if VER_x32
    CHook::WriteMemory(g_libGTASA + 0x001AE8DE, (uintptr_t)"\x01\x22", 2);
#else
    CHook::WriteMemory(g_libGTASA + 0x23FDE0, (uintptr_t)"\x22\x00\x80\x52", 4);
#endif

    //CHook::RET("_ZN6CTrain10InitTrainsEv"); // CTrain::InitTrains

    CHook::RET("_ZN8CClothes4InitEv"); // CClothes::Init()
    CHook::RET("_ZN8CClothes13RebuildPlayerEP10CPlayerPedb"); // CClothes::RebuildPlayer

    CHook::RET("_ZNK35CPedGroupDefaultTaskAllocatorRandom20AllocateDefaultTasksEP9CPedGroupP4CPed"); // AllocateDefaultTasks
    CHook::RET("_ZN6CGlass4InitEv"); // CGlass::Init
    CHook::RET("_ZN8CGarages17Init_AfterRestartEv"); // CGarages::Init_AfterRestart
    CHook::RET("_ZN6CGangs10InitialiseEv"); // CGangs::Initialise
    CHook::RET("_ZN5CHeli9InitHelisEv"); // CHeli::InitHelis(void)
    CHook::RET("_ZN11CFileLoader10LoadPickupEPKc"); // CFileLoader::LoadPickup
    CHook::RET("_ZN14CLoadingScreen15DisplayPCScreenEv"); // Loading screen

    // entryexit
    //CHook::RET("_ZN17CEntryExitManager4InitEv");
   // CHook::RET("_ZN17CEntryExitManager22PostEntryExitsCreationEv");

    CHook::RET("_ZN10CSkidmarks6UpdateEv"); // CSkidmarks::Update
    CHook::RET("_ZN10CSkidmarks6RenderEv"); // CSkidmarks::Render

    //CHook::RET("_ZN14SurfaceInfos_c17CreatesWheelGrassEj"); // SurfaceInfos_c::CreatesWheelGrass
    //CHook::RET("_ZN14SurfaceInfos_c18CreatesWheelGravelEj"); // SurfaceInfos_c::CreatesWheelGravel
    //CHook::RET("_ZN14SurfaceInfos_c15CreatesWheelMudEj"); // SurfaceInfos_c::CreatesWheelMud
    CHook::RET("_ZN14SurfaceInfos_c16CreatesWheelDustEj"); // SurfaceInfos_c::CreatesWheelDust
    //CHook::RET("_ZN14SurfaceInfos_c16CreatesWheelSandEj"); // SurfaceInfos_c::CreatesWheelSand
    CHook::RET("_ZN14SurfaceInfos_c17CreatesWheelSprayEj"); // SurfaceInfos_c::CreatesWheelSpray

    //CHook::RET("_ZN4Fx_c13AddWheelGrassEP8CVehicle7CVectorhf"); // Fx_c::AddWheelGrass
    //CHook::RET("_ZN4Fx_c14AddWheelGravelEP8CVehicle7CVectorhf"); // Fx_c::AddWheelGravel
    //CHook::RET("_ZN4Fx_c11AddWheelMudEP8CVehicle7CVectorhf"); // Fx_c::AddWheelMud
    CHook::RET("_ZN4Fx_c12AddWheelDustEP8CVehicle7CVectorhf"); // Fx_c::AddWheelDust
    //CHook::RET("_ZN4Fx_c12AddWheelSandEP8CVehicle7CVectorhf"); // Fx_c::AddWheelSand
    CHook::RET("_ZN4Fx_c13AddWheelSprayEP8CVehicle7CVectorhhf"); // Fx_c::AddWheelSpray
}

void ApplyGlobalPatches()
{
    // TEMP TEST:
    // Disable global patches to check whether a hardcoded libGTASA offset
    // is causing the SIGBUS while main.scm is loading.
    FLog("ApplyGlobalPatches: temporarily disabled for crash test");
    return;
}

void InstallVehicleEngineLightPatches()
{
	// типо фикс задних фар
	CHook::WriteMemory(g_libGTASA + 0x591272, (uintptr_t)"\x02", 1);
	CHook::WriteMemory(g_libGTASA + 0x59128E, (uintptr_t)"\x02", 1);
}
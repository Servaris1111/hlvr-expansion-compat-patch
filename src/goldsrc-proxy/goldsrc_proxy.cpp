#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdlib>
#include <cstring>

#include "extdll.h"

#ifndef ORIGINAL_DLL_NAME
#define ORIGINAL_DLL_NAME "opfor_stock.dll"
#endif

namespace
{
HMODULE g_proxyModule = nullptr;
HMODULE g_originalModule = nullptr;

enginefuncs_t g_engineFuncs{};
globalvars_t* g_globalVars = nullptr;

DLL_FUNCTIONS g_originalFunctions{};
bool g_hasOriginalFunctions = false;
enginefuncs_t g_forwardEngineFuncs{};
int g_msgVRControllerEnt = 0;
int g_weaponSequence = 0;
int g_weaponBody = 0;
float g_weaponAnimTime = 0.0f;

using GiveFnptrsToDllFn = void(__stdcall*)(enginefuncs_t*, globalvars_t*);
using GetEntityAPIFn = int (*)(DLL_FUNCTIONS*, int);
using GetEntityAPI2Fn = int (*)(DLL_FUNCTIONS*, int*);
using EntityFactoryFn = void (*)(entvars_t*);

bool IsSameCommand(const char* lhs, const char* rhs)
{
	return lhs && rhs && _stricmp(lhs, rhs) == 0;
}

bool IsHLVRClientCommand(const char* command)
{
	static const char* const commands[] = {
		"VModEnable",
		"vrupd_hmd",
		"vrupdctrl",
		"vrtele",
		"vrspeech",
		"vr_flashlight",
		"vr_teleporter",
		"vr_anlgfire",
		"vr_lngjump",
		"vr_restartmap",
		"vr_wpnanim",
		"vr_muzzleflash",
	};

	for (const char* candidate : commands)
	{
		if (IsSameCommand(command, candidate))
		{
			return true;
		}
	}

	return false;
}

float CurrentTime()
{
	return g_globalVars ? g_globalVars->time : 0.0f;
}

void WriteFloat(float value)
{
	int bits = 0;
	static_assert(sizeof(bits) == sizeof(value), "GoldSrc float messages are 32-bit");
	memcpy(&bits, &value, sizeof(value));
	g_engineFuncs.pfnWriteLong(bits);
}

void EnsureVRControllerMessage()
{
	if (!g_msgVRControllerEnt && g_engineFuncs.pfnRegUserMsg)
	{
		g_msgVRControllerEnt = g_engineFuncs.pfnRegUserMsg("VRCtrlEnt", -1);
	}
}

void RegisterHLVRMessages()
{
	EnsureVRControllerMessage();
}

const char* EngineString(string_t value)
{
	if (!value || !g_engineFuncs.pfnSzFromIndex)
	{
		return "";
	}

	const char* result = g_engineFuncs.pfnSzFromIndex(value);
	return result ? result : "";
}

bool HasSuit(const edict_t* entity)
{
	const unsigned int weapons = entity ? static_cast<unsigned int>(entity->v.weapons) : 0;
	return (weapons & (1u << 31)) != 0;
}

float CVarFloatOrDefault(const char* name, float fallback)
{
	if (!name || !g_engineFuncs.pfnCVarGetFloat)
	{
		return fallback;
	}

	const float value = g_engineFuncs.pfnCVarGetFloat(name);
	return value == 0.0f ? fallback : value;
}

float ControllerModelScale()
{
	float weaponScale = CVarFloatOrDefault("vr_weaponscale", 1.0f);
	if (weaponScale < 0.01f)
	{
		weaponScale = 1.0f;
	}

	float worldScale = CVarFloatOrDefault("vr_world_scale", 1.0f);
	if (worldScale < 0.1f)
	{
		worldScale = 0.1f;
	}
	else if (worldScale > 100.0f)
	{
		worldScale = 100.0f;
	}

	return weaponScale / worldScale;
}

const char* ControllerSafeWeaponModel(const char* modelName)
{
	if (!modelName || !*modelName)
	{
		return nullptr;
	}

	struct Mapping
	{
		const char* from;
		const char* to;
	};

	static const Mapping mappings[] = {
		{"models/v_357.mdl", "models/w_357.mdl"},
		{"models/v_9mmar.mdl", "models/w_9mmar.mdl"},
		{"models/v_9mmhandgun.mdl", "models/w_9mmhandgun.mdl"},
		{"models/v_crossbow.mdl", "models/w_crossbow.mdl"},
		{"models/v_crowbar.mdl", "models/w_crowbar.mdl"},
		{"models/v_egon.mdl", "models/w_egon.mdl"},
		{"models/v_gauss.mdl", "models/w_gauss.mdl"},
		{"models/v_grenade.mdl", "models/w_grenade.mdl"},
		{"models/v_hgun.mdl", "models/w_hgun.mdl"},
		{"models/v_rpg.mdl", "models/w_rpg.mdl"},
		{"models/v_satchel.mdl", "models/w_satchel.mdl"},
		{"models/v_satchel_radio.mdl", "models/p_satchel_radio.mdl"},
		{"models/v_shotgun.mdl", "models/w_shotgun.mdl"},
		{"models/v_squeak.mdl", "models/w_squeak.mdl"},
		{"models/v_tripmine.mdl", "models/p_tripmine.mdl"},
	};

	for (const Mapping& mapping : mappings)
	{
		if (_stricmp(modelName, mapping.from) == 0)
		{
			return mapping.to;
		}
	}

	return nullptr;
}

bool IsControllerSafeWeaponModel(const char* modelName)
{
	return modelName
		&& (_strnicmp(modelName, "models/w_", 9) == 0 || _strnicmp(modelName, "models/p_", 9) == 0);
}

const char* SelectControllerModel(edict_t* entity, int controllerId)
{
	constexpr int kWeaponController = 0;
	constexpr int kHandController = 1;

	if (controllerId == kWeaponController)
	{
		const char* viewModel = entity ? EngineString(entity->v.viewmodel) : "";
		if (viewModel && *viewModel && viewModel[0] != '*')
		{
			const char* safeWeaponModel = ControllerSafeWeaponModel(viewModel);
			if (safeWeaponModel)
			{
				return safeWeaponModel;
			}

			const char* playerWeaponModel = entity ? EngineString(entity->v.weaponmodel) : "";
			if (playerWeaponModel && *playerWeaponModel && playerWeaponModel[0] != '*')
			{
				return playerWeaponModel;
			}

			return viewModel;
		}
	}

	if (controllerId == kHandController || controllerId == kWeaponController)
	{
		return HasSuit(entity) ? "models/v_hand_hevsuit.mdl" : "models/v_hand_labcoat.mdl";
	}

	return "";
}

void SendVRControllerModel(edict_t* entity, bool isLeftHand, const char* modelName, int body, int sequence, bool isDragging)
{
	if (!entity || !modelName || !*modelName || !g_engineFuncs.pfnMessageBegin)
	{
		return;
	}

	EnsureVRControllerMessage();
	if (!g_msgVRControllerEnt)
	{
		return;
	}

	const bool isHandModel = _stricmp(modelName, "models/v_hand_hevsuit.mdl") == 0 || _stricmp(modelName, "models/v_hand_labcoat.mdl") == 0;
	if (isHandModel && isDragging)
	{
		sequence = 7; // FULLGRAB_START in HLVR's hand model sequence table.
	}
	else if (isHandModel && !isDragging)
	{
		sequence = 0;
	}
	else if (IsControllerSafeWeaponModel(modelName))
	{
		body = 0;
		sequence = 0;
	}

	g_engineFuncs.pfnMessageBegin(MSG_ONE, g_msgVRControllerEnt, nullptr, entity);
	g_engineFuncs.pfnWriteByte(isLeftHand ? 1 : 0);
	g_engineFuncs.pfnWriteByte(body);
	g_engineFuncs.pfnWriteByte(0); // skin
	WriteFloat(ControllerModelScale());

	g_engineFuncs.pfnWriteLong(sequence);
	WriteFloat(0.0f); // frame
	WriteFloat(1.0f); // framerate
	WriteFloat(g_weaponAnimTime > 0.0f ? g_weaponAnimTime : CurrentTime());

	g_engineFuncs.pfnWriteString(modelName);
	g_engineFuncs.pfnWriteByte(0); // no dragged entity data in the compatibility bridge
	g_engineFuncs.pfnMessageEnd();
}

bool HandleVRControllerUpdate(edict_t* entity)
{
	if (!g_engineFuncs.pfnCmd_Argc || !g_engineFuncs.pfnCmd_Argv)
	{
		return true;
	}

	if (g_engineFuncs.pfnCmd_Argc() != 16)
	{
		return true;
	}

	const bool isValid = std::atoi(g_engineFuncs.pfnCmd_Argv(2)) != 0;
	const int controllerId = std::atoi(g_engineFuncs.pfnCmd_Argv(3));
	const bool isLeftHand = std::atoi(g_engineFuncs.pfnCmd_Argv(4)) != 0;
	const bool isDragging = std::atoi(g_engineFuncs.pfnCmd_Argv(14)) != 0;
	if (!isValid)
	{
		return true;
	}

	const char* modelName = SelectControllerModel(entity, controllerId);
	const int body = controllerId == 0 ? g_weaponBody : 0;
	const int sequence = controllerId == 0 ? g_weaponSequence : 0;
	SendVRControllerModel(entity, isLeftHand, modelName, body, sequence, isDragging);
	return true;
}

bool HandleVRWeaponAnimation()
{
	if (!g_engineFuncs.pfnCmd_Argc || !g_engineFuncs.pfnCmd_Argv)
	{
		return true;
	}

	if (g_engineFuncs.pfnCmd_Argc() >= 3)
	{
		g_weaponSequence = std::atoi(g_engineFuncs.pfnCmd_Argv(1));
		g_weaponBody = std::atoi(g_engineFuncs.pfnCmd_Argv(2));
		g_weaponAnimTime = CurrentTime();
	}
	return true;
}

bool GetOriginalDllPath(char* buffer, DWORD bufferSize)
{
	if (!buffer || bufferSize == 0 || !g_proxyModule)
	{
		return false;
	}

	const DWORD length = GetModuleFileNameA(g_proxyModule, buffer, bufferSize);
	if (length == 0 || length >= bufferSize)
	{
		return false;
	}

	char* lastSlash = strrchr(buffer, '\\');
	if (!lastSlash)
	{
		return false;
	}

	*(lastSlash + 1) = '\0';

	const char* originalName = ORIGINAL_DLL_NAME;
	const size_t remaining = bufferSize - strlen(buffer) - 1;
	strncat(buffer, originalName, remaining);
	return true;
}

bool EnsureOriginalLoaded()
{
	if (g_originalModule)
	{
		return true;
	}

	char originalPath[MAX_PATH]{};
	if (!GetOriginalDllPath(originalPath, sizeof(originalPath)))
	{
		return false;
	}

	g_originalModule = LoadLibraryA(originalPath);
	return g_originalModule != nullptr;
}

FARPROC GetOriginalProc(const char* name)
{
	if (!EnsureOriginalLoaded())
	{
		return nullptr;
	}

	return GetProcAddress(g_originalModule, name);
}

void ProxyCVarRegister(cvar_t* cvar)
{
	if (!cvar || !cvar->name)
	{
		return;
	}

	if (g_engineFuncs.pfnCVarGetPointer && g_engineFuncs.pfnCVarGetPointer(cvar->name))
	{
		return;
	}

	if (g_engineFuncs.pfnCVarRegister)
	{
		g_engineFuncs.pfnCVarRegister(cvar);
	}
}

void ProxyClientCommand(edict_t* entity)
{
	const char* command = nullptr;
	if (g_engineFuncs.pfnCmd_Argv)
	{
		command = g_engineFuncs.pfnCmd_Argv(0);
	}

	if (IsSameCommand(command, "vrupdctrl"))
	{
		HandleVRControllerUpdate(entity);
		return;
	}

	if (IsSameCommand(command, "vr_wpnanim"))
	{
		HandleVRWeaponAnimation();
		return;
	}

	if (IsHLVRClientCommand(command))
	{
		return;
	}

	if (g_hasOriginalFunctions && g_originalFunctions.pfnClientCommand)
	{
		g_originalFunctions.pfnClientCommand(entity);
	}
}

void ProxyGameInit()
{
	if (g_hasOriginalFunctions && g_originalFunctions.pfnGameInit)
	{
		g_originalFunctions.pfnGameInit();
	}

	RegisterHLVRMessages();
}

void PatchFunctionTable(DLL_FUNCTIONS* table)
{
	if (!table)
	{
		return;
	}

	g_originalFunctions = *table;
	g_hasOriginalFunctions = true;

	if (table->pfnGameInit)
	{
		table->pfnGameInit = ProxyGameInit;
	}

	if (table->pfnClientCommand)
	{
		table->pfnClientCommand = ProxyClientCommand;
	}
}

void ForwardEntityAs(const char* replacementClassName, entvars_t* pev)
{
	if (!replacementClassName || !pev)
	{
		return;
	}

	if (g_engineFuncs.pfnAllocString)
	{
		pev->classname = g_engineFuncs.pfnAllocString(replacementClassName);
	}

	const auto originalFactory = reinterpret_cast<EntityFactoryFn>(GetOriginalProc(replacementClassName));
	if (originalFactory)
	{
		originalFactory(pev);
	}
}
} // namespace

extern "C"
{
__declspec(dllexport) void __stdcall GiveFnptrsToDll(enginefuncs_t* engineFuncs, globalvars_t* globalVars)
{
	if (engineFuncs)
	{
		memcpy(&g_engineFuncs, engineFuncs, sizeof(g_engineFuncs));
		memcpy(&g_forwardEngineFuncs, engineFuncs, sizeof(g_forwardEngineFuncs));
		g_forwardEngineFuncs.pfnCVarRegister = ProxyCVarRegister;
		g_forwardEngineFuncs.pfnCvar_RegisterVariable = ProxyCVarRegister;
	}
	g_globalVars = globalVars;

	const auto originalGiveFnptrs = reinterpret_cast<GiveFnptrsToDllFn>(GetOriginalProc("GiveFnptrsToDll"));
	if (originalGiveFnptrs)
	{
		originalGiveFnptrs(engineFuncs ? &g_forwardEngineFuncs : engineFuncs, globalVars);
	}
}

__declspec(dllexport) int GetEntityAPI(DLL_FUNCTIONS* functionTable, int interfaceVersion)
{
	const auto originalGetEntityAPI = reinterpret_cast<GetEntityAPIFn>(GetOriginalProc("GetEntityAPI"));
	if (!originalGetEntityAPI)
	{
		return 0;
	}

	const int result = originalGetEntityAPI(functionTable, interfaceVersion);
	if (result)
	{
		PatchFunctionTable(functionTable);
	}
	return result;
}

__declspec(dllexport) int GetEntityAPI2(DLL_FUNCTIONS* functionTable, int* interfaceVersion)
{
	const auto originalGetEntityAPI2 = reinterpret_cast<GetEntityAPI2Fn>(GetOriginalProc("GetEntityAPI2"));
	if (!originalGetEntityAPI2)
	{
		return 0;
	}

	const int result = originalGetEntityAPI2(functionTable, interfaceVersion);
	if (result)
	{
		PatchFunctionTable(functionTable);
	}
	return result;
}

#ifdef ENABLE_OPFOR_VR_WEAPON_FALLBACKS
#define HLVR_ENTITY_FALLBACK(exportedName, replacementName) \
	__declspec(dllexport) void exportedName(entvars_t* pev) { ForwardEntityAs(replacementName, pev); }

HLVR_ENTITY_FALLBACK(ammo_556, "ammo_9mmAR")
HLVR_ENTITY_FALLBACK(ammo_762, "ammo_crossbow")
HLVR_ENTITY_FALLBACK(ammo_eagleclip, "ammo_357")
HLVR_ENTITY_FALLBACK(ammo_spore, "ammo_rpgclip")
HLVR_ENTITY_FALLBACK(weapon_eagle, "weapon_357")
HLVR_ENTITY_FALLBACK(weapon_knife, "weapon_crowbar")
HLVR_ENTITY_FALLBACK(weapon_m249, "weapon_9mmAR")
HLVR_ENTITY_FALLBACK(weapon_penguin, "weapon_snark")
HLVR_ENTITY_FALLBACK(weapon_pipewrench, "weapon_crowbar")
HLVR_ENTITY_FALLBACK(weapon_shockrifle, "weapon_gauss")
HLVR_ENTITY_FALLBACK(weapon_shockroach, "weapon_gauss")
HLVR_ENTITY_FALLBACK(weapon_sniperrifle, "weapon_crossbow")
HLVR_ENTITY_FALLBACK(weapon_sporelauncher, "weapon_rpg")

#undef HLVR_ENTITY_FALLBACK
#endif
}

BOOL WINAPI DllMain(HINSTANCE instance, DWORD reason, LPVOID)
{
	if (reason == DLL_PROCESS_ATTACH)
	{
		g_proxyModule = instance;
		DisableThreadLibraryCalls(instance);
	}
	return TRUE;
}

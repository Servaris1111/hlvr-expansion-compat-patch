#define WIN32_LEAN_AND_MEAN
#include <windows.h>

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

	if (IsHLVRClientCommand(command))
	{
		return;
	}

	if (g_hasOriginalFunctions && g_originalFunctions.pfnClientCommand)
	{
		g_originalFunctions.pfnClientCommand(entity);
	}
}

void PatchFunctionTable(DLL_FUNCTIONS* table)
{
	if (!table)
	{
		return;
	}

	g_originalFunctions = *table;
	g_hasOriginalFunctions = true;

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
HLVR_ENTITY_FALLBACK(weapon_displacer, "weapon_egon")
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

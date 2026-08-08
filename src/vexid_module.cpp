#include <cstdio>
#include <cstring>

#include "platform.h"
#include <extdll.h>
#include <eiface.h>

#ifndef _WIN32
#include <dlfcn.h>
#endif

#include <rehlds_api.h>
#include <reunion_api.h>
#include "amxxmodule.h"

#ifndef C_DLLEXPORT
#ifdef _WIN32
#define C_DLLEXPORT extern "C" __declspec(dllexport)
#else
#define C_DLLEXPORT extern "C" __attribute__((visibility("default")))
#endif
#endif

namespace
{
constexpr int kLongAuthIdBytes = IReunionApi::LONG_AUTHID_LEN;
constexpr int kLongAuthIdHexLength = kLongAuthIdBytes * 2;

IRehldsApi* g_rehldsApi = nullptr;
IReunionApi* g_reunionApi = nullptr;
amxxapi_t g_amxxApi = {};

enum ApiStatus
{
	API_UNINITIALIZED = 0,
	API_READY,
	API_ENGINE_NOT_FOUND,
	API_REHLDS_NOT_FOUND,
	API_REHLDS_VERSION_MISMATCH,
	API_REUNION_NOT_FOUND,
	API_REUNION_VERSION_MISMATCH
};

ApiStatus g_apiStatus = API_UNINITIALIZED;

const char* ApiStatusText(ApiStatus status)
{
	switch (status)
	{
		case API_READY: return "ready";
		case API_ENGINE_NOT_FOUND: return "engine module not found";
		case API_REHLDS_NOT_FOUND: return "ReHLDS API not found";
		case API_REHLDS_VERSION_MISMATCH: return "ReHLDS API version mismatch";
		case API_REUNION_NOT_FOUND: return "Reunion API not found";
		case API_REUNION_VERSION_MISMATCH: return "Reunion API version mismatch";
		default: return "not initialized";
	}
}

void* GetEngineModule()
{
#ifdef _WIN32
	const char* candidates[] = { "swds.dll", "sw.dll", "hw.dll" };
	for (const char* name : candidates)
	{
		if (HMODULE module = GetModuleHandleA(name))
			return module;
	}
	return nullptr;
#else
	static void* engineModule = nullptr;
	if (!engineModule)
		engineModule = dlopen("engine_i486.so", RTLD_NOW | RTLD_NOLOAD);
	return engineModule;
#endif
}

void* GetModuleSymbol(void* module, const char* symbol)
{
#ifdef _WIN32
	return reinterpret_cast<void*>(GetProcAddress(static_cast<HMODULE>(module), symbol));
#else
	return dlsym(module, symbol);
#endif
}

bool ResolveApis(bool forceRetry = false)
{
	if (g_apiStatus == API_READY && g_rehldsApi && g_reunionApi)
		return true;

	if (!forceRetry && g_apiStatus != API_UNINITIALIZED)
		return false;

	g_rehldsApi = nullptr;
	g_reunionApi = nullptr;

	void* engineModule = GetEngineModule();
	if (!engineModule)
	{
		g_apiStatus = API_ENGINE_NOT_FOUND;
		return false;
	}

	auto factory = reinterpret_cast<CreateInterfaceFn>(
		GetModuleSymbol(engineModule, CREATEINTERFACE_PROCNAME)
	);
	if (!factory)
	{
		g_apiStatus = API_REHLDS_NOT_FOUND;
		return false;
	}

	int returnCode = 0;
	g_rehldsApi = reinterpret_cast<IRehldsApi*>(
		factory(VREHLDS_HLDS_API_VERSION, &returnCode)
	);
	if (!g_rehldsApi)
	{
		g_apiStatus = API_REHLDS_NOT_FOUND;
		return false;
	}

	if (g_rehldsApi->GetMajorVersion() != REHLDS_API_VERSION_MAJOR ||
		g_rehldsApi->GetMinorVersion() < REHLDS_API_VERSION_MINOR)
	{
		g_apiStatus = API_REHLDS_VERSION_MISMATCH;
		return false;
	}

	const RehldsFuncs_t* rehldsFuncs = g_rehldsApi->GetFuncs();
	if (!rehldsFuncs || !rehldsFuncs->GetPluginApi)
	{
		g_apiStatus = API_REHLDS_NOT_FOUND;
		return false;
	}

	g_reunionApi = static_cast<IReunionApi*>(rehldsFuncs->GetPluginApi("reunion"));
	if (!g_reunionApi)
	{
		g_apiStatus = API_REUNION_NOT_FOUND;
		return false;
	}

	if (g_reunionApi->version_major != REUNION_API_VERSION_MAJOR ||
		g_reunionApi->version_minor < REUNION_API_VERSION_MINOR)
	{
		g_apiStatus = API_REUNION_VERSION_MISMATCH;
		g_reunionApi = nullptr;
		return false;
	}

	g_apiStatus = API_READY;
	return true;
}

cell AMX_NATIVE_CALL NativeIsAvailable(AMX*, cell*)
{
	return ResolveApis(true) ? 1 : 0;
}

cell AMX_NATIVE_CALL NativeGetStatus(AMX* amx, cell* params)
{
	ResolveApis(true);
	return g_amxxApi.SetAmxString(amx, params[1], ApiStatusText(g_apiStatus), params[2]);
}

cell AMX_NATIVE_CALL NativeGetAuthType(AMX* amx, cell* params)
{
	if (!ResolveApis(true))
		return 0;

	const int player = static_cast<int>(params[1]);
	if (player < 1 || player > 32 || !g_amxxApi.IsPlayerInGame(player))
	{
		g_amxxApi.LogError(amx, AMX_ERR_NATIVE, "Invalid or disconnected player index %d", player);
		return 0;
	}

	return static_cast<cell>(g_reunionApi->GetClientAuthtype(player - 1));
}

cell AMX_NATIVE_CALL NativeGetLongId(AMX* amx, cell* params)
{
	if (!ResolveApis(true))
		return 0;

	const int player = static_cast<int>(params[1]);
	if (player < 1 || player > 32 || !g_amxxApi.IsPlayerInGame(player) ||
		!g_amxxApi.IsPlayerAuthorized(player))
	{
		g_amxxApi.LogError(amx, AMX_ERR_NATIVE, "Player %d is not connected and authorized", player);
		return 0;
	}

	unsigned char longId[kLongAuthIdBytes] = {};
	g_reunionApi->GetLongAuthId(player - 1, longId);

	static constexpr char kHex[] = "0123456789abcdef";
	char output[kLongAuthIdHexLength + 1] = {};
	for (int i = 0; i < kLongAuthIdBytes; ++i)
	{
		output[i * 2] = kHex[(longId[i] >> 4) & 0x0F];
		output[i * 2 + 1] = kHex[longId[i] & 0x0F];
	}

	const int copied = g_amxxApi.SetAmxString(amx, params[2], output, params[3]);
	std::memset(longId, 0, sizeof(longId));
	std::memset(output, 0, sizeof(output));
	return copied;
}

AMX_NATIVE_INFO g_natives[] =
{
	{ "VEXID_IsAvailable", NativeIsAvailable },
	{ "VEXID_GetStatus", NativeGetStatus },
	{ "VEXID_GetAuthType", NativeGetAuthType },
	{ "VEXID_GetLongId", NativeGetLongId },
	{ nullptr, nullptr }
};

struct FunctionRequest
{
	const char* name;
	size_t offset;
};

#define REQUEST_AMXX_FUNCTION(name) { #name, offsetof(amxxapi_t, name) }

FunctionRequest g_functionRequests[] =
{
	REQUEST_AMXX_FUNCTION(AddNatives),
	REQUEST_AMXX_FUNCTION(SetAmxString),
	REQUEST_AMXX_FUNCTION(Log),
	REQUEST_AMXX_FUNCTION(LogError),
	REQUEST_AMXX_FUNCTION(IsPlayerInGame),
	REQUEST_AMXX_FUNCTION(IsPlayerAuthorized)
};

amxx_module_info_s g_moduleInfo =
{
	"VEX Identity",
	"PawNod' & OpenAI",
	"0.1.0",
	0,
	"VEXID",
	"vexid",
	"vexid"
};
}

C_DLLEXPORT int AMXX_Query(int* interfaceVersion, amxx_module_info_s* moduleInfo)
{
	if (!interfaceVersion || !moduleInfo)
		return AMXX_PARAM;

	if (*interfaceVersion != AMXX_INTERFACE_VERSION)
	{
		*interfaceVersion = AMXX_INTERFACE_VERSION;
		return AMXX_IFVERS;
	}

	std::memcpy(moduleInfo, &g_moduleInfo, sizeof(g_moduleInfo));
	return AMXX_OK;
}

C_DLLEXPORT int AMXX_CheckGame(const char*)
{
	return AMXX_GAME_OK;
}

C_DLLEXPORT int AMXX_Attach(PFN_REQ_FNPTR requestFunction)
{
	if (!requestFunction)
		return AMXX_PARAM;

	for (const FunctionRequest& request : g_functionRequests)
	{
		void* address = requestFunction(request.name);
		if (!address)
			return AMXX_FUNC_NOT_PRESENT;

		*reinterpret_cast<void**>(reinterpret_cast<unsigned char*>(&g_amxxApi) + request.offset) = address;
	}

	g_amxxApi.AddNatives(g_natives);
	ResolveApis(true);
	g_amxxApi.Log("[VEXID] module attached; Reunion identity API status: %s", ApiStatusText(g_apiStatus));
	return AMXX_OK;
}

C_DLLEXPORT int AMXX_Detach()
{
	g_reunionApi = nullptr;
	g_rehldsApi = nullptr;
	g_apiStatus = API_UNINITIALIZED;
	return AMXX_OK;
}

C_DLLEXPORT int AMXX_PluginsLoaded()
{
	ResolveApis(true);
	return AMXX_OK;
}

C_DLLEXPORT void AMXX_PluginsUnloading()
{
}

C_DLLEXPORT void AMXX_PluginsUnloaded()
{
}

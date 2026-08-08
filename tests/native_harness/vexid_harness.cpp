#include <cstdarg>
#include <cstdio>
#include <cstring>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#else
#include <dlfcn.h>
#endif

#include "platform.h"
#include <extdll.h>
#include <eiface.h>
#include "amxxmodule.h"

namespace
{
const AMX_NATIVE_INFO* g_natives = nullptr;
constexpr int kMockRevEmuAuthType = 4;

#ifdef _WIN32
using ModuleHandle = HMODULE;

ModuleHandle LoadModule(const char* path, bool)
{
	return LoadLibraryA(path);
}

void* GetSymbol(ModuleHandle module, const char* name)
{
	return reinterpret_cast<void*>(GetProcAddress(module, name));
}

void UnloadModule(ModuleHandle module)
{
	FreeLibrary(module);
}
#else
using ModuleHandle = void*;

ModuleHandle LoadModule(const char* path, bool global)
{
	return dlopen(path, RTLD_NOW | (global ? RTLD_GLOBAL : RTLD_LOCAL));
}

void* GetSymbol(ModuleHandle module, const char* name)
{
	return dlsym(module, name);
}

void UnloadModule(ModuleHandle module)
{
	dlclose(module);
}
#endif

int AddNatives(const AMX_NATIVE_INFO* natives)
{
	g_natives = natives;
	return 1;
}

int SetAmxString(AMX*, cell address, const char* source, int maxLength)
{
	cell* output = reinterpret_cast<cell*>(address);
	int copied = 0;
	while (source[copied] && copied < maxLength)
	{
		output[copied] = static_cast<unsigned char>(source[copied]);
		++copied;
	}
	output[copied] = 0;
	return copied;
}

void Log(const char* format, ...)
{
	va_list args;
	va_start(args, format);
	std::vprintf(format, args);
	std::printf("\n");
	va_end(args);
}

void LogError(AMX*, int, const char* format, ...)
{
	va_list args;
	va_start(args, format);
	std::vfprintf(stderr, format, args);
	std::fprintf(stderr, "\n");
	va_end(args);
}

int IsPlayerInGame(int id) { return id == 1; }
int IsPlayerAuthorized(int id) { return id == 1; }

void* RequestFunction(const char* name)
{
	if (std::strcmp(name, "AddNatives") == 0) return reinterpret_cast<void*>(AddNatives);
	if (std::strcmp(name, "SetAmxString") == 0) return reinterpret_cast<void*>(SetAmxString);
	if (std::strcmp(name, "Log") == 0) return reinterpret_cast<void*>(Log);
	if (std::strcmp(name, "LogError") == 0) return reinterpret_cast<void*>(LogError);
	if (std::strcmp(name, "IsPlayerInGame") == 0) return reinterpret_cast<void*>(IsPlayerInGame);
	if (std::strcmp(name, "IsPlayerAuthorized") == 0) return reinterpret_cast<void*>(IsPlayerAuthorized);
	return nullptr;
}

AMX_NATIVE FindNative(const char* name)
{
	if (!g_natives)
		return nullptr;
	for (const AMX_NATIVE_INFO* native = g_natives; native->name; ++native)
	{
		if (std::strcmp(native->name, name) == 0)
			return native->func;
	}
	return nullptr;
}

void CellsToString(const cell* input, char* output, size_t outputSize)
{
	size_t i = 0;
	while (i + 1 < outputSize && input[i])
	{
		output[i] = static_cast<char>(input[i]);
		++i;
	}
	output[i] = '\0';
}

int Fail(const char* message)
{
	std::fprintf(stderr, "FAIL: %s\n", message);
	return 1;
}
}

int main(int argc, char** argv)
{
	if (argc != 3)
		return Fail("usage: vexid_harness <mock swds.dll> <vexid_amxx.dll>");

	ModuleHandle engine = LoadModule(argv[1], true);
	if (!engine)
		return Fail("mock swds.dll could not be loaded");

	ModuleHandle module = LoadModule(argv[2], false);
	if (!module)
		return Fail("vexid_amxx.dll could not be loaded");

	using QueryFn = int (*)(int*, amxx_module_info_s*);
	using AttachFn = int (*)(PFN_REQ_FNPTR);
	auto query = reinterpret_cast<QueryFn>(GetSymbol(module, "AMXX_Query"));
	auto attach = reinterpret_cast<AttachFn>(GetSymbol(module, "AMXX_Attach"));
	if (!query || !attach)
		return Fail("required AMXX exports are missing");

	int interfaceVersion = AMXX_INTERFACE_VERSION;
	amxx_module_info_s info = {};
	if (query(&interfaceVersion, &info) != AMXX_OK || std::strcmp(info.library, "vexid") != 0)
		return Fail("AMXX_Query metadata is invalid");
	if (attach(RequestFunction) != AMXX_OK)
		return Fail("AMXX_Attach failed");

	AMX_NATIVE isAvailable = FindNative("VEXID_IsAvailable");
	AMX_NATIVE getStatus = FindNative("VEXID_GetStatus");
	AMX_NATIVE getAuthType = FindNative("VEXID_GetAuthType");
	AMX_NATIVE getLongId = FindNative("VEXID_GetLongId");
	if (!isAvailable || !getStatus || !getAuthType || !getLongId)
		return Fail("one or more Pawn natives are missing");

	cell simpleParams[2] = { sizeof(cell), 0 };
	if (isAvailable(nullptr, simpleParams) != 1)
		return Fail("VEXID_IsAvailable did not return true");

	cell authParams[2] = { sizeof(cell), 1 };
	if (getAuthType(nullptr, authParams) != kMockRevEmuAuthType)
		return Fail("VEXID_GetAuthType returned an unexpected value");

	cell statusCells[64] = {};
	cell statusParams[3] = {
		2 * sizeof(cell),
		reinterpret_cast<cell>(statusCells),
		63
	};
	if (getStatus(nullptr, statusParams) != 5)
		return Fail("VEXID_GetStatus returned an unexpected length");
	char status[64] = {};
	CellsToString(statusCells, status, sizeof(status));
	if (std::strcmp(status, "ready") != 0)
		return Fail("VEXID_GetStatus did not return ready");

	cell idCells[33] = {};
	cell longIdParams[4] = {
		3 * sizeof(cell),
		1,
		reinterpret_cast<cell>(idCells),
		32
	};
	if (getLongId(nullptr, longIdParams) != 32)
		return Fail("VEXID_GetLongId did not return 32 chars");
	char longId[33] = {};
	CellsToString(idCells, longId, sizeof(longId));
	if (std::strcmp(longId, "00017f80ff102030405060700809aa55") != 0)
		return Fail("binary LongAuthId was not converted to exact hex");

	std::printf("PASS: API ready, auth type %d, LongAuthId %s\n", kMockRevEmuAuthType, longId);
	UnloadModule(module);
	UnloadModule(engine);
	return 0;
}

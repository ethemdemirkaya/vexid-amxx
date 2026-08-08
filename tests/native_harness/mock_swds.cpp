#include <cstring>
#include <cstdio>

#include "platform.h"
#include <extdll.h>
#include <eiface.h>
#include <rehlds_api.h>
#include <reunion_api.h>

namespace
{
class MockReunionApi final : public IReunionApi
{
public:
	MockReunionApi()
	{
		version_major = REUNION_API_VERSION_MAJOR;
		version_minor = REUNION_API_VERSION_MINOR;
	}

	int GetClientProtocol(int) override { return 48; }
	dp_authkind_e GetClientAuthtype(int) override { return DP_AUTH_REVEMU; }
	size_t GetClientAuthdata(int, void*, int) override { return 0; }
	const char* GetClientAuthdataString(int, char*, int) override { return ""; }
	int GetMajorVersion() override { return version_major; }
	int GetMinorVersion() override { return version_minor; }

	void GetLongAuthId(int, unsigned char (&authId)[LONG_AUTHID_LEN]) override
	{
		const unsigned char expected[LONG_AUTHID_LEN] = {
			0x00, 0x01, 0x7f, 0x80, 0xff, 0x10, 0x20, 0x30,
			0x40, 0x50, 0x60, 0x70, 0x08, 0x09, 0xaa, 0x55
		};
		std::memcpy(authId, expected, sizeof(expected));
	}

	reu_authkey_kind GetAuthKeyKind(int) override { return REU_AK_HDDSN; }
	void SetConnectTime(int, double) override {}
	USERID_t* GetSerializedId(int) const override { return nullptr; }
	USERID_t* GetStorageId(int) const override { return nullptr; }
	uint64 GetDisplaySteamId(int) const override { return 0; }
};

MockReunionApi g_reunionApi;
RehldsFuncs_t g_rehldsFuncs = {};

void* GetPluginApi(const char* name)
{
	return name && std::strcmp(name, "reunion") == 0 ? &g_reunionApi : nullptr;
}

class MockRehldsApi final : public IRehldsApi
{
public:
	int GetMajorVersion() override { return REHLDS_API_VERSION_MAJOR; }
	int GetMinorVersion() override { return REHLDS_API_VERSION_MINOR; }
	const RehldsFuncs_t* GetFuncs() override
	{
		g_rehldsFuncs.GetPluginApi = GetPluginApi;
		return &g_rehldsFuncs;
	}
	IRehldsHookchains* GetHookchains() override { return nullptr; }
	IRehldsServerStatic* GetServerStatic() override { return nullptr; }
	IRehldsServerData* GetServerData() override { return nullptr; }
	IRehldsFlightRecorder* GetFlightRecorder() override { return nullptr; }
	IMessageManager* GetMessageManager() override { return nullptr; }
};

MockRehldsApi g_rehldsApi;
}

#ifdef _WIN32
#define MOCK_EXPORT extern "C" __declspec(dllexport)
#else
#define MOCK_EXPORT extern "C" __attribute__((visibility("default")))
#endif

MOCK_EXPORT IBaseInterface* CreateInterface(const char* name, int* returnCode)
{
	if (name && std::strcmp(name, VREHLDS_HLDS_API_VERSION) == 0)
	{
		if (returnCode)
			*returnCode = 0;
		return reinterpret_cast<IBaseInterface*>(&g_rehldsApi);
	}

	if (returnCode)
		*returnCode = 1;
	return nullptr;
}

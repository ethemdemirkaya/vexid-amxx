#include <amxmodx>
#include <vexid>

public plugin_init()
{
	register_plugin("VEX Identity Test", "0.1.0", "PawNod'");
	register_concmd("vexid_test", "@CommandVexIdTest", ADMIN_RCON, "<slot>");
}

public @CommandVexIdTest(id, level, cid)
{
	if(id && !(get_user_flags(id) & level)) {
		console_print(id, "[VEXID] Bu komut icin yetkin yok.");
		return PLUGIN_HANDLED;
	}

	new szStatus[96];
	VEXID_GetStatus(szStatus, charsmax(szStatus));
	console_print(id, "[VEXID] API: %s", szStatus);

	if(!VEXID_IsAvailable()) {
		return PLUGIN_HANDLED;
	}

	new szSlot[8];
	read_argv(1, szSlot, charsmax(szSlot));
	new iPlayer = str_to_num(szSlot);
	if(iPlayer < 1 || iPlayer > MaxClients || !is_user_connected(iPlayer)) {
		console_print(id, "[VEXID] Kullanim: vexid_test <bagli oyuncu slotu>");
		return PLUGIN_HANDLED;
	}

	new szLongId[33];
	new iLength = VEXID_GetLongId(iPlayer, szLongId, charsmax(szLongId));
	if(iLength != 32) {
		console_print(id, "[VEXID] LongAuthId alinamadi. Donen uzunluk: %d", iLength);
		return PLUGIN_HANDLED;
	}

	new szName[32];
	get_user_name(iPlayer, szName, charsmax(szName));
	console_print(id, "[VEXID] Oyuncu: %s | AuthType: %d", szName, VEXID_GetAuthType(iPlayer));
	console_print(id, "[VEXID] LongAuthId: %s", szLongId);
	return PLUGIN_HANDLED;
}

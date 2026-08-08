#if defined _vexid_included
	#endinput
#endif
#define _vexid_included

#if AMXX_VERSION_NUM >= 175
	#pragma reqlib vexid
	#if !defined AMXMODX_NOAUTOLOAD
		#pragma loadlib vexid
	#endif
#else
	#pragma library vexid
#endif

/**
 * Returns whether ReHLDS and Reunion identity API 1.4 are available.
 */
native bool:VEXID_IsAvailable();

/**
 * Copies a human-readable module/API status into output.
 *
 * @return Number of characters copied.
 */
native VEXID_GetStatus(output[], maxlen);

/**
 * Returns Reunion's client authentication type.
 * Values match client_auth_type from reapi_reunion.inc.
 */
native VEXID_GetAuthType(const index);

/**
 * Returns Reunion's binary-safe 128-bit LongAuthId as 32 lowercase hex chars.
 * Call only for a connected and authorized player.
 *
 * @return Number of characters copied (32 on success), 0 when unavailable.
 */
native VEXID_GetLongId(const index, output[], maxlen);

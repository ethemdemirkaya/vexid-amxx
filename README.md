# VEX Identity AMXX Module

[![build](https://github.com/ethemdemirkaya/vexid-amxx/actions/workflows/build.yml/badge.svg)](https://github.com/ethemdemirkaya/vexid-amxx/actions/workflows/build.yml)

`vexid_amxx` exposes Reunion API 1.4 `GetLongAuthId()` to Pawn as a
binary-safe, 32-character hexadecimal identity.

This is not a universal or unchangeable hardware ID. For supported non-Steam
clients it is derived from Reunion's complete raw authentication material
(often HDD serial or VolumeID). For genuine Steam clients it is derived from
the authentication type and Steam account ID.

## Pawn API

```pawn
new longId[33];
if (VEXID_GetLongId(id, longId, charsmax(longId)) == 32) {
    // Hash with a server-side secret before persistence.
}
```

## Requirements

- 32-bit ReHLDS with API 3.15 or newer
- Reunion with API 1.4 or newer
- AMX Mod X 1.9/1.10

## Windows build

Build `msvc/vexid.vcxproj` as `Release|Win32`. If the ReAPI SDK is elsewhere,
pass `VexReapiRoot` to MSBuild. The output is `bin/windows/vexid_amxx.dll`.

## Linux build

Install a 32-bit-capable GCC/G++ toolchain and run `make`. Override
`REAPI_ROOT=/path/to/reapi` when the SDK is not at the default relative path.
The output is `bin/linux/vexid_amxx_i386.so`.

## Installation and smoke test

1. Copy `vexid_amxx.dll` to `addons/amxmodx/modules/`.
2. Copy `include/vexid.inc` to the scripting include directory.
3. Compile `tests/vexid_test.sma` and install the resulting plugin.
4. Start the server and run `vexid_test <slot>` from the server console.

The test is successful when the API status is `ready` and a 32-character
LongAuthId is printed. Do not publish or log these identifiers in production;
VEX Ban should hash them with `VEX_Secret.ini` before persistence.

The `tests/native_harness` projects provide an offline mock ReHLDS/Reunion
test. It deliberately returns a binary identifier containing null bytes and
verifies that the module produces the exact 32-character hexadecimal value.

Turkish installation instructions are available in `docs/INSTALL_TR.md`.

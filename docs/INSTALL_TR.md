# VEX Identity kurulumu

## Gereksinimler

- 32-bit ReHLDS, API 3.15 veya daha yeni
- Reunion, API 1.4 veya daha yeni
- AMX Mod X 1.9 ya da 1.10

## Windows

`vexid_amxx.dll` dosyasını `addons/amxmodx/modules/` klasörüne kopyalayın.

## Linux

`vexid_amxx_i386.so` dosyasını `addons/amxmodx/modules/` klasörüne kopyalayın.
Dosyanın okunabilir olduğundan emin olun.

## Test

1. Paketteki `vexid_test.amxx` dosyasını `addons/amxmodx/plugins/` içine koyun.
2. `addons/amxmodx/configs/plugins.ini` dosyasına `vexid_test.amxx` ekleyin.
3. Oyuncu bağlandıktan sonra sunucu konsolunda `vexid_test <slot>` çalıştırın.

Başarılı sonuçta API durumu `ready` ve 32 karakterlik LongAuthId görünür.
Üretimde bu kimliği açık şekilde loglamayın; sunucuya özel gizli değerle hashleyin.

Bu değer bütün istemcilerde fiziksel donanım seri numarası değildir. Non-Steam
istemcilerde Reunion'ın tam auth verisinden, gerçek Steam istemcisinde Steam
hesap kimliğinden türetilir.

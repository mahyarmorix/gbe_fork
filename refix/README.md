# ReFix Photon emulator (Jong)

`photon/` and `refix_steam_flat.*` come from ReFix by Coronitaa, https://github.com/Coronitaa/ReFix
(commit c2aaa6c), licensed CC BY-NC-SA 4.0 (`LICENSE`). Jong's changes: `pn_dns.cpp` hooks with Goldberg's
Detours instead of MinHook, `pn_p2p.cpp` turns the emulator on for Photon games by default (as ReFix LAN does),
and `jong_refix.*` / `steam_p2p_hook.h` connect it to Goldberg.

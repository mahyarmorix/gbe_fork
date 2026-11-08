#pragma once
// Jong: ReFix's Photon emulator (refix/photon) inside Goldberg's experimental steam_api, Windows only.
// Photon games reach a local Photon server; players' traffic goes over Goldberg's SteamNetworkingMessages
// (Jong's LAN), so no Photon Cloud app, AppId or CCU limit is needed. ReFix.ini [Photon] Backend=OfficialPhoton
// next to the game's exe turns it off for that game.
void jong_photon_init();     // after SteamAPI_Init
void jong_photon_service();  // on the game's thread, from RunCallbacks

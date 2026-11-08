#pragma once
// Jong: ReFix's legacy UDP-over-SDR layer is not part of Goldberg; the Photon emulator's calls into it do nothing.
#include <cstdint>
#include <vector>

namespace SteamP2PHook {
    inline void ExcludeSocket(uintptr_t) {}
    inline void IncludeSocket(uintptr_t) {}
    inline std::vector<uint64_t> GetRegisteredPeers() { return {}; }
}

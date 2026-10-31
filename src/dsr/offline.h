#pragma once

#include <cstdint>

// BlockOnline: the game's own Winsock imports are redirected so it cannot find or reach FromSoftware's
// servers and plays offline. Returns false if the imports could not all be redirected.
bool offline_install();

// Number of lookups, connections and packets blocked so far.
uint64_t offline_blocked_count();

#pragma once

/**
 * Allocation shim.
 * On the device, large buffers belong in PSRAM, not the ~500KB internal
 * heap. On the host test build there is no PSRAM, so it is plain malloc.
 * This is the only host/device #ifdef in the codebase - keep it that way.
 */

#include <cstddef>
#include <cstdlib>

#ifdef POCKETCAN_HOST_TEST

inline void* pc_alloc(size_t bytes) { return std::malloc(bytes); }
inline void  pc_free(void* p)       { std::free(p); }

#else

#include <esp_heap_caps.h>

inline void* pc_alloc(size_t bytes) {
    void* p = heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    return p ? p : std::malloc(bytes);   // fall back to internal heap
}
inline void  pc_free(void* p) { heap_caps_free(p); }

#endif

# PocketCAN Phase A (Foundation) Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Turn PocketCAN from a well-structured scaffold whose core modules are never called into a device that captures, filters, transmits, and emulates CAN frames on a real bus, with host-run unit tests and a reproducible build.

**Architecture:** Introduce a narrow `ICANBus` abstract base class so core logic compiles and tests on a host. `Esp32CanBus` implements it against ESP-IDF TWAI and is the only thing the firmware ever uses. A `CanService` orchestrator owns the bus and the four core modules, exposes one `update(now)` to the main loop, and reduces `main.cpp` to a composition root. Time enters the system at exactly one place — `CanService::update()` calls `millis()` once and passes the value down — so wrap-around bugs become unit-testable.

**Tech Stack:** PlatformIO (`pioarduino` platform fork), Arduino framework, C++17, ESP32-P4 (M5Stack Tab5), ESP-IDF TWAI driver, LVGL 8.3, M5GFX/M5Unified, Unity test framework via `platform = native`.

**Spec:** `docs/superpowers/specs/2026-09-16-pocketcan-foundation-design.md`

## Global Constraints

- **The device is standalone.** `ICANBus` is a compile-time C++ abstraction only. The shipped firmware talks directly to the TWAI peripheral and never depends on a host computer at runtime. The native test binary is a separate artifact that is never flashed.
- **LVGL stays at 8.3** (`lvgl/lvgl@^8.3.11`). `include/lv_conf.h` is the single source of truth. Never add an LVGL v9 config.
- **C++17**, `#pragma once` header guards, classes `PascalCase`, functions `snake_case()`, constants `UPPER_SNAKE_CASE`.
- **Core layer is platform-free.** After Task 8, nothing under `src/core/` or `src/utils/` may include `<Arduino.h>`, `driver/twai.h`, `M5GFX.h`, or `lvgl.h`, or call `millis()` or `Serial`. Logging lives in `src/hal/` and `src/services/`.
- **Every task ends with a commit.** Attribution line on every commit message: `Co-Authored-By: Claude Opus 5 (1M context) <noreply@anthropic.com>`
- **Device build command:** `pio run -e esp32p4_pioarduino`
- **Host test command:** `pio test -e native`
- **Hardware gates are mandatory.** Tasks 14 and 20 require a physical M5Stack Tab5 on a live CAN bus. Do not mark them complete from a code review.

## Deviations from the spec (approved during planning)

Three places where this plan differs from the spec, each for a concrete reason:

1. **`Esp32CanBus` lands at the end of A2, not in A3** (spec §9). Once `CANTransmitter` and `DeviceEmulator` take an `ICANBus&`, the device build cannot compile against the old static `CANHAL`. Introducing a throwaway adapter to defer the rename by one phase would be pure waste.
2. **`CanService` does not own a separate `RingBuffer history_`** (spec §4.2). `CANSniffer` already owns `message_buffer{CAN_RX_BUFFER_SIZE}` plus the message-rate statistics that go with it. `CanService::history()` delegates to it, so the public interface in the spec is preserved and a second 24 KB buffer is not allocated.
3. **The bus-off *policy* lives in `CanService`, not `Esp32CanBus`** (spec §6). The retry count, settle delay, and `FAILED` terminal state are exactly the logic worth testing, and `FakeCanBus` can drive them. `Esp32CanBus::recover()` keeps the *mechanism* (`twai_initiate_recovery`). Mechanism on the device, policy under test.

## File structure

| File | Responsibility | Task |
|---|---|---|
| `.gitignore`, `LICENSE` | repo hygiene | 1 |
| `partitions_pocketcan.csv` | 3 MB app partition for 16 MB flash | 2 |
| `src/core/can_types.h` | `CANMessage`, `CANStats`, `CANBusState` — platform-free | 3 |
| `src/utils/psram_alloc.h` | one allocation shim; the only place a host/device `#ifdef` appears | 3 |
| `src/core/can_timing.h` | `CANBaudRate -> CanTimingSpec`, pure and testable | 4 |
| `src/core/i_can_bus.h` | the seam: transmit/receive/stats/error/recover | 6 |
| `src/hal/esp32_can_bus.{h,cpp}` | TWAI implementation of `ICANBus` (was `can_hal`) | 9 |
| `src/services/can_stats.{h,cpp}` | rx/tx counters and real bus-load calculation | 10 |
| `src/services/can_service.{h,cpp}` | composition root: owns bus + 4 core modules, one `update(now)` | 11, 12 |
| `test/fakes/fake_can_bus.h` | scripted `ICANBus` for host tests | 6 |
| `test/test_*/` | one Unity suite per area | 3–12 |

---

# Phase A1 — Build hygiene

## Task 1: Remove the config collision and add repo hygiene

The repo root holds a stock **LVGL v9.4.0** `lv_conf.h` (1477 lines) while the working config is `include/lv_conf.h` (LVGL 8.3, 241 lines). Both `#define LV_CONF_H`, so which one wins depends on include order. There is also no `.gitignore` (PlatformIO's `.pio/` is untracked only by luck) and no `LICENSE` file despite the README's MIT badge.

**Files:**
- Delete: `lv_conf.h` (repo root — the v9.4.0 one; **not** `include/lv_conf.h`)
- Create: `.gitignore`, `LICENSE`
- Modify: `platformio.ini:28-30`

**Interfaces:**
- Consumes: nothing.
- Produces: a build where `include/lv_conf.h` is unambiguously the LVGL config.

- [ ] **Step 1: Confirm which file is which before deleting anything**

```bash
head -5 lv_conf.h
head -5 include/lv_conf.h
grep -c "" lv_conf.h include/lv_conf.h
```

Expected: the root file mentions `v9.4.0` and reports 1477 lines; `include/lv_conf.h` reports 241 lines. If this does not match, **stop** — do not delete.

- [ ] **Step 2: Delete the stray v9 config**

```bash
git rm lv_conf.h
```

- [ ] **Step 3: Remove the dead build_src_filter**

The `lvgl_example/` directory does not exist. In `platformio.ini`, delete these three lines:

```ini
build_src_filter =
    +<*>
    -<lvgl_example/>
```

- [ ] **Step 4: Create `.gitignore`**

```gitignore
.pio/
.pioenvs/
.piolibdeps/
.vscode/
.clangd/
compile_commands.json
*.o
*.a
*.elf
*.bin
.DS_Store
```

- [ ] **Step 5: Create `LICENSE`**

Write the standard MIT License text, with `Copyright (c) 2026 Jakub Charvat` as the copyright line. The README already advertises MIT; this makes it true.

- [ ] **Step 6: Verify the build still compiles**

Run: `pio run -e esp32p4_pioarduino`
Expected: compiles. Record the reported `Flash:` and `RAM:` percentages — Task 2 compares against them.

- [ ] **Step 7: Commit**

```bash
git add -A
git commit -m "build: delete stray LVGL v9 config, add .gitignore and LICENSE

The repo root held a stock LVGL v9.4.0 lv_conf.h shadowing the working
v8.3 config in include/. Both define LV_CONF_H, so the winner depended
on include order.

Co-Authored-By: Claude Opus 5 (1M context) <noreply@anthropic.com>"
```

---

## Task 2: Fit the application in flash

`include/lv_conf.h` enables **every** Montserrat font from 12 to 48 (lines 88–108), costing roughly 1 MB of flash. `platformio.ini` uses `default.csv`, which gives the application ~1.3 MB. The UI only ever uses sizes 20, 24, 32, and 48 (`ui_manager.cpp:29`, `screen_main.cpp` tile labels).

**Files:**
- Modify: `include/lv_conf.h:88-108`
- Create: `partitions_pocketcan.csv`
- Modify: `platformio.ini`

**Interfaces:**
- Consumes: Task 1's clean build.
- Produces: a debug and a release environment; `pio run -e esp32p4_pioarduino` remains the debug default.

- [ ] **Step 1: Confirm which fonts the UI actually references**

```bash
grep -rho "lv_font_montserrat_[0-9]*" src/ | sort -u
```

Expected: `lv_font_montserrat_20`, `_24`, `_32`, `_48`. If any other size appears, keep that size enabled too in Step 2.

- [ ] **Step 2: Disable the unused fonts**

In `include/lv_conf.h`, set every `LV_FONT_MONTSERRAT_*` to `0` **except** 20, 24, 32, and 48, which stay `1`. Leave `LV_FONT_DEFAULT` pointing at a size that is still enabled — if it references a disabled size, change it to `&lv_font_montserrat_24`.

- [ ] **Step 3: Create the partition table**

`partitions_pocketcan.csv` — 16 MB flash, 3 MB per app slot:

```csv
# Name,     Type, SubType, Offset,   Size,     Flags
nvs,        data, nvs,     0x9000,   0x5000,
otadata,    data, ota,     0xE000,   0x2000,
app0,       app,  ota_0,   0x10000,  0x300000,
app1,       app,  ota_1,   0x310000, 0x300000,
spiffs,     data, spiffs,  0x610000, 0x9E0000,
coredump,   data, coredump,0xFF0000, 0x10000,
```

- [ ] **Step 4: Restructure `platformio.ini` into a shared base plus two environments**

```ini
; PocketCAN - Professional CAN Bus Analyzer
; Platform: M5Stack Tab5 (ESP32-P4)

[pocketcan_base]
platform = https://github.com/pioarduino/platform-espressif32.git#54.03.20
framework = arduino
board = esp32-p4-evboard
board_build.mcu = esp32p4
board_build.flash_mode = qio
board_build.flash_size = 16MB
board_upload.flash_size = 16MB
board_build.partitions = partitions_pocketcan.csv

upload_speed = 1500000
monitor_speed = 115200

lib_deps =
    https://github.com/M5Stack/M5Unified.git
    https://github.com/M5Stack/M5GFX.git
    lvgl/lvgl@^8.3.11

[env:esp32p4_pioarduino]
extends = pocketcan_base
build_type = debug
build_flags =
    -DBOARD_HAS_PSRAM
    -DCORE_DEBUG_LEVEL=5
    -DARDUINO_USB_CDC_ON_BOOT=1
    -DARDUINO_USB_MODE=1
    -I include
    -std=c++17

[env:esp32p4_release]
extends = pocketcan_base
build_type = release
build_flags =
    -DBOARD_HAS_PSRAM
    -DCORE_DEBUG_LEVEL=1
    -DARDUINO_USB_CDC_ON_BOOT=1
    -DARDUINO_USB_MODE=1
    -I include
    -std=c++17
    -Os
```

- [ ] **Step 5: Build both environments and compare**

Run: `pio run -e esp32p4_pioarduino && pio run -e esp32p4_release`
Expected: both compile. Flash usage is materially lower than the figure recorded in Task 1 Step 6 (the font trim alone should free several hundred KB), and the percentage is now computed against a 3 MB partition rather than 1.3 MB.

If the build fails with `undefined reference to lv_font_montserrat_NN`, a source file references a font you disabled. Re-enable that one size and note it.

- [ ] **Step 6: Commit**

```bash
git add -A
git commit -m "build: trim fonts to 20/24/32/48, add 3MB partition table

All Montserrat sizes 12-48 were enabled, costing ~1MB of flash, while
default.csv allowed only ~1.3MB for the application. Adds a release
environment alongside the debug default.

Co-Authored-By: Claude Opus 5 (1M context) <noreply@anthropic.com>"
```

**A1 GATE:** `pio run -e esp32p4_pioarduino` and `pio run -e esp32p4_release` both complete clean, and the application sits comfortably inside its 3 MB partition. Record both figures in the commit message or a comment before proceeding.

---

# Phase A2 — Test harness and core logic

## Task 3: Native test environment, platform-free types, RingBuffer tests

Core code currently reaches `CANMessage` through `src/hal/can_hal.h`, which includes `driver/twai.h` — so nothing in `src/core/` can compile on a host. Extracting the types is the precondition for every test in this plan.

**Files:**
- Create: `src/core/can_types.h`, `src/utils/psram_alloc.h`, `test/test_ring_buffer/test_ring_buffer.cpp`
- Modify: `src/hal/can_hal.h`, `src/services/state_manager.h`, `src/utils/ring_buffer.h`, `platformio.ini`

**Interfaces:**
- Consumes: Task 2's `platformio.ini` layout.
- Produces: `src/core/can_types.h` defining `struct CANMessage`, `struct CANStats`, `enum class CANBusState`; `src/utils/psram_alloc.h` defining `void* pc_alloc(size_t)` and `void pc_free(void*)`; a working `pio test -e native`.

- [ ] **Step 1: Create `src/core/can_types.h`**

```cpp
#pragma once

/**
 * Platform-free CAN types.
 * Deliberately includes nothing from ESP-IDF, Arduino, or LVGL so that
 * core logic compiles and tests on a host.
 */

#include <cstdint>

#include "../config/can_config.h"

// A single CAN 2.0A/B frame.
struct CANMessage {
    uint32_t id;                    // CAN identifier (11-bit or 29-bit)
    uint8_t data[8];                // Data bytes; bytes at or past dlc are zero
    uint8_t dlc;                    // Data length code (0-8)
    CANFrameType type;              // Standard or Extended
    uint32_t timestamp_ms;          // Capture time
    bool rtr;                       // Remote transmission request
};

// Rolling bus statistics.
struct CANStats {
    uint32_t rx_count;
    uint32_t tx_count;
    uint32_t error_count;           // tx_error_counter + rx_error_counter
    uint32_t bus_off_count;         // bus-off events, not error frames
    float bus_load_percent;         // estimated, see CanStatsCollector
};

// Lifecycle of the bus as the application sees it.
enum class CANBusState {
    STOPPED,
    RUNNING,
    BUS_OFF,        // controller has gone bus-off, recovery not yet attempted
    RECOVERING,     // recovery initiated, waiting for the controller to settle
    FAILED          // recovery gave up; needs user intervention
};
```

- [ ] **Step 2: Point the old headers at the new one**

In `src/hal/can_hal.h`, delete the `struct CANMessage {...}` and `struct CANStats {...}` definitions and add `#include "../core/can_types.h"` after the existing includes. Everything else in that file stays for now.

In `src/services/state_manager.h`, delete the `enum class CANBusState {...}` definition and add `#include "../core/can_types.h"`.

- [ ] **Step 3: Create `src/utils/psram_alloc.h`**

This is the only file in the project allowed to branch on host-vs-device.

```cpp
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
```

- [ ] **Step 4: Make RingBuffer allocate through the shim and correct its lies**

In `src/utils/ring_buffer.h`: add `#include "psram_alloc.h"`, replace the constructor body and destructor, and fix two incorrect doc comments.

```cpp
    RingBuffer(size_t capacity) : capacity(capacity), head(0), tail(0), count(0) {
        buffer = static_cast<T*>(pc_alloc(sizeof(T) * capacity));
    }

    ~RingBuffer() {
        pc_free(buffer);
    }
```

Change the class doc comment from `Thread-safe circular buffer implementation` to:

```cpp
/**
 * Ring Buffer (Circular Buffer)
 * NOT thread-safe. All access is from the main loop by design.
 * Storage comes from PSRAM on the device via pc_alloc().
 */
```

Change the `push()` doc comment from `@return true if successful, false if buffer full` to:

```cpp
    /**
     * Push item to buffer. When full, silently overwrites the oldest item.
     * @return always true - kept for call-site compatibility
     */
```

- [ ] **Step 5: Add the native test environment to `platformio.ini`**

Append:

```ini
[env:native]
platform = native
test_framework = unity
test_filter = test_*
build_flags =
    -std=c++17
    -D POCKETCAN_HOST_TEST
    -I src
    -I test
build_src_filter = +<core/> +<utils/>
```

`test_filter = test_*` keeps PlatformIO from treating `test/fakes/` (added in Task 6) as a test suite of its own.

- [ ] **Step 6: Write the failing RingBuffer tests**

`test/test_ring_buffer/test_ring_buffer.cpp`:

```cpp
#include <unity.h>

#include "utils/ring_buffer.h"

void setUp(void) {}
void tearDown(void) {}

// Guards: overwrite semantics at capacity.
void test_ring_buffer_wraps_and_reports_size(void) {
    RingBuffer<int> rb(3);

    rb.push(1);
    rb.push(2);
    rb.push(3);
    TEST_ASSERT_EQUAL_UINT32(3, rb.size());
    TEST_ASSERT_TRUE(rb.is_full());

    rb.push(4);   // overwrites the oldest (1)

    TEST_ASSERT_EQUAL_UINT32(3, rb.size());
    int oldest = 0;
    TEST_ASSERT_TRUE(rb.peek(0, oldest));
    TEST_ASSERT_EQUAL_INT(2, oldest);

    int newest = 0;
    TEST_ASSERT_TRUE(rb.peek(2, newest));
    TEST_ASSERT_EQUAL_INT(4, newest);
}

// Guards: push() always returns true; its doc comment claimed otherwise.
void test_ring_buffer_push_returns_documented_value(void) {
    RingBuffer<int> rb(2);
    TEST_ASSERT_TRUE(rb.push(1));
    TEST_ASSERT_TRUE(rb.push(2));
    TEST_ASSERT_TRUE(rb.push(3));   // full - still true, oldest discarded
    TEST_ASSERT_EQUAL_UINT32(2, rb.size());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_ring_buffer_wraps_and_reports_size);
    RUN_TEST(test_ring_buffer_push_returns_documented_value);
    return UNITY_END();
}
```

- [ ] **Step 7: Run the tests**

Run: `pio test -e native`
Expected: the suite builds and both tests PASS. (These two document existing correct behaviour rather than a defect — they exist so that the buffer's contract is pinned before `CanService` starts depending on it. If either fails, the RingBuffer has a second bug the audit missed; fix it before continuing.)

- [ ] **Step 8: Verify the device build is still green**

Run: `pio run -e esp32p4_pioarduino`
Expected: compiles. The type extraction must not have broken anything.

- [ ] **Step 9: Commit**

```bash
git add -A
git commit -m "test: add native test environment and platform-free CAN types

Extracts CANMessage, CANStats, and CANBusState out of the TWAI-dependent
HAL header so core logic can compile on a host. RingBuffer now allocates
from PSRAM on device via a single allocation shim.

Co-Authored-By: Claude Opus 5 (1M context) <noreply@anthropic.com>"
```

---

## Task 4: Baud-rate table (defects #9, #10)

`can_hal.cpp:26-27` maps `BAUD_20K` to `TWAI_TIMING_CONFIG_25KBITS()` — a 25% bit-rate error, which on a real bus means silently failing to sync at all. `BAUD_10K` is absent from the switch entirely and falls through to the 500K default at line 29. Neither is testable while the table returns `twai_timing_config_t`, an ESP-IDF type. Split the *mapping* (pure, testable) from the *register values* (ESP-IDF's, trusted).

**Files:**
- Create: `src/core/can_timing.h`, `test/test_can_timing/test_can_timing.cpp`

**Interfaces:**
- Consumes: `src/core/can_types.h`.
- Produces: `enum class CanTimingId`, `struct CanTimingSpec { CanTimingId id; uint32_t nominal_bps; }`, and `constexpr CanTimingSpec can_timing_for(CANBaudRate)`. Task 9's `Esp32CanBus` switches on `CanTimingId`.

- [ ] **Step 1: Write the failing test**

`test/test_can_timing/test_can_timing.cpp`:

```cpp
#include <unity.h>

#include "core/can_timing.h"

void setUp(void) {}
void tearDown(void) {}

static const CANBaudRate ALL_RATES[] = {
    CANBaudRate::BAUD_10K,  CANBaudRate::BAUD_20K,  CANBaudRate::BAUD_50K,
    CANBaudRate::BAUD_100K, CANBaudRate::BAUD_125K, CANBaudRate::BAUD_250K,
    CANBaudRate::BAUD_500K, CANBaudRate::BAUD_800K, CANBaudRate::BAUD_1M,
};
static const int RATE_COUNT = sizeof(ALL_RATES) / sizeof(ALL_RATES[0]);

// Guards defect #9 (20K mapped to 25K) and #10 (10K missing entirely).
// Every rate must map to a spec whose nominal bit rate equals the enum's
// own numeric value. The 20K bug fails the equality; the 10K bug fails it
// too, because the default branch returns 500000.
void test_can_timing_table_is_complete_and_distinct(void) {
    for (int i = 0; i < RATE_COUNT; i++) {
        CanTimingSpec spec = can_timing_for(ALL_RATES[i]);
        TEST_ASSERT_EQUAL_UINT32(static_cast<uint32_t>(ALL_RATES[i]), spec.nominal_bps);
    }

    // No two rates may share a timing id.
    for (int i = 0; i < RATE_COUNT; i++) {
        for (int j = i + 1; j < RATE_COUNT; j++) {
            TEST_ASSERT_NOT_EQUAL(
                static_cast<int>(can_timing_for(ALL_RATES[i]).id),
                static_cast<int>(can_timing_for(ALL_RATES[j]).id));
        }
    }
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_can_timing_table_is_complete_and_distinct);
    return UNITY_END();
}
```

- [ ] **Step 2: Run it to confirm it fails**

Run: `pio test -e native -f test_can_timing`
Expected: FAIL to compile — `core/can_timing.h: No such file or directory`.

- [ ] **Step 3: Write `src/core/can_timing.h`**

```cpp
#pragma once

/**
 * CAN bit timing selection.
 *
 * This header is deliberately free of ESP-IDF types so the mapping can be
 * unit-tested on a host. It names a timing preset; esp32_can_bus.cpp turns
 * the name into the ESP-IDF TWAI_TIMING_CONFIG_* register values. Keeping
 * the two apart is what makes a wrong mapping a test failure rather than a
 * bus that silently never syncs.
 */

#include <cstdint>

#include "can_types.h"

enum class CanTimingId : uint8_t {
    T_10K, T_20K, T_50K, T_100K, T_125K, T_250K, T_500K, T_800K, T_1M
};

struct CanTimingSpec {
    CanTimingId id;
    uint32_t    nominal_bps;
};

constexpr CanTimingSpec can_timing_for(CANBaudRate rate) {
    switch (rate) {
        case CANBaudRate::BAUD_10K:  return {CanTimingId::T_10K,     10000};
        case CANBaudRate::BAUD_20K:  return {CanTimingId::T_20K,     20000};
        case CANBaudRate::BAUD_50K:  return {CanTimingId::T_50K,     50000};
        case CANBaudRate::BAUD_100K: return {CanTimingId::T_100K,   100000};
        case CANBaudRate::BAUD_125K: return {CanTimingId::T_125K,   125000};
        case CANBaudRate::BAUD_250K: return {CanTimingId::T_250K,   250000};
        case CANBaudRate::BAUD_500K: return {CanTimingId::T_500K,   500000};
        case CANBaudRate::BAUD_800K: return {CanTimingId::T_800K,   800000};
        case CANBaudRate::BAUD_1M:   return {CanTimingId::T_1M,    1000000};
    }
    return {CanTimingId::T_500K, 500000};   // unreachable; enum is exhaustive
}
```

- [ ] **Step 4: Run the test to verify it passes**

Run: `pio test -e native -f test_can_timing`
Expected: PASS.

- [ ] **Step 5: Commit**

```bash
git add -A
git commit -m "fix(can): correct 20K baud mapping and add missing 10K

BAUD_20K returned TWAI_TIMING_CONFIG_25KBITS - a 25% bit rate error that
means total failure to sync on a real bus. BAUD_10K was absent from the
switch and silently fell through to the 500K default.

Splits the mapping into a pure, host-testable table.

Co-Authored-By: Claude Opus 5 (1M context) <noreply@anthropic.com>"
```

---

## Task 5: Filter allow-list fall-through (defect #3)

`can_filter.cpp:107-108` returns `true` when no rule matched, so enabling filtering with an accept rule passes **100% of traffic** — the exact inverse of an allow-list, and contradicted by the file's own comment at line 102.

**Files:**
- Create: `test/test_can_filter/test_can_filter.cpp`
- Modify: `src/core/can_filter.h:8`, `src/core/can_filter.cpp:1-18,62-109`

**Interfaces:**
- Consumes: `src/core/can_types.h`.
- Produces: `CANFilter::check_message()` with allow-list semantics unchanged in signature.

- [ ] **Step 1: Write the failing tests**

`test/test_can_filter/test_can_filter.cpp`:

```cpp
#include <unity.h>

#include "core/can_filter.h"

void setUp(void) {}
void tearDown(void) {}

static CANMessage make_msg(uint32_t id) {
    CANMessage m = {};
    m.id   = id;
    m.dlc  = 8;
    m.type = CANFrameType::STANDARD;
    return m;
}

static FilterRule make_rule(uint32_t id, bool accept) {
    FilterRule r = {};
    r.id      = id;
    r.mask    = 0x7FF;          // exact match on a standard ID
    r.enabled = true;
    r.accept  = accept;
    r.type    = CANFrameType::STANDARD;
    return r;
}

// Guards defect #3. An allow-list must reject everything it did not name.
void test_filter_accept_list_rejects_unmatched(void) {
    CANFilter f;
    f.init();
    f.set_enabled(true);
    f.add_rule(make_rule(0x123, true));

    TEST_ASSERT_TRUE(f.check_message(make_msg(0x123)));
    TEST_ASSERT_FALSE(f.check_message(make_msg(0x456)));
}

// The complementary direction: a reject-list passes everything it did not name.
void test_filter_reject_list_blocks_matched(void) {
    CANFilter f;
    f.init();
    f.set_enabled(true);
    f.add_rule(make_rule(0x123, false));

    TEST_ASSERT_FALSE(f.check_message(make_msg(0x123)));
    TEST_ASSERT_TRUE(f.check_message(make_msg(0x456)));
}

// An accept rule for a different frame type still constitutes an allow-list,
// so a standard frame that matches nothing must still be rejected.
void test_filter_accept_list_applies_across_frame_types(void) {
    CANFilter f;
    f.init();
    f.set_enabled(true);
    FilterRule ext = make_rule(0x123, true);
    ext.type = CANFrameType::EXTENDED;
    f.add_rule(ext);

    TEST_ASSERT_FALSE(f.check_message(make_msg(0x123)));   // standard frame
}

// Filtering off, or no rules at all, means pass everything.
void test_filter_disabled_passes_all(void) {
    CANFilter f;
    f.init();
    TEST_ASSERT_TRUE(f.check_message(make_msg(0x999)));

    f.set_enabled(true);
    TEST_ASSERT_TRUE(f.check_message(make_msg(0x999)));   // enabled, zero rules
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_filter_accept_list_rejects_unmatched);
    RUN_TEST(test_filter_reject_list_blocks_matched);
    RUN_TEST(test_filter_accept_list_applies_across_frame_types);
    RUN_TEST(test_filter_disabled_passes_all);
    return UNITY_END();
}
```

- [ ] **Step 2: Make the filter compile on a host**

In `src/core/can_filter.h`, replace `#include "../hal/can_hal.h"` with `#include "can_types.h"`.

In `src/core/can_filter.cpp`, delete `#include <Arduino.h>` and delete the `Serial.println("ERROR: Maximum filters reached");` line from `add_rule` (lines 11-14 become a bare `return -1;` inside the guard). Core code does not log.

- [ ] **Step 3: Run the tests to confirm they fail for the right reason**

Run: `pio test -e native -f test_can_filter`
Expected: `test_filter_accept_list_rejects_unmatched` FAILS (`check_message(0x456)` returns true), and `test_filter_accept_list_applies_across_frame_types` FAILS. The other two PASS. This is the defect, reproduced.

- [ ] **Step 4: Fix `check_message`**

Replace the body of `CANFilter::check_message` (`can_filter.cpp:62-109`) with:

```cpp
bool CANFilter::check_message(const CANMessage& msg) const {
    if (!filter_enabled) {
        return true;
    }
    if (rule_count == 0) {
        return true;
    }

    bool matched_accept = false;
    bool matched_reject = false;
    bool allow_list_present = false;

    for (int i = 0; i < rule_count; i++) {
        if (!rules[i].enabled) {
            continue;
        }

        // Counted before the frame-type check: an accept rule for extended
        // frames still means the user is running an allow-list, and a
        // standard frame that matches nothing must not sail through it.
        if (rules[i].accept) {
            allow_list_present = true;
        }

        if (rules[i].type != msg.type) {
            continue;
        }

        if (id_matches(msg.id, rules[i])) {
            if (rules[i].accept) {
                matched_accept = true;
            } else {
                matched_reject = true;
            }
        }
    }

    // Reject always wins over accept.
    if (matched_reject) {
        return false;
    }
    if (matched_accept) {
        return true;
    }

    // Nothing matched. With an allow-list active that means reject; with only
    // reject rules configured it means this frame was never on the block list.
    return !allow_list_present;
}
```

- [ ] **Step 5: Run the tests to verify they pass**

Run: `pio test -e native -f test_can_filter`
Expected: all four PASS.

- [ ] **Step 6: Commit**

```bash
git add -A
git commit -m "fix(can): allow-list filter no longer passes unmatched traffic

check_message() fell through to 'return true' when no rule matched, so an
accept rule passed 100% of the bus - the exact inverse of its purpose, and
contradicted by its own comment.

Co-Authored-By: Claude Opus 5 (1M context) <noreply@anthropic.com>"
```

---

## Task 6: The `ICANBus` seam, `FakeCanBus`, and transmitter injection

`CANTransmitter::send()` calls `CANHAL::transmit()` directly and `update()` calls `millis()`, so it cannot run on a host. This task introduces the seam and converts the simpler of the two consumers.

**Files:**
- Create: `src/core/i_can_bus.h`, `test/fakes/fake_can_bus.h`, `test/test_can_transmitter/test_can_transmitter.cpp`
- Modify: `src/core/can_transmitter.h`, `src/core/can_transmitter.cpp`

**Interfaces:**
- Consumes: `can_types.h`.
- Produces: `class ICANBus` with `transmit`, `receive`, `get_stats`, `is_bus_error`, `recover`. `CANTransmitter(ICANBus& bus)` constructor; `void update(uint32_t now)`. Tasks 7, 9, 11 all depend on `ICANBus`.

- [ ] **Step 1: Create `src/core/i_can_bus.h`**

```cpp
#pragma once

/**
 * The CAN bus seam.
 *
 * COMPILE-TIME ABSTRACTION ONLY. The shipped firmware always binds this to
 * Esp32CanBus and talks straight to the TWAI peripheral - the device is
 * standalone and never depends on a host at runtime. The cost is one vtable
 * lookup per frame. The only other implementation is FakeCanBus, which lives
 * in test/ and is never flashed.
 *
 * Bus I/O only. Lifecycle (init/start/stop/set_baudrate) stays on the
 * concrete class so the fake stays trivial.
 */

#include "can_types.h"

class ICANBus {
public:
    virtual ~ICANBus() = default;

    // Send one frame. Returns false if the bus is not running or the TX
    // queue did not accept it within the driver timeout.
    virtual bool transmit(const CANMessage& msg) = 0;

    // Non-blocking receive of one frame. Returns false when nothing is queued.
    virtual bool receive(CANMessage& msg) = 0;

    virtual CANStats get_stats() const = 0;

    // True when the controller is bus-off or already recovering.
    virtual bool is_bus_error() const = 0;

    // Ask the controller to begin bus-off recovery. Returns false if it
    // could not be initiated. Recovery policy lives in CanService.
    virtual bool recover() = 0;
};
```

- [ ] **Step 2: Create `test/fakes/fake_can_bus.h`**

```cpp
#pragma once

/**
 * Scripted ICANBus for host tests.
 * rx_queue is what the bus will hand back; sent records what the code
 * under test emitted.
 */

#include <deque>
#include <vector>

#include "core/i_can_bus.h"

class FakeCanBus : public ICANBus {
public:
    std::deque<CANMessage>  rx_queue;
    std::vector<CANMessage> sent;
    CANStats stats{};
    bool bus_error      = false;
    bool recover_called = false;
    bool transmit_ok    = true;

    bool transmit(const CANMessage& msg) override {
        if (!transmit_ok) {
            return false;
        }
        sent.push_back(msg);
        stats.tx_count++;
        return true;
    }

    bool receive(CANMessage& msg) override {
        if (rx_queue.empty()) {
            return false;
        }
        msg = rx_queue.front();
        rx_queue.pop_front();
        stats.rx_count++;
        return true;
    }

    CANStats get_stats() const override { return stats; }
    bool is_bus_error() const override  { return bus_error; }

    bool recover() override {
        recover_called = true;
        return true;
    }

    // Convenience for tests.
    void queue(uint32_t id, uint8_t dlc = 8) {
        CANMessage m = {};
        m.id   = id;
        m.dlc  = dlc;
        m.type = CANFrameType::STANDARD;
        rx_queue.push_back(m);
    }
};
```

- [ ] **Step 3: Write the failing transmitter test**

`test/test_can_transmitter/test_can_transmitter.cpp`:

```cpp
#include <unity.h>

#include "core/can_transmitter.h"
#include "fakes/fake_can_bus.h"

void setUp(void) {}
void tearDown(void) {}

static CANMessage make_msg(uint32_t id) {
    CANMessage m = {};
    m.id   = id;
    m.dlc  = 2;
    m.type = CANFrameType::STANDARD;
    return m;
}

void test_transmitter_send_reaches_the_bus(void) {
    FakeCanBus bus;
    CANTransmitter tx(bus);
    tx.init(0);

    TEST_ASSERT_TRUE(tx.send(make_msg(0x100)));
    TEST_ASSERT_EQUAL_UINT32(1, bus.sent.size());
    TEST_ASSERT_EQUAL_UINT32(0x100, bus.sent[0].id);
}

void test_transmitter_periodic_interval(void) {
    FakeCanBus bus;
    CANTransmitter tx(bus);
    tx.init(1000);

    TEST_ASSERT_EQUAL_INT(0, tx.add_periodic(make_msg(0x200), 100));

    tx.update(1050);
    TEST_ASSERT_EQUAL_UINT32(0, bus.sent.size());   // not yet due

    tx.update(1100);
    TEST_ASSERT_EQUAL_UINT32(1, bus.sent.size());   // due

    tx.update(1150);
    TEST_ASSERT_EQUAL_UINT32(1, bus.sent.size());   // not due again

    tx.update(1200);
    TEST_ASSERT_EQUAL_UINT32(2, bus.sent.size());
}

// The transmitter already used overflow-safe subtraction; this pins it so a
// later edit cannot regress it into the emulator's mistake.
void test_transmitter_periodic_survives_millis_wrap(void) {
    FakeCanBus bus;
    CANTransmitter tx(bus);
    const uint32_t near_wrap = 0xFFFFFF00u;
    tx.init(near_wrap);

    tx.add_periodic(make_msg(0x300), 500);

    tx.update(near_wrap + 400);   // still 0xFFFFFF... - not due
    TEST_ASSERT_EQUAL_UINT32(0, bus.sent.size());

    tx.update(near_wrap + 500);   // wrapped past zero - due
    TEST_ASSERT_EQUAL_UINT32(1, bus.sent.size());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_transmitter_send_reaches_the_bus);
    RUN_TEST(test_transmitter_periodic_interval);
    RUN_TEST(test_transmitter_periodic_survives_millis_wrap);
    return UNITY_END();
}
```

- [ ] **Step 4: Run it to confirm it fails**

Run: `pio test -e native -f test_can_transmitter`
Expected: FAIL to compile — `CANTransmitter` has no constructor taking a bus, and `init`/`update` take no argument.

- [ ] **Step 5: Convert `CANTransmitter` to injection and explicit time**

`src/core/can_transmitter.h` — replace the include and the class's public front matter:

```cpp
#include "can_types.h"
#include "i_can_bus.h"

struct PeriodicMessage {
    CANMessage message;
    uint32_t interval_ms;
    uint32_t last_sent;
    bool active;
};

class CANTransmitter {
public:
    explicit CANTransmitter(ICANBus& bus) : bus(bus) {}

    /**
     * Initialize transmitter.
     * @param now current millisecond tick, used to seed periodic schedules
     */
    void init(uint32_t now);

    bool send(const CANMessage& msg);

    int  add_periodic(const CANMessage& msg, uint32_t interval_ms);
    bool remove_periodic(int index);
    bool set_periodic_enabled(int index, bool enabled);
    bool set_periodic_interval(int index, uint32_t interval_ms);
    void clear_periodic();

    /**
     * Send any periodic messages that have come due.
     * @param now current millisecond tick, supplied by CanService
     */
    void update(uint32_t now);

    int get_periodic_count() const { return periodic_count; }
    const PeriodicMessage* get_periodic(int index) const;

private:
    static constexpr int MAX_PERIODIC = 10;
    ICANBus& bus;
    PeriodicMessage periodic_messages[MAX_PERIODIC];
    int periodic_count = 0;
    uint32_t now_ = 0;      // last tick seen; lets add_periodic seed last_sent
};
```

`src/core/can_transmitter.cpp` — delete `#include <Arduino.h>`, and change the five functions that touched `millis()`, `CANHAL`, or `Serial`:

```cpp
void CANTransmitter::init(uint32_t now) {
    now_ = now;
    clear_periodic();
}

bool CANTransmitter::send(const CANMessage& msg) {
    return bus.transmit(msg);
}

int CANTransmitter::add_periodic(const CANMessage& msg, uint32_t interval_ms) {
    if (periodic_count >= MAX_PERIODIC) {
        return -1;
    }

    periodic_messages[periodic_count].message     = msg;
    periodic_messages[periodic_count].interval_ms = interval_ms;
    periodic_messages[periodic_count].last_sent   = now_;
    periodic_messages[periodic_count].active      = true;

    return periodic_count++;
}

bool CANTransmitter::set_periodic_enabled(int index, bool enabled) {
    if (index < 0 || index >= periodic_count) {
        return false;
    }
    periodic_messages[index].active = enabled;
    if (enabled) {
        periodic_messages[index].last_sent = now_;
    }
    return true;
}

void CANTransmitter::update(uint32_t now) {
    now_ = now;

    for (int i = 0; i < periodic_count; i++) {
        if (!periodic_messages[i].active) {
            continue;
        }

        // Subtraction, not addition: correct across the 49.7-day millis() wrap.
        if (now - periodic_messages[i].last_sent >= periodic_messages[i].interval_ms) {
            if (bus.transmit(periodic_messages[i].message)) {
                periodic_messages[i].last_sent = now;
            }
        }
    }
}
```

- [ ] **Step 6: Run the tests to verify they pass**

Run: `pio test -e native -f test_can_transmitter`
Expected: all three PASS.

- [ ] **Step 7: Commit**

```bash
git add -A
git commit -m "refactor(can): inject ICANBus into the transmitter

Adds the ICANBus seam and a FakeCanBus for host tests. The transmitter now
takes its bus by reference and receives the current tick as a parameter, so
periodic scheduling is testable without waiting 49 days for a wrap.

Compile-time abstraction only - the firmware still binds straight to TWAI.

Co-Authored-By: Claude Opus 5 (1M context) <noreply@anthropic.com>"
```

**NOTE:** the device build is intentionally red from here until Task 9. `main.cpp` constructs `CANTransmitter transmitter;` with no bus. Do not try to fix it in this task — Task 9 supplies `Esp32CanBus` and Task 13 rewrites `main.cpp`.

---

## Task 7: Emulator injection and the `millis()` wrap (defect #7)

`device_emulator.cpp:94` computes `send_time = millis() + delay_ms` and line 112 compares `now >= send_time`. Across the 49.7-day `millis()` wrap this either fires every scheduled response instantly or never fires them at all. `can_transmitter.cpp:72` already does this correctly by subtraction; the emulator does not.

**Files:**
- Create: `test/test_device_emulator/test_device_emulator.cpp`
- Modify: `src/core/device_emulator.h`, `src/core/device_emulator.cpp`

**Interfaces:**
- Consumes: `ICANBus` from Task 6.
- Produces: `DeviceEmulator(ICANBus& bus)`; `void process_message(const CANMessage& msg, uint32_t now)`; `void update(uint32_t now)`.

- [ ] **Step 1: Write the failing test**

`test/test_device_emulator/test_device_emulator.cpp`:

```cpp
#include <unity.h>

#include <cstring>

#include "core/device_emulator.h"
#include "fakes/fake_can_bus.h"

void setUp(void) {}
void tearDown(void) {}

static CANMessage make_msg(uint32_t id) {
    CANMessage m = {};
    m.id   = id;
    m.dlc  = 1;
    m.type = CANFrameType::STANDARD;
    return m;
}

static EmulationRule make_rule(uint32_t trigger, uint32_t response_id, uint32_t delay_ms) {
    EmulationRule r = {};
    r.trigger_id = trigger;
    r.response   = make_msg(response_id);
    r.delay_ms   = delay_ms;
    r.enabled    = true;
    std::strncpy(r.name, "test", sizeof(r.name) - 1);
    return r;
}

void test_emulator_responds_to_trigger_after_delay(void) {
    FakeCanBus bus;
    DeviceEmulator em(bus);
    em.init();
    em.start();
    em.add_rule(make_rule(0x7DF, 0x7E8, 50));

    em.process_message(make_msg(0x7DF), 1000);

    em.update(1040);
    TEST_ASSERT_EQUAL_UINT32(0, bus.sent.size());   // delay not elapsed

    em.update(1050);
    TEST_ASSERT_EQUAL_UINT32(1, bus.sent.size());
    TEST_ASSERT_EQUAL_UINT32(0x7E8, bus.sent[0].id);

    em.update(1100);
    TEST_ASSERT_EQUAL_UINT32(1, bus.sent.size());   // fires exactly once
}

// Guards defect #7. send_time wraps past zero; the response must still fire
// at the right moment, and must not fire early.
void test_emulator_fires_across_millis_wrap(void) {
    FakeCanBus bus;
    DeviceEmulator em(bus);
    em.init();
    em.start();
    em.add_rule(make_rule(0x100, 0x200, 500));

    const uint32_t near_wrap = 0xFFFFFF00u;   // send_time becomes 0x000000F4
    em.process_message(make_msg(0x100), near_wrap);

    em.update(near_wrap + 400);    // 0xFFFFFF90 - not due
    TEST_ASSERT_EQUAL_UINT32(0, bus.sent.size());

    em.update(near_wrap + 500);    // 0x000000F4 - due
    TEST_ASSERT_EQUAL_UINT32(1, bus.sent.size());
}

void test_emulator_ignores_messages_when_stopped(void) {
    FakeCanBus bus;
    DeviceEmulator em(bus);
    em.init();
    em.add_rule(make_rule(0x100, 0x200, 0));

    em.process_message(make_msg(0x100), 1000);   // never started
    em.update(2000);
    TEST_ASSERT_EQUAL_UINT32(0, bus.sent.size());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_emulator_responds_to_trigger_after_delay);
    RUN_TEST(test_emulator_fires_across_millis_wrap);
    RUN_TEST(test_emulator_ignores_messages_when_stopped);
    return UNITY_END();
}
```

- [ ] **Step 2: Run it to confirm it fails**

Run: `pio test -e native -f test_device_emulator`
Expected: FAIL to compile — no constructor taking a bus, `process_message` takes one argument, `update` takes none.

- [ ] **Step 3: Update the emulator header**

In `src/core/device_emulator.h`: replace `#include "../hal/can_hal.h"` with `#include "can_types.h"` and `#include "i_can_bus.h"`, then change these members:

```cpp
class DeviceEmulator {
public:
    explicit DeviceEmulator(ICANBus& bus) : bus(bus) {}

    void init();
    void start();
    void stop();
    bool is_running() const { return running; }

    int  add_rule(const EmulationRule& rule);
    bool remove_rule(int index);
    bool set_rule_enabled(int index, bool enabled);
    const EmulationRule* get_rule(int index) const;
    int  get_rule_count() const { return rule_count; }
    void clear_rules();

    /**
     * Check an incoming frame against the rules and schedule any responses.
     * @param now current millisecond tick, supplied by CanService
     */
    void process_message(const CANMessage& msg, uint32_t now);

    /**
     * Send any scheduled responses that have come due.
     * @param now current millisecond tick, supplied by CanService
     */
    void update(uint32_t now);

    // Deferred to sub-project C (persistence). These return false today;
    // that is a documented gap, not an unfinished edit.
    bool load_profile(const char* filename);
    bool save_profile(const char* filename);

private:
    static constexpr int MAX_RULES = 20;

    struct PendingResponse {
        CANMessage message;
        uint32_t send_time;
        bool active;
    };

    ICANBus& bus;
    EmulationRule rules[MAX_RULES];
    int rule_count = 0;
    bool running = false;

    PendingResponse pending_responses[MAX_RULES];
    int pending_count = 0;

    void add_pending_response(const CANMessage& msg, uint32_t delay_ms, uint32_t now);
};
```

- [ ] **Step 4: Fix the implementation**

In `src/core/device_emulator.cpp`: delete `#include <Arduino.h>`, delete every `Serial.println` / `Serial.printf` call, and replace these four functions:

```cpp
void DeviceEmulator::process_message(const CANMessage& msg, uint32_t now) {
    if (!running) {
        return;
    }

    for (int i = 0; i < rule_count; i++) {
        if (!rules[i].enabled) {
            continue;
        }
        if (msg.id == rules[i].trigger_id) {
            add_pending_response(rules[i].response, rules[i].delay_ms, now);
        }
    }
}

void DeviceEmulator::add_pending_response(const CANMessage& msg, uint32_t delay_ms,
                                          uint32_t now) {
    if (pending_count >= MAX_RULES) {
        return;   // queue full; drop rather than overwrite a pending response
    }

    pending_responses[pending_count].message   = msg;
    pending_responses[pending_count].send_time = now + delay_ms;   // may wrap
    pending_responses[pending_count].active    = true;
    pending_count++;
}

void DeviceEmulator::update(uint32_t now) {
    if (!running) {
        return;
    }

    for (int i = 0; i < pending_count; i++) {
        if (!pending_responses[i].active) {
            continue;
        }

        // Signed difference, not 'now >= send_time'. send_time is allowed to
        // wrap past zero; the signed delta stays correct across the wrap as
        // long as the delay is under ~24 days, which every real delay is.
        if (static_cast<int32_t>(now - pending_responses[i].send_time) >= 0) {
            if (bus.transmit(pending_responses[i].message)) {
                pending_responses[i].active = false;
            }
        }
    }

    // Compact the queue, dropping sent responses.
    int write_idx = 0;
    for (int read_idx = 0; read_idx < pending_count; read_idx++) {
        if (pending_responses[read_idx].active) {
            if (write_idx != read_idx) {
                pending_responses[write_idx] = pending_responses[read_idx];
            }
            write_idx++;
        }
    }
    pending_count = write_idx;
}

bool DeviceEmulator::load_profile(const char* filename) {
    (void)filename;
    return false;   // deferred to sub-project C
}

bool DeviceEmulator::save_profile(const char* filename) {
    (void)filename;
    return false;   // deferred to sub-project C
}
```

- [ ] **Step 5: Run the tests to verify they pass**

Run: `pio test -e native -f test_device_emulator`
Expected: all three PASS.

- [ ] **Step 6: Commit**

```bash
git add -A
git commit -m "fix(can): emulator responses survive the millis() wrap

send_time = millis() + delay_ms compared with 'now >= send_time' breaks at
the 49.7-day wrap - every response either fires instantly or never. Uses a
signed delta, and takes the tick as a parameter so a test can prove it.

Co-Authored-By: Claude Opus 5 (1M context) <noreply@anthropic.com>"
```

---

## Task 8: Make the sniffer a passive sink

`CANSniffer::update()` (`can_sniffer.cpp:34`) pulls straight from `CANHAL::receive()` in an unbounded `while` loop. In the new design `CanService` owns the receive loop and its frame budget, so the sniffer becomes what it should have been: the thing that holds captured history and the message-rate statistic.

**Files:**
- Modify: `src/core/can_sniffer.h`, `src/core/can_sniffer.cpp`

**Interfaces:**
- Consumes: `can_types.h`, `ring_buffer.h`.
- Produces: `void CANSniffer::record(const CANMessage& msg)`, `void CANSniffer::update_stats(uint32_t now)`, `void CANSniffer::init(uint32_t now)`. `CanService` (Task 11) calls all three.

- [ ] **Step 1: Update the header**

In `src/core/can_sniffer.h`, replace `#include "../hal/can_hal.h"` with `#include "can_types.h"`, and replace `init()`/`update()` with:

```cpp
    /**
     * Initialize sniffer.
     * @param now current millisecond tick, seeds the rate window
     */
    void init(uint32_t now);

    /**
     * Record one captured frame. Called by CanService for every frame that
     * survives the filter.
     */
    void record(const CANMessage& msg);

    /**
     * Recompute the messages-per-second figure. Cheap; call every tick.
     * @param now current millisecond tick, supplied by CanService
     */
    void update_stats(uint32_t now);

    /**
     * Clear captured history and reset counters.
     * @param now current millisecond tick, re-seeds the rate window
     */
    void clear(uint32_t now);
```

- [ ] **Step 2: Update the implementation**

Replace `src/core/can_sniffer.cpp` in full:

```cpp
#include "can_sniffer.h"

void CANSniffer::init(uint32_t now) {
    message_buffer.clear();
    message_count  = 0;
    last_count     = 0;
    last_stat_time = now;
    running        = false;
}

void CANSniffer::start() {
    running = true;
}

void CANSniffer::stop() {
    running = false;
}

void CANSniffer::record(const CANMessage& msg) {
    if (!running) {
        return;
    }

    message_buffer.push(msg);
    message_count++;

    if (message_callback) {
        message_callback(msg);
    }
}

void CANSniffer::update_stats(uint32_t now) {
    uint32_t elapsed = now - last_stat_time;   // wrap-safe subtraction
    if (elapsed >= 1000) {
        messages_per_sec = (message_count - last_count) * 1000.0f / elapsed;
        last_count       = message_count;
        last_stat_time   = now;
    }
}

void CANSniffer::on_message_received(CANMessageCallback callback) {
    message_callback = callback;
}

void CANSniffer::clear(uint32_t now) {
    message_buffer.clear();
    message_count    = 0;
    last_count       = 0;
    messages_per_sec = 0.0f;
    last_stat_time   = now;
}

uint32_t CANSniffer::get_message_count() const {
    return message_count;
}

float CANSniffer::get_messages_per_second() const {
    return messages_per_sec;
}
```

Note `start()` no longer calls `clear()`. Clearing captured history is now an explicit user action, not a side effect of resuming capture.

- [ ] **Step 3: Verify the core layer is platform-free**

```bash
grep -rn "Arduino.h\|millis()\|Serial\.\|driver/twai.h\|M5GFX\|lvgl.h" src/core/ src/utils/
```

Expected: **no output.** If anything matches, remove it — this is the Global Constraint that makes the native build work.

- [ ] **Step 4: Confirm the native suite still passes**

Run: `pio test -e native`
Expected: all suites from Tasks 3–7 PASS. The sniffer has no test of its own yet; Task 11 covers it through `CanService`.

- [ ] **Step 5: Commit**

```bash
git add -A
git commit -m "refactor(can): sniffer becomes a passive capture sink

update() pulled straight from CANHAL in an unbounded loop. CanService now
owns the receive loop and its frame budget; the sniffer holds history and
the message-rate statistic. Core layer is now free of Arduino and TWAI.

Co-Authored-By: Claude Opus 5 (1M context) <noreply@anthropic.com>"
```

---

## Task 9: `Esp32CanBus` — the TWAI implementation (defects #2, #11)

`can_hal.cpp:125` declares `twai_message_t twai_msg;` on the stack and sets only four of its fields, leaving the `ss` (single-shot), `self` (self-reception), and `dlc_non_comp` flag bits as whatever the stack held — so the device can transmit a single-shot or self-received frame by accident. Line 179 assigns `status.bus_error_count` to `bus_off_count`, conflating error frames with bus-off events. And nothing anywhere calls `twai_initiate_recovery()`.

**Files:**
- Create: `src/hal/esp32_can_bus.h`, `src/hal/esp32_can_bus.cpp`
- Delete: `src/hal/can_hal.h`, `src/hal/can_hal.cpp`

**Interfaces:**
- Consumes: `ICANBus`, `can_timing.h`, `can_types.h`.
- Produces: `class Esp32CanBus : public ICANBus` with additional lifecycle `bool init(CANBaudRate)`, `bool start()`, `bool stop()`, `bool set_baudrate(CANBaudRate)`, `uint32_t available()`, `void reset_stats()`, `CANBaudRate get_baudrate() const`. Task 11 and Task 13 consume it.

- [ ] **Step 1: Create `src/hal/esp32_can_bus.h`**

```cpp
#pragma once

/**
 * ESP32-P4 TWAI implementation of ICANBus.
 * This is what the firmware always runs. Lifecycle lives here rather than on
 * the interface so that FakeCanBus stays trivial.
 */

#include "driver/twai.h"

#include "../core/can_timing.h"
#include "../core/i_can_bus.h"
#include "../config/can_config.h"

class Esp32CanBus : public ICANBus {
public:
    bool init(CANBaudRate baudrate = DEFAULT_CAN_BAUD);
    bool start();
    bool stop();
    bool set_baudrate(CANBaudRate baudrate);

    bool transmit(const CANMessage& msg) override;
    bool receive(CANMessage& msg) override;
    CANStats get_stats() const override;
    bool is_bus_error() const override;
    bool recover() override;

    uint32_t available() const;
    void reset_stats();
    CANBaudRate get_baudrate() const { return current_baudrate; }
    bool is_running() const { return running; }

private:
    static twai_timing_config_t timing_config_for(CanTimingId id);

    CANBaudRate current_baudrate = DEFAULT_CAN_BAUD;
    mutable CANStats stats = {};
    bool initialized = false;
    bool running = false;
};
```

- [ ] **Step 2: Create `src/hal/esp32_can_bus.cpp`**

```cpp
#include "esp32_can_bus.h"

#include <Arduino.h>
#include <cstring>

#include "../config/hardware_config.h"

twai_timing_config_t Esp32CanBus::timing_config_for(CanTimingId id) {
    // The only place ESP-IDF register values appear. The enum-to-rate mapping
    // that used to live here is now in core/can_timing.h, under test.
    switch (id) {
        case CanTimingId::T_10K:  return TWAI_TIMING_CONFIG_10KBITS();
        case CanTimingId::T_20K:  return TWAI_TIMING_CONFIG_20KBITS();
        case CanTimingId::T_50K:  return TWAI_TIMING_CONFIG_50KBITS();
        case CanTimingId::T_100K: return TWAI_TIMING_CONFIG_100KBITS();
        case CanTimingId::T_125K: return TWAI_TIMING_CONFIG_125KBITS();
        case CanTimingId::T_250K: return TWAI_TIMING_CONFIG_250KBITS();
        case CanTimingId::T_500K: return TWAI_TIMING_CONFIG_500KBITS();
        case CanTimingId::T_800K: return TWAI_TIMING_CONFIG_800KBITS();
        case CanTimingId::T_1M:   return TWAI_TIMING_CONFIG_1MBITS();
    }
    return TWAI_TIMING_CONFIG_500KBITS();
}

bool Esp32CanBus::init(CANBaudRate baudrate) {
    if (initialized) {
        return true;
    }

    twai_timing_config_t t_config = timing_config_for(can_timing_for(baudrate).id);
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();
    twai_general_config_t g_config =
        TWAI_GENERAL_CONFIG_DEFAULT(CAN_TX_PIN, CAN_RX_PIN, TWAI_MODE_NORMAL);
    g_config.rx_queue_len = CAN_RX_BUFFER_SIZE;
    g_config.tx_queue_len = CAN_TX_QUEUE_SIZE;

    esp_err_t err = twai_driver_install(&g_config, &t_config, &f_config);
    if (err != ESP_OK) {
        Serial.printf("ERROR: twai_driver_install failed: %d\n", err);
        return false;
    }

    current_baudrate = baudrate;
    initialized = true;
    reset_stats();

    Serial.printf("CAN initialized at %u baud\n",
                  static_cast<unsigned>(can_timing_for(baudrate).nominal_bps));
    return true;
}

bool Esp32CanBus::start() {
    if (!initialized) {
        return false;
    }
    if (running) {
        return true;
    }

    esp_err_t err = twai_start();
    if (err != ESP_OK) {
        Serial.printf("ERROR: twai_start failed: %d\n", err);
        return false;
    }

    running = true;
    return true;
}

bool Esp32CanBus::stop() {
    if (!running) {
        return true;
    }
    if (twai_stop() != ESP_OK) {
        return false;
    }
    running = false;
    return true;
}

bool Esp32CanBus::set_baudrate(CANBaudRate baudrate) {
    if (running) {
        stop();
    }
    if (initialized) {
        twai_driver_uninstall();
        initialized = false;
    }
    return init(baudrate);
}

bool Esp32CanBus::transmit(const CANMessage& msg) {
    if (!running) {
        return false;
    }

    // Zero-initialized: the flag bits ss, self, and dlc_non_comp share this
    // struct. Leaving them as stack residue transmits single-shot or
    // self-reception frames by accident.
    twai_message_t twai_msg = {};
    twai_msg.identifier       = msg.id;
    twai_msg.data_length_code = msg.dlc;
    twai_msg.rtr              = msg.rtr;
    twai_msg.extd             = (msg.type == CANFrameType::EXTENDED);
    if (msg.dlc <= 8) {
        std::memcpy(twai_msg.data, msg.data, msg.dlc);
    }

    if (twai_transmit(&twai_msg, pdMS_TO_TICKS(10)) != ESP_OK) {
        return false;
    }

    stats.tx_count++;
    return true;
}

bool Esp32CanBus::receive(CANMessage& msg) {
    if (!running) {
        return false;
    }

    twai_message_t twai_msg = {};
    if (twai_receive(&twai_msg, 0) != ESP_OK) {   // non-blocking
        return false;
    }

    msg.id           = twai_msg.identifier;
    msg.dlc          = twai_msg.data_length_code > 8 ? 8 : twai_msg.data_length_code;
    msg.rtr          = twai_msg.rtr;
    msg.type         = twai_msg.extd ? CANFrameType::EXTENDED : CANFrameType::STANDARD;
    msg.timestamp_ms = millis();

    // Zero the tail so the sniffer table never shows stack residue as bus data.
    std::memset(msg.data, 0, sizeof(msg.data));
    std::memcpy(msg.data, twai_msg.data, msg.dlc);

    stats.rx_count++;
    return true;
}

uint32_t Esp32CanBus::available() const {
    if (!running) {
        return 0;
    }
    twai_status_info_t status;
    twai_get_status_info(&status);
    return status.msgs_to_rx;
}

CANStats Esp32CanBus::get_stats() const {
    if (running) {
        twai_status_info_t status;
        twai_get_status_info(&status);
        stats.error_count = status.tx_error_counter + status.rx_error_counter;
        // bus_off_count is incremented by recover(), not read from the driver.
        // status.bus_error_count counts error frames, which is a different
        // thing entirely - conflating them was the original bug.
    }
    return stats;
}

void Esp32CanBus::reset_stats() {
    stats = {};
}

bool Esp32CanBus::is_bus_error() const {
    if (!running) {
        return false;
    }
    twai_status_info_t status;
    twai_get_status_info(&status);
    return status.state == TWAI_STATE_BUS_OFF;
}

bool Esp32CanBus::recover() {
    twai_status_info_t status;
    twai_get_status_info(&status);

    if (status.state == TWAI_STATE_RECOVERING) {
        return true;   // already under way
    }
    if (status.state != TWAI_STATE_BUS_OFF) {
        return false;  // nothing to recover from
    }

    if (twai_initiate_recovery() != ESP_OK) {
        return false;
    }

    stats.bus_off_count++;
    running = false;   // the driver returns to STOPPED once recovery completes
    return true;
}
```

- [ ] **Step 3: Delete the old HAL and repoint its includes**

```bash
git rm src/hal/can_hal.h src/hal/can_hal.cpp
grep -rn "can_hal.h\|CANHAL::" src/
```

Every remaining hit is in `src/main.cpp`, which Task 13 rewrites. Leave them; the device build stays red until then.

- [ ] **Step 4: Verify the native suite is unaffected**

Run: `pio test -e native`
Expected: every suite PASSES. `src/hal/` is not in the native `build_src_filter`, so this task cannot have touched it.

- [ ] **Step 5: Commit**

```bash
git add -A
git commit -m "refactor(can): can_hal becomes Esp32CanBus implementing ICANBus

Fixes the uninitialized twai_message_t on transmit, whose ss/self/
dlc_non_comp flag bits were sent as stack garbage. Zeroes the receive data
tail. Stops conflating bus-off events with error-frame counts. Adds
twai_initiate_recovery(), which the codebase had never called.

Co-Authored-By: Claude Opus 5 (1M context) <noreply@anthropic.com>"
```

**A2 GATE:** `pio test -e native` runs all six suites green. `src/core/` and `src/utils/` contain no platform headers. The device build is knowingly red pending Task 13.

---

# Phase A3 — Service layer

## Task 10: Real bus load (defect #12)

`stats.bus_load_percent` is only ever assigned `0.0f` (`can_hal.cpp:189`), so the dashboard has always displayed a constant zero.

**Files:**
- Create: `src/services/can_stats.h`, `src/services/can_stats.cpp`, `test/test_can_stats/test_can_stats.cpp`
- Modify: `platformio.ini` (native `build_src_filter`)

**Interfaces:**
- Consumes: `can_types.h`.
- Produces: `class CanStatsCollector` with `set_bitrate(uint32_t)`, `record_rx(const CANMessage&)`, `record_tx(const CANMessage&)`, `update(uint32_t now)`, `reset(uint32_t now)`, `float bus_load_percent() const`, `uint32_t rx_count() const`, `uint32_t tx_count() const`. Task 11 owns one.

- [ ] **Step 1: Add `services/` to the native build**

In `platformio.ini`'s `[env:native]`, change:

```ini
build_src_filter = +<core/> +<utils/> +<services/can_stats.cpp> +<services/can_service.cpp>
```

`state_manager.cpp` stays out — it is device-side application state and Task 11's service does not depend on it.

- [ ] **Step 2: Write the failing test**

`test/test_can_stats/test_can_stats.cpp`:

```cpp
#include <unity.h>

#include "services/can_stats.h"

void setUp(void) {}
void tearDown(void) {}

static CANMessage std_frame(uint8_t dlc) {
    CANMessage m = {};
    m.id   = 0x123;
    m.dlc  = dlc;
    m.type = CANFrameType::STANDARD;
    return m;
}

// Guards defect #12. 100 standard frames with dlc=8 at 500 kbit/s:
//   bits per frame = (47 + 8*8) = 111, x1.1 for stuffing = 122
//   100 frames     = 12200 bits over a 1 second window
//   load           = 12200 / 500000 = 2.44%
void test_bus_load_matches_known_traffic(void) {
    CanStatsCollector s;
    s.reset(0);
    s.set_bitrate(500000);

    for (int i = 0; i < 100; i++) {
        s.record_rx(std_frame(8));
    }
    s.update(1000);   // close the window

    TEST_ASSERT_FLOAT_WITHIN(0.05f, 2.44f, s.bus_load_percent());
    TEST_ASSERT_EQUAL_UINT32(100, s.rx_count());
}

// An extended frame carries 20 more bits of overhead than a standard one.
void test_bus_load_accounts_for_extended_frames(void) {
    CanStatsCollector s;
    s.reset(0);
    s.set_bitrate(500000);

    CANMessage ext = std_frame(8);
    ext.type = CANFrameType::EXTENDED;
    for (int i = 0; i < 100; i++) {
        s.record_rx(ext);
    }
    s.update(1000);

    // (67 + 64) = 131, x1.1 = 144; 14400 / 500000 = 2.88%
    TEST_ASSERT_FLOAT_WITHIN(0.05f, 2.88f, s.bus_load_percent());
}

// A silent bus reads zero, and the window rolls rather than accumulating.
void test_bus_load_returns_to_zero_on_a_silent_bus(void) {
    CanStatsCollector s;
    s.reset(0);
    s.set_bitrate(500000);

    for (int i = 0; i < 100; i++) {
        s.record_rx(std_frame(8));
    }
    s.update(1000);
    TEST_ASSERT_TRUE(s.bus_load_percent() > 2.0f);

    s.update(2000);   // a second window with no traffic
    TEST_ASSERT_FLOAT_WITHIN(0.01f, 0.0f, s.bus_load_percent());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_bus_load_matches_known_traffic);
    RUN_TEST(test_bus_load_accounts_for_extended_frames);
    RUN_TEST(test_bus_load_returns_to_zero_on_a_silent_bus);
    return UNITY_END();
}
```

- [ ] **Step 3: Run it to confirm it fails**

Run: `pio test -e native -f test_can_stats`
Expected: FAIL to compile — `services/can_stats.h: No such file or directory`.

- [ ] **Step 4: Write `src/services/can_stats.h`**

```cpp
#pragma once

/**
 * Bus statistics collector.
 * Bus load is an estimate: it counts frame bits, including a flat 10%
 * allowance for bit stuffing, over a rolling one-second window. Error frames
 * are not counted. The UI labels the figure as estimated.
 */

#include <cstdint>

#include "../core/can_types.h"

class CanStatsCollector {
public:
    void set_bitrate(uint32_t bits_per_second);

    void record_rx(const CANMessage& msg);
    void record_tx(const CANMessage& msg);

    /**
     * Close the current window if a second has elapsed.
     * @param now current millisecond tick, supplied by CanService
     */
    void update(uint32_t now);

    void reset(uint32_t now);

    float    bus_load_percent() const { return load_percent; }
    uint32_t rx_count() const         { return rx; }
    uint32_t tx_count() const         { return tx; }

    /**
     * Bits a frame occupies on the wire, stuffing allowance included.
     * Standard: 47 bits of framing; extended: 67. Public for testability.
     */
    static uint32_t frame_bits(const CANMessage& msg);

private:
    uint32_t bitrate_bps  = 500000;
    uint32_t window_bits  = 0;
    uint32_t window_start = 0;
    float    load_percent = 0.0f;
    uint32_t rx = 0;
    uint32_t tx = 0;
};
```

- [ ] **Step 5: Write `src/services/can_stats.cpp`**

```cpp
#include "can_stats.h"

uint32_t CanStatsCollector::frame_bits(const CANMessage& msg) {
    const uint32_t overhead = (msg.type == CANFrameType::EXTENDED) ? 67u : 47u;
    const uint32_t raw      = overhead + 8u * msg.dlc;
    return (raw * 11u) / 10u;   // flat 10% bit-stuffing allowance
}

void CanStatsCollector::set_bitrate(uint32_t bits_per_second) {
    bitrate_bps = bits_per_second ? bits_per_second : 500000;
}

void CanStatsCollector::record_rx(const CANMessage& msg) {
    rx++;
    window_bits += frame_bits(msg);
}

void CanStatsCollector::record_tx(const CANMessage& msg) {
    tx++;
    window_bits += frame_bits(msg);
}

void CanStatsCollector::update(uint32_t now) {
    const uint32_t elapsed = now - window_start;   // wrap-safe subtraction
    if (elapsed < 1000) {
        return;
    }

    const float capacity = (static_cast<float>(bitrate_bps) * elapsed) / 1000.0f;
    load_percent = capacity > 0.0f ? (window_bits * 100.0f) / capacity : 0.0f;
    if (load_percent > 100.0f) {
        load_percent = 100.0f;
    }

    window_bits  = 0;
    window_start = now;
}

void CanStatsCollector::reset(uint32_t now) {
    window_bits  = 0;
    window_start = now;
    load_percent = 0.0f;
    rx = 0;
    tx = 0;
}
```

- [ ] **Step 6: Run the tests to verify they pass**

Run: `pio test -e native -f test_can_stats`
Expected: all three PASS.

- [ ] **Step 7: Commit**

```bash
git add -A
git commit -m "feat(can): compute real bus load

bus_load_percent was only ever assigned 0.0f, so the dashboard has always
shown a constant zero. Counts frame bits over a rolling one-second window
with a 10% stuffing allowance.

Co-Authored-By: Claude Opus 5 (1M context) <noreply@anthropic.com>"
```

---

## Task 11: `CanService` — the composition root

This is the task that makes the project work. Everything up to here has been preparation.

**Files:**
- Create: `src/services/can_service.h`, `src/services/can_service.cpp`, `test/test_can_service/test_can_service.cpp`
- Modify: `src/config/can_config.h`

**Interfaces:**
- Consumes: `ICANBus`, `CANSniffer`, `CANFilter`, `CANTransmitter`, `DeviceEmulator`, `CanStatsCollector`.
- Produces: `class CanService` with `explicit CanService(ICANBus&)`, `void init(uint32_t now)`, `bool start(uint32_t now)`, `void stop()`, `void update(uint32_t now)`, `bool send(const CANMessage&)`, accessors `sniffer()`, `filter()`, `transmitter()`, `emulator()`, `stats()`, `history()`, `bus_state()`. Task 13's `main.cpp` consumes all of it.

- [ ] **Step 1: Add the frame budget constant**

In `src/config/can_config.h`, after the buffer sizes:

```cpp
// Receive pipeline
#define MAX_FRAMES_PER_TICK     32      // Bounds one CanService::update() so a
                                        // saturated bus cannot starve LVGL.
                                        // At 1 Mbit/s a full bus delivers
                                        // ~8000 frames/s; tuned on hardware.
```

- [ ] **Step 2: Write the failing test**

`test/test_can_service/test_can_service.cpp`:

```cpp
#include <unity.h>

#include "fakes/fake_can_bus.h"
#include "services/can_service.h"

void setUp(void) {}
void tearDown(void) {}

static FilterRule accept_rule(uint32_t id) {
    FilterRule r = {};
    r.id      = id;
    r.mask    = 0x7FF;
    r.enabled = true;
    r.accept  = true;
    r.type    = CANFrameType::STANDARD;
    return r;
}

// A flooded bus must not be drained in one tick - lv_timer_handler() has to
// get a turn.
void test_can_service_respects_frame_budget(void) {
    FakeCanBus bus;
    CanService svc(bus);
    svc.init(0);
    svc.start(0);

    for (int i = 0; i < 1000; i++) {
        bus.queue(0x100 + i);
    }

    svc.update(10);

    TEST_ASSERT_EQUAL_UINT32(MAX_FRAMES_PER_TICK, svc.history().size());
    TEST_ASSERT_EQUAL_UINT32(1000 - MAX_FRAMES_PER_TICK, bus.rx_queue.size());
}

// The full pipeline: receive -> filter -> history -> emulator -> stats.
void test_can_service_routes_filtered_frames_to_history_and_emulator(void) {
    FakeCanBus bus;
    CanService svc(bus);
    svc.init(0);
    svc.start(0);

    svc.filter().add_rule(accept_rule(0x7DF));
    svc.filter().set_enabled(true);

    EmulationRule rule = {};
    rule.trigger_id      = 0x7DF;
    rule.response        = CANMessage{};
    rule.response.id     = 0x7E8;
    rule.response.dlc    = 1;
    rule.response.type   = CANFrameType::STANDARD;
    rule.delay_ms        = 0;
    rule.enabled         = true;
    svc.emulator().add_rule(rule);
    svc.emulator().start();

    bus.queue(0x7DF);    // passes the filter, triggers the emulator
    bus.queue(0x456);    // filtered out

    svc.update(10);

    TEST_ASSERT_EQUAL_UINT32(1, svc.history().size());
    const CANMessage* captured = svc.history().get(0);
    TEST_ASSERT_NOT_NULL(captured);
    TEST_ASSERT_EQUAL_UINT32(0x7DF, captured->id);

    TEST_ASSERT_EQUAL_UINT32(1, bus.sent.size());
    TEST_ASSERT_EQUAL_UINT32(0x7E8, bus.sent[0].id);
}

// Capture paused means no history, but the bus is still drained so the
// hardware RX queue does not overflow.
void test_can_service_drains_bus_while_capture_is_paused(void) {
    FakeCanBus bus;
    CanService svc(bus);
    svc.init(0);
    svc.start(0);
    svc.sniffer().stop();

    bus.queue(0x100);
    bus.queue(0x101);
    svc.update(10);

    TEST_ASSERT_EQUAL_UINT32(0, svc.history().size());
    TEST_ASSERT_EQUAL_UINT32(0, bus.rx_queue.size());
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_can_service_respects_frame_budget);
    RUN_TEST(test_can_service_routes_filtered_frames_to_history_and_emulator);
    RUN_TEST(test_can_service_drains_bus_while_capture_is_paused);
    return UNITY_END();
}
```

- [ ] **Step 3: Run it to confirm it fails**

Run: `pio test -e native -f test_can_service`
Expected: FAIL to compile — `services/can_service.h: No such file or directory`.

- [ ] **Step 4: Write `src/services/can_service.h`**

```cpp
#pragma once

/**
 * CAN service - the composition root for the CAN subsystem.
 *
 * Owns the bus and the four core modules, and exposes one update() to the
 * main loop. This is the object whose absence made every core module in this
 * project unreachable: they were all constructed, none were ever wired.
 *
 * Time enters the system here and nowhere else. update() takes the current
 * tick and passes it down, so wrap-around behaviour is unit-testable.
 */

#include "../config/can_config.h"
#include "../core/can_filter.h"
#include "../core/can_sniffer.h"
#include "../core/can_transmitter.h"
#include "../core/can_types.h"
#include "../core/device_emulator.h"
#include "../core/i_can_bus.h"
#include "can_stats.h"

class CanService {
public:
    explicit CanService(ICANBus& bus);

    void init(uint32_t now);
    bool start(uint32_t now);
    void stop();

    /**
     * One pass of the receive pipeline plus the scheduled transmissions.
     * Bounded by MAX_FRAMES_PER_TICK so a saturated bus cannot starve LVGL.
     * @param now current millisecond tick - the single time source
     */
    void update(uint32_t now);

    bool send(const CANMessage& msg);

    void set_bitrate(uint32_t bits_per_second);

    CANSniffer&     sniffer()     { return sniffer_; }
    CANFilter&      filter()      { return filter_; }
    CANTransmitter& transmitter() { return transmitter_; }
    DeviceEmulator& emulator()    { return emulator_; }

    const CanStatsCollector& stats() const { return stats_; }
    const RingBuffer<CANMessage>& history() const { return sniffer_.get_buffer(); }

    CANBusState bus_state() const { return state; }

private:
    void service_bus_health(uint32_t now);

    static constexpr uint8_t  MAX_RECOVERY_ATTEMPTS = 5;
    static constexpr uint32_t RECOVERY_SETTLE_MS    = 100;

    ICANBus&          bus;
    CANSniffer        sniffer_;
    CANFilter         filter_;
    CANTransmitter    transmitter_;
    DeviceEmulator    emulator_;
    CanStatsCollector stats_;

    CANBusState state = CANBusState::STOPPED;
    uint8_t  recovery_attempts = 0;
    uint32_t recovery_started  = 0;
};
```

- [ ] **Step 5: Write `src/services/can_service.cpp`**

Bus-off handling is stubbed to a single early return in this task; Task 12 fills it in and tests it.

```cpp
#include "can_service.h"

CanService::CanService(ICANBus& bus)
    : bus(bus), transmitter_(bus), emulator_(bus) {}

void CanService::init(uint32_t now) {
    sniffer_.init(now);
    filter_.init();
    transmitter_.init(now);
    emulator_.init();
    stats_.reset(now);
    state = CANBusState::STOPPED;
    recovery_attempts = 0;
}

bool CanService::start(uint32_t now) {
    sniffer_.start();
    stats_.reset(now);
    state = CANBusState::RUNNING;
    recovery_attempts = 0;
    return true;
}

void CanService::stop() {
    sniffer_.stop();
    emulator_.stop();
    state = CANBusState::STOPPED;
}

void CanService::set_bitrate(uint32_t bits_per_second) {
    stats_.set_bitrate(bits_per_second);
}

bool CanService::send(const CANMessage& msg) {
    if (!bus.transmit(msg)) {
        return false;
    }
    stats_.record_tx(msg);
    return true;
}

void CanService::update(uint32_t now) {
    service_bus_health(now);
    if (state == CANBusState::BUS_OFF || state == CANBusState::RECOVERING ||
        state == CANBusState::FAILED) {
        return;
    }

    // Bounded receive loop. The bus is drained even while capture is paused,
    // so the hardware RX queue does not overflow behind a paused UI.
    CANMessage msg;
    int budget = MAX_FRAMES_PER_TICK;
    while (budget-- > 0 && bus.receive(msg)) {
        stats_.record_rx(msg);

        if (!filter_.check_message(msg)) {
            continue;
        }

        sniffer_.record(msg);
        emulator_.process_message(msg, now);
    }

    transmitter_.update(now);
    emulator_.update(now);
    sniffer_.update_stats(now);
    stats_.update(now);
}

void CanService::service_bus_health(uint32_t now) {
    (void)now;   // Task 12 implements the recovery state machine here
}
```

- [ ] **Step 6: Run the tests to verify they pass**

Run: `pio test -e native -f test_can_service`
Expected: all three PASS.

- [ ] **Step 7: Run the whole suite**

Run: `pio test -e native`
Expected: every suite PASSES.

- [ ] **Step 8: Commit**

```bash
git add -A
git commit -m "feat(can): add CanService composition root

Owns the bus and the four core modules and exposes one update() to the main
loop. The receive pipeline is bounded by MAX_FRAMES_PER_TICK so a saturated
bus cannot starve LVGL. Time enters the system here and nowhere else.

This is the object whose absence made every core module unreachable.

Co-Authored-By: Claude Opus 5 (1M context) <noreply@anthropic.com>"
```

---

## Task 12: Bus-off recovery policy (defect #13)

`CANBusState::ERROR` was a one-way door — once the bus went off, only a reboot brought it back, and nothing in the repository ever called `twai_initiate_recovery()`. The policy is testable logic, so it lives here rather than in the HAL.

**Files:**
- Modify: `src/services/can_service.cpp`, `test/test_can_service/test_can_service.cpp`

**Interfaces:**
- Consumes: `ICANBus::is_bus_error()`, `ICANBus::recover()`.
- Produces: `CanService::bus_state()` transitions through `RUNNING -> BUS_OFF -> RECOVERING -> RUNNING`, or to `FAILED` after 5 attempts.

- [ ] **Step 1: Write the failing tests**

Append to `test/test_can_service/test_can_service.cpp`, and add the two `RUN_TEST` lines to `main()`:

```cpp
void test_can_service_recovers_from_bus_off(void) {
    FakeCanBus bus;
    CanService svc(bus);
    svc.init(0);
    svc.start(0);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(CANBusState::RUNNING),
                          static_cast<int>(svc.bus_state()));

    bus.bus_error = true;
    svc.update(1000);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(CANBusState::RECOVERING),
                          static_cast<int>(svc.bus_state()));
    TEST_ASSERT_TRUE(bus.recover_called);

    // Recovery succeeded on the hardware; the settle delay must still elapse.
    bus.bus_error = false;
    svc.update(1050);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(CANBusState::RECOVERING),
                          static_cast<int>(svc.bus_state()));

    svc.update(1100);
    TEST_ASSERT_EQUAL_INT(static_cast<int>(CANBusState::RUNNING),
                          static_cast<int>(svc.bus_state()));
}

void test_can_service_gives_up_after_five_attempts(void) {
    FakeCanBus bus;
    CanService svc(bus);
    svc.init(0);
    svc.start(0);

    bus.bus_error = true;   // never clears
    uint32_t now = 1000;
    for (int attempt = 0; attempt < 6; attempt++) {
        svc.update(now);          // notice, initiate recovery
        now += 200;
        svc.update(now);          // settle elapsed, still bus-off -> retry
        now += 200;
    }

    TEST_ASSERT_EQUAL_INT(static_cast<int>(CANBusState::FAILED),
                          static_cast<int>(svc.bus_state()));
}
```

- [ ] **Step 2: Run them to confirm they fail**

Run: `pio test -e native -f test_can_service`
Expected: both new tests FAIL — `bus_state()` stays `RUNNING` because `service_bus_health` is a stub.

- [ ] **Step 3: Implement the state machine**

Replace `CanService::service_bus_health` in `src/services/can_service.cpp`:

```cpp
void CanService::service_bus_health(uint32_t now) {
    switch (state) {
        case CANBusState::RUNNING:
            if (bus.is_bus_error()) {
                state = CANBusState::BUS_OFF;
            } else {
                break;
            }
            // fall through: attempt recovery on the same tick we notice

        case CANBusState::BUS_OFF:
            if (recovery_attempts >= MAX_RECOVERY_ATTEMPTS) {
                state = CANBusState::FAILED;
                break;
            }
            recovery_attempts++;
            recovery_started = now;
            bus.recover();
            state = CANBusState::RECOVERING;
            break;

        case CANBusState::RECOVERING:
            if (now - recovery_started < RECOVERY_SETTLE_MS) {
                break;   // give the controller time to settle
            }
            if (bus.is_bus_error()) {
                state = CANBusState::BUS_OFF;   // retry on the next tick
            } else {
                state = CANBusState::RUNNING;
                recovery_attempts = 0;
            }
            break;

        case CANBusState::STOPPED:
        case CANBusState::FAILED:
            break;
    }
}
```

- [ ] **Step 4: Run the tests to verify they pass**

Run: `pio test -e native -f test_can_service`
Expected: all five PASS.

- [ ] **Step 5: Run the whole suite**

Run: `pio test -e native`
Expected: every suite PASSES.

- [ ] **Step 6: Commit**

```bash
git add -A
git commit -m "feat(can): recover from bus-off instead of latching an error

CANBusState::ERROR was a one-way door and twai_initiate_recovery() was
never called anywhere in the project. Adds a RUNNING -> BUS_OFF ->
RECOVERING -> RUNNING state machine with 5 attempts before FAILED.

Policy lives in CanService so it is testable; the HAL keeps the mechanism.

Co-Authored-By: Claude Opus 5 (1M context) <noreply@anthropic.com>"
```

**A3 GATE:** `pio test -e native` green across all seven suites. The device build is still red pending Task 13.

---

# Phase A4 — Composition and the first real capture

## Task 13: Rewrite `main.cpp` as a composition root (defect #1)

The central defect. `main.cpp:69-72` calls `init()` on all four core modules and `start()` on none, so `CANSniffer::update()` has always returned immediately at `can_sniffer.cpp:28`.

**Files:**
- Modify: `src/main.cpp` (full rewrite)

**Interfaces:**
- Consumes: `Esp32CanBus`, `CanService`, `DisplayHAL`, `TouchHAL`, `StorageHAL`, `StateManager`, `UIManager`.
- Produces: `CanService& app_can_service()` — an accessor the UI layer uses in sub-project B.

- [ ] **Step 1: Replace `src/main.cpp` in full**

```cpp
#include <Arduino.h>

#include "config/app_config.h"
#include "config/can_config.h"
#include "config/hardware_config.h"

#include "hal/display_hal.h"
#include "hal/esp32_can_bus.h"
#include "hal/storage_hal.h"
#include "hal/touch_hal.h"

#include "services/can_service.h"
#include "services/state_manager.h"

#include "ui/ui_manager.h"

// Composition root. These two are the whole object graph: the bus is the
// hardware, the service owns everything that uses it.
static Esp32CanBus can_bus;
static CanService  can_service(can_bus);

// Accessor for the UI layer (sub-project B wires screens to this).
CanService& app_can_service() {
    return can_service;
}

static void fatal(const char* subsystem) {
    Serial.printf("FATAL: %s failed to initialize\n", subsystem);
    DisplayHAL::show_fatal(subsystem);
    // Park, but keep the watchdog fed and the message on screen.
    for (;;) {
        delay(1000);
    }
}

void setup() {
    Serial.begin(115200);
    delay(1000);

    Serial.println("========================================");
    Serial.printf("%s v%d.%d.%d\n", APP_NAME,
                  APP_VERSION_MAJOR, APP_VERSION_MINOR, APP_VERSION_PATCH);
    Serial.println("========================================");

    // Display first, so every later failure has somewhere to be reported.
    if (!DisplayHAL::init()) {
        Serial.println("FATAL: display failed to initialize");
        for (;;) {
            delay(1000);
        }
    }

    if (!TouchHAL::init()) {
        fatal("Touch");
    }

    StateManager::init();

    if (!UIManager::init()) {
        fatal("UI");
    }

    if (!StorageHAL::init()) {
        Serial.println("WARNING: storage unavailable - logging disabled");
        UIManager::show_notification("Storage unavailable", 3000);
    }

    const uint32_t now = millis();

    if (!can_bus.init(DEFAULT_CAN_BAUD)) {
        fatal("CAN controller");
    }

    can_service.init(now);
    can_service.set_bitrate(static_cast<uint32_t>(DEFAULT_CAN_BAUD));

    if (can_bus.start()) {
        can_service.start(now);          // <- the call this project never made
        StateManager::set_can_state(CANBusState::RUNNING);
        Serial.println("CAN bus started");
    } else {
        StateManager::set_can_state(CANBusState::FAILED);
        UIManager::show_notification("CAN bus failed to start", 5000);
        Serial.println("WARNING: CAN bus failed to start");
    }

    StateManager::set_can_baudrate(DEFAULT_CAN_BAUD);

    Serial.println("Initialization complete");
}

void loop() {
    const uint32_t now = millis();

    lv_timer_handler();

    can_service.update(now);

    StateManager::set_message_count(can_service.sniffer().get_message_count());
    StateManager::set_bus_load(can_service.stats().bus_load_percent());
    StateManager::set_error_count(can_bus.get_stats().error_count);
    StateManager::set_can_state(can_service.bus_state());

    UIManager::update();

    delay(SYSTEM_TICK_MS);
}
```

- [ ] **Step 2: Add `DisplayHAL::show_fatal` (defect #14)**

`main.cpp` previously spun in `while(1);` at four points, which on a watchdog-enabled board means a silent reboot loop with no indication of what failed. This renders the failure with raw M5GFX, so it works even when LVGL is not up.

In `src/hal/display_hal.h`, add to the public section:

```cpp
    /**
     * Draw a fatal initialization error directly with M5GFX.
     * Deliberately bypasses LVGL: this must work when LVGL is not running.
     */
    static void show_fatal(const char* subsystem);
```

In `src/hal/display_hal.cpp`, add:

```cpp
void DisplayHAL::show_fatal(const char* subsystem) {
    display.setRotation(1);
    display.fillScreen(TFT_BLACK);
    display.setTextColor(TFT_RED, TFT_BLACK);
    display.setTextSize(3);
    display.setCursor(40, 60);
    display.print("STARTUP FAILED");

    display.setTextColor(TFT_WHITE, TFT_BLACK);
    display.setTextSize(2);
    display.setCursor(40, 140);
    display.print(subsystem);
    display.setCursor(40, 180);
    display.print("could not be initialized.");
    display.setCursor(40, 240);
    display.print("Check wiring and power, then reset.");
}
```

- [ ] **Step 3: Extend `StateManager` for the new bus states**

`StateManager::set_can_state` already takes a `CANBusState`; the enum gained `BUS_OFF`, `RECOVERING`, and `FAILED` in Task 3. Check `src/ui/widgets/status_bar.cpp` and `src/ui/ui_manager.cpp:96` for comparisons against the old `CANBusState::ERROR`:

```bash
grep -rn "CANBusState::ERROR" src/
```

Replace each with the appropriate new state. `ui_manager.cpp:96` reads `state.can_state == CANBusState::RUNNING` and needs no change.

- [ ] **Step 4: Build for the device**

Run: `pio run -e esp32p4_pioarduino`
Expected: compiles. This is the first green device build since Task 6.

If the linker complains about `CANHAL`, a stale include survives — `grep -rn "can_hal" src/` and remove it.

- [ ] **Step 5: Confirm the host tests are unaffected**

Run: `pio test -e native`
Expected: all seven suites PASS.

- [ ] **Step 6: Commit**

```bash
git add -A
git commit -m "feat: wire the CAN pipeline into main

main.cpp called init() on all four core modules and start() on none, so
CANSniffer::update() returned immediately on every iteration and the entire
feature set was unreachable. main.cpp is now a composition root: construct
the bus, construct the service around it, update once per loop.

Replaces four while(1) spins with an on-screen failure report.

Co-Authored-By: Claude Opus 5 (1M context) <noreply@anthropic.com>"
```

---

## Task 14: HARDWARE GATE — first real capture

**This task requires a physical M5Stack Tab5 and a live CAN bus. It cannot be completed by code review.**

**Files:** none — this is verification. Any fix it uncovers gets its own commit.

- [ ] **Step 1: Flash the device**

```bash
pio run -e esp32p4_pioarduino --target upload
pio device monitor
```

Expected serial output: the banner, `Display HAL initialized`, `Touch HAL initialized`, `UI Manager initialized`, `CAN initialized at 500000 baud`, `CAN bus started`, `Initialization complete`.

- [ ] **Step 2: Connect to a live bus**

Wire CAN H and CAN L from `CAN_TX_PIN` (GPIO 17) / `CAN_RX_PIN` (GPIO 18) through the transceiver to the bus. Confirm 120 Ω termination at both ends. Confirm the bus is actually running at 500 kbit/s — a mismatched bit rate presents exactly as a dead bus.

- [ ] **Step 3: Verify the capture gate**

Watch the dashboard status bar and confirm all four:

1. The message counter climbs continuously with real traffic.
2. Bus load shows a non-zero percentage that tracks activity — it rises when traffic increases and falls when it stops. It must **not** read a constant 0.0 (that was defect #12) and must not peg at 100.
3. CAN status reads connected.
4. `pio device monitor` shows no watchdog resets over five minutes of continuous traffic.

- [ ] **Step 4: Verify transmit against a second node**

Using a second CAN node or analyzer on the same bus, confirm frames sent by PocketCAN arrive with the correct ID, DLC, and payload — and that **no unexpected single-shot or self-received frames appear**. That absence is the proof that defect #2 is fixed.

- [ ] **Step 5: Verify bus-off recovery**

Short CAN H to CAN L briefly to force the controller bus-off. Expected: the status bar reports the error, then returns to RUNNING within about a second of restoring the wiring — **without a reboot**. If it reaches FAILED after five attempts, that is also correct behaviour; verify it recovers after a reset.

- [ ] **Step 6: Tune the frame budget**

With the busiest traffic available, watch for UI stutter. If the interface feels unresponsive, lower `MAX_FRAMES_PER_TICK` in `src/config/can_config.h`. If the RX queue overflows (frames missing from the count), raise it. Record the final value and the bus conditions it was tuned under in the commit message.

- [ ] **Step 7: Commit the tuning and record the gate**

```bash
git add -A
git commit -m "tune: set MAX_FRAMES_PER_TICK from hardware measurement

A4 hardware gate passed: Tab5 on a live 500kbit/s bus captures frames,
reports real bus load, transmits correctly to a second node, and recovers
from a forced bus-off without a reboot.

Co-Authored-By: Claude Opus 5 (1M context) <noreply@anthropic.com>"
```

**A4 GATE:** all six checks above pass on real hardware. **Do not start Phase A5 until they do** — A5 hardens a UI whose data source must first be known good.

---

# Phase A5 — UI robustness

## Task 15: Screen deletion use-after-free (defects #4, #21, #22)

`ui_manager.cpp:77-80` calls `lv_obj_del(old_screen)` from inside `navigate_to()`, which is reached from `screen_main.cpp:133` `sniffer_tile_cb` — an event callback owned by a button that lives on the screen being deleted. LVGL then returns into freed memory. This is the crash that happens the first time a user taps a tile twice.

**Files:**
- Modify: `src/ui/ui_manager.cpp`

**Interfaces:**
- Consumes: LVGL 8.3 `lv_obj_del_async`.
- Produces: no signature changes.

- [ ] **Step 1: Replace the synchronous delete**

In `src/ui/ui_manager.cpp`, replace lines 77-80:

```cpp
    if (old_screen != nullptr && old_screen != lv_scr_act())
    {
        // Async: navigate_to() is reached from an event callback owned by a
        // button on old_screen. Deleting it synchronously frees the object
        // LVGL is about to return into. lv_obj_del_async defers the free to
        // the end of the current timer handler pass.
        lv_obj_del_async(old_screen);
    }
```

- [ ] **Step 2: Remove the debug leftover**

Delete `Serial.println("5");` at `ui_manager.cpp:63`.

- [ ] **Step 3: Wire the splash screen**

`create_splash_screen()` at `ui_manager.cpp:140` has no callers, so `ScreenSplash` is dead code — but the screen is finished and `UIManager` already carries `splash_start_time` and `splash_shown` for exactly this purpose.

In `UIManager::init()`, replace `create_main_dashboard(); splash_shown = false;` with:

```cpp
    create_splash_screen();
    splash_start_time = millis();
    splash_shown = true;
```

At the top of `UIManager::update()`, add the dismissal:

```cpp
    if (splash_shown) {
        if (millis() - splash_start_time >= 2000) {
            splash_shown = false;
            navigate_to(Screen::MAIN_DASHBOARD);
        }
        return;   // nothing else to update while the splash is up
    }
```

`create_splash_screen()` currently sets `active_screen_obj` but never calls `lv_scr_load`. Add it:

```cpp
void UIManager::create_splash_screen()
{
    active_screen_obj = ScreenSplash::create(0);   // 0 = manual dismiss
    lv_scr_load(active_screen_obj);
}
```

- [ ] **Step 4: Build**

Run: `pio run -e esp32p4_pioarduino`
Expected: compiles.

- [ ] **Step 5: Commit**

```bash
git add -A
git commit -m "fix(ui): delete screens asynchronously

navigate_to() ran lv_obj_del() on the screen that owned the button
dispatching the current event, so LVGL returned into freed memory. Also
wires the orphaned splash screen and drops a debug println.

Co-Authored-By: Claude Opus 5 (1M context) <noreply@anthropic.com>"
```

---

## Task 16: DMA race and refresh mode (defects #5, #16)

`display_hal.cpp:51-52` calls `lv_disp_flush_ready()` immediately after `pushImageDMA()`, while DMA is still reading the buffer. LVGL is then free to draw into memory the display controller is mid-transfer from. The draw buffer is also a single full-screen buffer (921,600 px × 2 B = 1.84 MB) with `sw_rotate = 1` but `full_refresh` never set.

**Files:**
- Modify: `src/hal/display_hal.cpp`

**Interfaces:**
- Consumes: M5GFX `startWrite`, `waitDMA`, `endWrite`.
- Produces: no signature changes.

- [ ] **Step 1: Fix the flush callback**

Replace `DisplayHAL::flush_cb` (`display_hal.cpp:48-53`):

```cpp
void DisplayHAL::flush_cb(lv_disp_drv_t *disp, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);

    display.startWrite();
    display.pushImageDMA(area->x1, area->y1, w, h, (uint16_t*)&color_p->full);
    display.waitDMA();      // DMA is still reading color_p; LVGL must not
    display.endWrite();     // be told the buffer is reusable before this.
    lv_disp_flush_ready(disp);
}
```

- [ ] **Step 2: Match the driver to the buffer that is actually allocated**

In `DisplayHAL::init()`, after `disp_drv.rotated = LV_DISP_ROT_90;`, add:

```cpp
    // One full-screen buffer is allocated, so tell LVGL to use it as one.
    // Without this LVGL treats it as a large partial buffer and redraws
    // in bands, which with sw_rotate costs an extra rotation per band.
    disp_drv.full_refresh = 1;
```

- [ ] **Step 3: Build and flash**

```bash
pio run -e esp32p4_pioarduino --target upload
pio device monitor
```

Expected: the device boots, the splash appears, then the dashboard.

- [ ] **Step 4: Assess refresh rate**

Navigate between screens and watch for tearing or sluggishness. If the interface feels slower than 20 fps under continuous CAN traffic, revert `full_refresh` to 0 and switch to dual partial buffers using the already-defined-but-unused `LVGL_PARTIAL_BUF_SIZE` (`hardware_config.h:15`):

```cpp
    static lv_color_t* buf2 = (lv_color_t*)heap_caps_malloc(
        sizeof(lv_color_t) * LVGL_PARTIAL_BUF_SIZE,
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    lv_disp_draw_buf_init(&draw_buf, buf, buf2, LVGL_PARTIAL_BUF_SIZE);
```

Record which configuration you kept and why in the commit message.

- [ ] **Step 5: Commit**

```bash
git add -A
git commit -m "fix(display): wait for DMA before signalling flush complete

lv_disp_flush_ready() was called while pushImageDMA() was still reading the
buffer, leaving LVGL free to draw into memory mid-transfer. Also sets
full_refresh to match the single full-screen buffer already allocated.

Co-Authored-By: Claude Opus 5 (1M context) <noreply@anthropic.com>"
```

---

## Task 17: Touch coordinate transform (defect #6)

`touch_hal.cpp:22` uses `getTouchRaw()`, which bypasses rotation and calibration, and feeds panel coordinates straight to LVGL. The panel is 720×1280; after `LV_DISP_ROT_90` LVGL's screen is 1280×720. Touches land transposed.

**Files:**
- Modify: `src/hal/touch_hal.cpp`

**Interfaces:**
- Consumes: `DISPLAY_WIDTH` (720) from `hardware_config.h`.
- Produces: no signature changes.

- [ ] **Step 1: Apply the rotation transform**

Replace `TouchHAL::read_cb` (`touch_hal.cpp:19-31`):

```cpp
void TouchHAL::read_cb(lv_indev_drv_t *indev_driver, lv_indev_data_t *data) {
    lgfx::touch_point_t tp[3];
    M5GFX& display = DisplayHAL::get_display();
    uint8_t touchpad = display.getTouchRaw(tp, 3);

    if (touchpad == 0) {
        data->state = LV_INDEV_STATE_REL;
        return;
    }

    // The panel reports its own 720x1280 coordinates. LVGL's screen is
    // 1280x720 after LV_DISP_ROT_90, so raw coordinates land transposed.
    // Rotating content 90 degrees clockwise maps panel (x,y) to screen
    // (y, WIDTH-1-x).
    data->state   = LV_INDEV_STATE_PR;
    data->point.x = tp[0].y;
    data->point.y = (DISPLAY_WIDTH - 1) - tp[0].x;
}
```

- [ ] **Step 2: Build and flash**

```bash
pio run -e esp32p4_pioarduino --target upload
```

- [ ] **Step 3: Verify all four corners on hardware**

Tap each of the four dashboard tiles and the home button in the status bar. Each tap must activate the control directly under your finger.

If taps land **mirrored** — top-left activating bottom-left, or left/right swapped — the panel's origin runs the other way. Use the counter-clockwise mapping instead and re-test:

```cpp
    data->point.x = (DISPLAY_HEIGHT - 1) - tp[0].y;
    data->point.y = tp[0].x;
```

Only one of these two is correct for this panel; the corner test decides it. Do not proceed until every corner registers accurately.

- [ ] **Step 4: Commit**

```bash
git add -A
git commit -m "fix(touch): transform panel coordinates into LVGL screen space

getTouchRaw() returns 720x1280 panel coordinates while LVGL's screen is
1280x720 after LV_DISP_ROT_90, so every touch landed transposed. Verified
against all four screen corners on hardware.

Co-Authored-By: Claude Opus 5 (1M context) <noreply@anthropic.com>"
```

---

## Task 18: Stop repainting the UI every millisecond (defect #15)

`main.cpp` runs `delay(SYSTEM_TICK_MS)` with `SYSTEM_TICK_MS = 1` (`hardware_config.h:27`) and calls `UIManager::update()` on every pass, so the status bar and dashboard are rewritten a thousand times a second to display values that change a few times a second. `lv_timer_handler()` still runs every tick — only content updates are gated.

**Files:**
- Modify: `src/ui/ui_manager.h`, `src/ui/ui_manager.cpp`

**Interfaces:**
- Consumes: `StateManager::get_state()`, `CanService` via `app_can_service()`.
- Produces: no signature changes; `UIManager::update()` becomes cheap to call every tick.

- [ ] **Step 1: Add the gate state**

In `src/ui/ui_manager.h`, add to the private section:

```cpp
    static uint32_t last_content_update;
    static uint32_t last_message_count;
    static CANBusState last_can_state;
```

In `src/ui/ui_manager.cpp`, add the definitions beside the other statics at the top:

```cpp
uint32_t UIManager::last_content_update = 0;
uint32_t UIManager::last_message_count = 0;
CANBusState UIManager::last_can_state = CANBusState::STOPPED;
```

- [ ] **Step 2: Gate the content refresh**

In `UIManager::update()`, immediately after the splash block added in Task 15, insert:

```cpp
    // The main loop runs at 1kHz. Widget content changes a few times a
    // second at most, so repaint on a real change or at 10Hz, whichever
    // comes first. lv_timer_handler() still runs every tick in loop().
    const ApplicationState& state = StateManager::get_state();
    const uint32_t now = millis();

    const bool changed = (state.message_count != last_message_count) ||
                         (state.can_state != last_can_state);
    const bool due = (now - last_content_update) >= 100;

    if (!changed && !due) {
        return;
    }

    last_content_update = now;
    last_message_count  = state.message_count;
    last_can_state      = state.can_state;
```

Then delete the now-redundant `const ApplicationState& state = StateManager::get_state();` line further down in the status-bar block (line 95 in the original), since `state` is already in scope.

- [ ] **Step 3: Build, flash, and confirm responsiveness is unchanged**

```bash
pio run -e esp32p4_pioarduino --target upload
```

Expected: the status bar still tracks the message counter smoothly; touch response is unchanged or better. The counter must still visibly climb — if it freezes, the gate is wrong.

- [ ] **Step 4: Commit**

```bash
git add -A
git commit -m "perf(ui): repaint on change or at 10Hz, not every millisecond

The 1kHz main loop rewrote every widget on every pass to show values that
change a few times a second. lv_timer_handler() still runs every tick.

Co-Authored-By: Claude Opus 5 (1M context) <noreply@anthropic.com>"
```

---

## Task 19: Correct the documentation

Three documents now describe a system that no longer exists, and two of them were wrong when they were written. `docs/MEMORY_MANAGEMENT.md` recommends the create/destroy pattern that caused defect #4 and states `lv_obj_del()` as a best practice. `README.md:219` claims 8 MB PSRAM while `MEMORY_MANAGEMENT.md:53-56` shows 32 MB from real hardware. `README.md` describes features as working that had never executed.

**Files:**
- Modify: `docs/MEMORY_MANAGEMENT.md`, `docs/UI_WIREFRAMES.md`, `README.md`, `docs/ARCHITECTURE.md`

**Interfaces:** none.

- [ ] **Step 1: Correct `docs/MEMORY_MANAGEMENT.md`**

In the "DO" list (around line 223), replace the `lv_obj_del()` recommendation with:

```markdown
1. **Use `lv_obj_del_async()` to delete screens**
   ```cpp
   if (old_screen && old_screen != lv_scr_act()) {
       lv_obj_del_async(old_screen);   // NOT lv_obj_del()
   }
   ```
   Navigation is triggered from an event callback owned by a widget on the
   screen being deleted. A synchronous delete frees the object LVGL is about
   to return into. This was a real crash, fixed in Phase A.
```

Update the `navigate_to` example at lines 10-25 to match the real implementation, and correct the "Current PocketCAN Approach" summary at the bottom.

- [ ] **Step 2: Correct the PSRAM figure in `README.md`**

Line 219 of the specifications table: change `512KB SRAM + 8MB PSRAM` to `768KB SRAM + 32MB PSRAM`, matching the hardware console output in `MEMORY_MANAGEMENT.md`.

- [ ] **Step 3: Mark feature status honestly in `README.md`**

The Features section lists capabilities as though they are complete. Add a status column or suffix to each:

```markdown
### Core Functionality
- **CAN Sniffing** - Real-time monitoring — working (no dedicated screen yet)
- **Message Filtering** - ID and mask-based filtering — working (no editor UI yet)
- **CAN Transmission** - Single-shot and periodic — working (no composer UI yet)
- **Device Emulation** - Programmable responses — working (no editor UI yet)
- **Adjustable Baud Rate** - 10K to 1M — working
- **Oscilloscope** - not started
```

Add a `## Testing` note that `pio test -e native` runs the host suite and that `pio test` without an environment will attempt to run on the device.

- [ ] **Step 4: Update `docs/UI_WIREFRAMES.md` implementation status**

The footer claims the Main Dashboard is complete and four screens are planned. That is still true after Phase A — confirm it reads accurately and add a line noting that the sniffer, transmit, emulator, and settings screens are sub-project B.

- [ ] **Step 5: Update `docs/ARCHITECTURE.md`**

Add `ICANBus`, `Esp32CanBus`, and `CanService` to the layer diagram, and note that time enters the system only through `CanService::update(now)`.

- [ ] **Step 6: Commit**

```bash
git add -A
git commit -m "docs: correct memory guide, PSRAM figure, and feature status

MEMORY_MANAGEMENT.md recommended the synchronous lv_obj_del() pattern that
caused the navigation use-after-free. README claimed 8MB PSRAM against 32MB
measured on hardware, and listed unreachable features as working.

Co-Authored-By: Claude Opus 5 (1M context) <noreply@anthropic.com>"
```

---

## Task 20: HARDWARE GATE — Phase A acceptance

**This task requires a physical M5Stack Tab5 and a live CAN bus.**

**Files:** none — this is verification.

- [ ] **Step 1: Clean build and flash the release environment**

```bash
pio run -e esp32p4_release --target clean
pio run -e esp32p4_release --target upload
pio device monitor
```

- [ ] **Step 2: Run the host suite from a clean checkout**

Run: `pio test -e native`
Expected: all seven suites PASS.

- [ ] **Step 3: Navigation stress**

Tap all four dashboard tiles and the home button in rapid succession for two minutes, with CAN traffic running throughout. Expected: no crash, no reboot, no memory growth. Watch `pio device monitor` for heap warnings or watchdog resets.

- [ ] **Step 4: Touch accuracy**

Confirm taps register accurately in all four screen corners and on the status-bar home button.

- [ ] **Step 5: Visual quality under load**

With continuous CAN traffic, confirm no tearing, no flicker, and a status bar that updates smoothly rather than in visible jumps.

- [ ] **Step 6: Work the Definition of Done from the spec (§10)**

Each of these must hold on real hardware:

1. Captures frames and reports an accurate count and bus load.
2. An allow-list filter actually excludes non-matching traffic. *(Set a rule programmatically for now — the filter editor is sub-project B.)*
3. Transmits single and periodic frames that a second node receives correctly.
4. Responds to a trigger ID with an emulated reply within its configured delay.
5. Recovers from a bus-off event without a reboot.
6. Survives sustained navigation and sustained traffic without crashing.
7. Passes `pio test -e native` from a clean checkout.

- [ ] **Step 7: Tag the milestone**

```bash
git tag -a phase-a-complete -m "Phase A: foundation complete

PocketCAN captures, filters, transmits, and emulates on a real CAN bus.
Core logic covered by seven host test suites. All 24 audited defects fixed.

Placeholder screens behind three dashboard tiles are expected; they are
sub-project B."
git log --oneline phase-a-complete
```

**A5 GATE:** every check above passes on hardware. Phase A is complete.

---

# Self-review

**Spec coverage.** Every section of the spec maps to a task:

| Spec section | Task(s) |
|---|---|
| §4.1 the seam | 6 |
| §4.2 implementation and orchestrator | 9, 11, 13 |
| §4.3 time | 6, 7, 8, 11 |
| §4.4 baud rate timing | 4, 9 |
| §4.5 file layout | 3, 4, 6, 9, 10, 11 |
| §5 data flow and frame budget | 11 |
| §5 bus load | 10 |
| §6 bus-off recovery | 9 (mechanism), 12 (policy) |
| §6 initialization failure | 13 |
| §6 frame integrity | 9 |
| §6 history buffer allocation | 3 |
| §7 defects #1–#24 | 1 (#8, #23), 2 (#17, #18, #19, #20), 3 (#15 partial), 4 (#9, #10), 5 (#3), 7 (#7), 9 (#2, #11, #13), 10 (#12), 11 (#1), 13 (#1, #14), 15 (#4, #21, #22), 16 (#5, #16), 17 (#6), 18 (#15), 19 (docs) |
| §8 host tests | 3, 4, 5, 6, 7, 10, 11, 12 |
| §9 phases and gates | A1 gate (Task 2), A2 gate (Task 9), A3 gate (Task 12), A4 gate (Task 14), A5 gate (Task 20) |
| §10 definition of done | 20 |

Defect #24 (`load_profile`/`save_profile`) is handled in Task 7 Step 3 by marking the stubs as deferred to sub-project C rather than leaving them looking unfinished.

**Placeholder scan.** No "TBD", "implement later", or "similar to Task N". Two decisions are deliberately left to hardware measurement — the `full_refresh` fallback (Task 16 Step 4) and the touch rotation direction (Task 17 Step 3) — and both give the exact alternative code plus the observation that selects it. That is a decision procedure, not a placeholder.

**Type consistency.** Verified across tasks: `can_timing_for()` returns `CanTimingSpec{id, nominal_bps}` (Task 4) and is consumed by `Esp32CanBus::timing_config_for(CanTimingId)` (Task 9). `CANTransmitter(ICANBus&)` / `init(uint32_t)` / `update(uint32_t)` (Task 6) match `CanService`'s member construction and calls (Task 11). `DeviceEmulator(ICANBus&)` / `process_message(msg, now)` / `update(now)` (Task 7) likewise. `CANSniffer::init(now)` / `record(msg)` / `update_stats(now)` / `clear(now)` (Task 8) match Task 11's usage. `CanStatsCollector::record_rx/record_tx/update/reset/bus_load_percent` (Task 10) match Task 11 and Task 13. `CANBusState` gains `BUS_OFF`/`RECOVERING`/`FAILED` in Task 3 and is used in Tasks 11, 12, 13, 18.

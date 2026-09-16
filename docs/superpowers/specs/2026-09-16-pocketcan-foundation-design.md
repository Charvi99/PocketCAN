# PocketCAN — Phase A: Foundation

**Date:** 2026-09-16
**Status:** Approved design, ready for implementation planning
**Scope:** Sub-project A of four. A is a prerequisite for all the others.

## 1. Context

PocketCAN is a pocket CAN bus analyzer on M5Stack Tab5 (ESP32-P4, 32 MB PSRAM,
7" 720x1280 MIPI-DSI touchscreen), built with PlatformIO, Arduino framework, and
LVGL 8.3. One commit exists. An audit on 2026-09-16 found a well-structured but
hollow codebase: 2,747 lines of C++ across a clean four-layer architecture, in
which every core module is written and individually plausible, and none of them
is connected to anything.

The decisive evidence is that `CANSniffer::start()`, `DeviceEmulator::start()`,
`DeviceEmulator::process_message()`, `CANFilter::check_message()`,
`CANTransmitter::add_periodic()`, and `CANHAL::set_baudrate()` have **zero call
sites** anywhere in the repository. `main.cpp` constructs four core objects,
calls `init()` on each, and never calls `start()` on any of them — so
`CANSniffer::update()` returns immediately at `can_sniffer.cpp:28` on every
iteration, forever. The device boots, renders its dashboard, and does nothing.

That boot is real: `docs/MEMORY_MANAGEMENT.md:53-56` contains console output from
actual hardware (`SPIRAM Total Size: 33554432 B`). Display and LVGL work. The gap
is composition and correctness, not bring-up.

Assessed at roughly 20% functionally complete, 0% working end-to-end.

### Sub-project decomposition

| | Sub-project | Depends on |
|---|---|---|
| **A** | **Foundation** — this document | — |
| B | Analyzer UI — sniffer table, transmit composer, filter and emulator editors, settings | A |
| C | Persistence — SDMMC, CSV export, emulation profiles, durable settings | A |
| D | Oscilloscope — clean-sheet design; on-chip ADC vs. external STM unresolved | A |

Each gets its own spec, plan, and implementation cycle. D is deferred
indefinitely; nothing beyond a pin define and three unused macros exists for it.

## 2. Goals and non-goals

**Phase A delivers** a device that, on a real CAN bus, captures frames, applies
filters, transmits single and periodic frames, emulates device responses, and
reports honest statistics on its dashboard — with the core logic covered by
host-run unit tests and a reproducible build.

**Phase A does not deliver** any new screen. Three of the four dashboard tiles
still lead to placeholder labels at the end of A. Persistence, CSV export, and
the oscilloscope are out of scope.

## 3. Settled decisions

1. **Target:** sub-projects A + B + C. Oscilloscope deferred to its own project.
2. **Hardware:** a Tab5 and a live CAN source are available, so every phase ends
   in a hardware verification gate rather than a green checkmark.
3. **Testability:** inject an `ICANBus` abstract base class so core logic is
   host-testable. This is a **compile-time C++ abstraction only**. The shipped
   firmware is 100% standalone, talks directly to the TWAI peripheral, and never
   depends on a computer at runtime. The runtime cost is one vtable lookup per
   frame. The test binary is a separate artifact that is never flashed.
4. **LVGL:** stay on 8.3. Delete the stray LVGL v9.4.0 template at the repo root
   (`lv_conf.h`, 1477 lines); `include/lv_conf.h` becomes the single source of
   truth. Both currently define `LV_CONF_H`, so which one wins depends on include
   order — an accident waiting to happen on any toolchain change.
5. **Composition:** a `CanService` orchestrator owns the bus and the four core
   modules and exposes one `update()` to the main loop.

## 4. Architecture

### 4.1 The seam

The interface covers bus I/O only. Lifecycle (`init`, `start`, `stop`,
`set_baudrate`) stays on the concrete class, so the test fake stays trivial.

```cpp
// src/core/i_can_bus.h
class ICANBus {
public:
    virtual ~ICANBus() = default;
    virtual bool     transmit(const CANMessage& msg) = 0;
    virtual bool     receive(CANMessage& msg)        = 0;
    virtual CANStats get_stats() const               = 0;
    virtual bool     is_bus_error() const            = 0;
    virtual bool     recover()                       = 0;
};
```

`CANMessage` and `CANStats` move from `src/hal/can_hal.h` to `src/core/can_types.h`
so that core code and host tests do not transitively include `driver/twai.h`.

### 4.2 Implementation and orchestrator

`src/hal/can_hal.*` becomes `src/hal/esp32_can_bus.*`: the same TWAI code,
instance methods instead of statics, implementing `ICANBus`. No second layer of
indirection is introduced.

```cpp
// src/services/can_service.h
class CanService {
public:
    CanService(ICANBus& bus);
    bool init(CANBaudRate baud);
    bool start();
    void update();                       // called once per main-loop iteration

    CANFilter&      filter();            // accessors for the UI layer
    CANTransmitter& transmitter();
    DeviceEmulator& emulator();
    const RingBuffer<CANMessage>& history() const;

private:
    ICANBus& bus;
    CANFilter filter_;
    CANTransmitter transmitter_;
    DeviceEmulator emulator_;
    RingBuffer<CANMessage> history_;
    CanStatsCollector stats_;
};
```

`main.cpp` reduces to a composition root: construct `Esp32CanBus`, construct
`CanService` around it, initialize the HALs and the UI, then loop over
`lv_timer_handler()`, `service.update()`, `StateManager` push, `UIManager::update()`.

### 4.3 Time

`CanService::update()` calls `millis()` exactly once per iteration and passes the
value down: `transmitter_.update(now)`, `emulator_.update(now)`. One time source,
and host tests control time with a plain integer. No `IClock` interface is
introduced — the parameter is sufficient and cheaper.

### 4.4 Baud rate timing

The baud table currently returns `twai_timing_config_t`, an ESP-IDF type, which
makes it impossible to test on a host. It splits:

- `src/core/can_timing.h` — pure C++: `CANBaudRate -> CanTiming{brp, tseg_1,
  tseg_2, sjw, triple_sampling}`
- `Esp32CanBus` converts `CanTiming` to `twai_timing_config_t` in one place

This turns two real defects (see §7) into a table-driven unit test.

### 4.5 File layout

```
src/core/      can_types.h*  i_can_bus.h*  can_timing.h*
               can_sniffer   can_filter    can_transmitter  device_emulator
src/hal/       esp32_can_bus* (from can_hal)  display_hal  touch_hal  storage_hal
src/services/  can_service*  can_stats*  state_manager
src/ui/        ui_manager  screens/  widgets/  themes/
src/utils/     ring_buffer  hex_utils  time_utils
test/native/   fake_can_bus.h*  test_*.cpp*
```

`*` = new in Phase A.

## 5. Data flow

The receive path is bounded so that a saturated bus cannot starve the UI:

```
TWAI RX FIFO
  -> Esp32CanBus::receive()
  -> [budget loop: at most MAX_FRAMES_PER_TICK frames per update()]
       -> CANFilter::check_message()      (reject -> next frame)
       -> RingBuffer<CANMessage>::push()  (capture history for the sniffer table)
       -> DeviceEmulator::process_message() (schedule any triggered response)
       -> CanStatsCollector::record_rx()  (count, and accumulate bus-load bits)
  -> CANTransmitter::update(now)          (due periodic frames)
  -> DeviceEmulator::update(now)          (due emulated responses)
  -> StateManager push                    (counts, bus load, bus state)
```

`MAX_FRAMES_PER_TICK` is a new constant in `can_config.h`. At 1 Mbit/s a fully
loaded bus delivers roughly 8,000 frames per second; the budget bounds the worst
case per iteration so `lv_timer_handler()` is never starved. Initial value 32,
tuned on hardware at the A4 gate.

`CANSniffer` keeps its role as the capture-control module (run/pause/clear and
history ownership), driven by `CanService` rather than by nobody.

### Bus load

Currently hardcoded: `stats.bus_load_percent` is only ever assigned `0.0f`
(`can_hal.cpp:189`). It becomes a real calculation. Per frame, accumulate the
approximate bit count — `47 + 8 * dlc` for standard frames, `67 + 8 * dlc` for
extended — multiplied by 1.1 to approximate bit stuffing. Over a one-second
sliding window, divide accumulated bits by the configured bit rate. Error frames
are not counted; the figure is an estimate and is labeled as such in the UI.

## 6. Error handling

**Bus-off recovery.** `CANBusState::ERROR` is currently a one-way door: nothing
in the repository calls `twai_initiate_recovery()`. It becomes a real state
machine inside `Esp32CanBus`:

```
RUNNING --(bus-off detected)--> BUS_OFF
BUS_OFF --(recover() + settle delay)--> RECOVERING
RECOVERING --(driver reports running)--> RUNNING
RECOVERING --(retry limit exceeded)--> FAILED  (reported to the user, no further retries)
```

The retry limit is 5 attempts with a 100 ms settle delay. `FAILED` surfaces on
the status bar as a persistent error rather than a silent dead bus.

**Initialization failure.** `main.cpp` currently spins in `while(1);` at four
separate points, which on a watchdog-enabled board means a silent reboot loop
with no indication of what failed. Initialization order becomes: display first,
then everything else, so any subsequent failure renders a readable on-screen
error naming the failed subsystem. Only a display failure itself falls back to
serial output, and even then it reports rather than spins.

**Frame integrity.** `twai_message_t twai_msg;` at `can_hal.cpp:125` is an
uninitialized stack variable whose flag bits (`ss`, `self`, `dlc_non_comp`) are
then transmitted as garbage — a single-shot or self-reception frame sent by
accident, depending on stack contents. It becomes `twai_message_t twai_msg = {};`.
On the receive side, `CANMessage::data[dlc..7]` is zeroed so the sniffer table
never displays stack residue as bus data.

**History buffer allocation.** `RingBuffer` uses `new T[capacity]`, which places
1,000 `CANMessage` objects (~24 KB) on the internal heap. The history buffer is
allocated from PSRAM instead. `RingBuffer`'s header comment claims it is
"thread-safe"; it is not, and the comment is corrected rather than the code —
all access is from the main loop by design.

## 7. Defect inventory

Every item below was confirmed in the audit and is fixed within Phase A.

### Critical

| # | Location | Defect |
|---|---|---|
| 1 | `main.cpp` | Nothing is ever started. `init()` is called on all four core modules; `start()` on none. The entire feature set is unreachable. |
| 2 | `can_hal.cpp:125` | Uninitialized `twai_message_t` on transmit; flag bits sent as stack garbage. |
| 3 | `can_filter.cpp:108` | `check_message()` falls through to `return true` when no rule matches, so an allow-list passes 100% of traffic — the exact inverse of its purpose, and contradicted by its own comment at line 102. |
| 4 | `ui_manager.cpp:77-80` | Use-after-free: `lv_obj_del(old_screen)` runs while that screen still owns the button dispatching the current event (reached from `screen_main.cpp:133`). Fix: `lv_obj_del_async()`. |
| 5 | `display_hal.cpp:51-52` | `lv_disp_flush_ready()` is called immediately after `pushImageDMA()`, while DMA is still reading the buffer. Fix: `waitDMA()` before signaling, or signal from the DMA completion callback. |
| 6 | `touch_hal.cpp:22` | `getTouchRaw()` bypasses rotation and calibration. LVGL's coordinate space is 1280x720 after `LV_DISP_ROT_90`; the panel reports 720x1280. Touch coordinates are transposed. |
| 7 | `device_emulator.cpp:94` | `send_time = millis() + delay_ms` compared with `now >= send_time` — breaks at the 49.7-day `millis()` wrap. `can_transmitter.cpp:65` already does this correctly by subtraction; the emulator does not. |
| 8 | root `lv_conf.h` | An LVGL v9.4.0 template shadows the working v8.3 config in `include/lv_conf.h`. Both define `LV_CONF_H`. Delete it. |

### Significant

| # | Location | Defect |
|---|---|---|
| 9 | `can_hal.cpp:27` | `BAUD_20K` returns `TWAI_TIMING_CONFIG_25KBITS()` — a 25% bit-rate error, which on a real bus means silent total failure to sync. |
| 10 | `can_hal.cpp:29` | `BAUD_10K` is absent from the switch and silently falls through to the 500K default. |
| 11 | `can_hal.cpp:179` | `stats.bus_off_count = status.bus_error_count` conflates two distinct counters. |
| 12 | `can_hal.cpp:189` | `bus_load_percent` is never computed. |
| 13 | repo-wide | `twai_initiate_recovery()` is never called; bus-off is terminal. |
| 14 | `main.cpp` | `while(1);` on init failure, in four places. |
| 15 | `main.cpp:120`, `hardware_config.h:27` | `delay(SYSTEM_TICK_MS)` of 1 ms drives a full UI update every millisecond. Screen updates become change-gated; the dashboard refreshes at a fixed interval, not every tick. |
| 16 | `display_hal.cpp:28` | A single full-screen draw buffer (921,600 px x 2 B = 1.84 MB) with `sw_rotate = 1` but `full_refresh` never set. Set `full_refresh = 1`, matching the buffer that is already allocated. If the A5 gate measures a refresh rate below 20 fps under continuous traffic, fall back to dual partial buffers of `LVGL_PARTIAL_BUF_SIZE` (already defined, currently unused). |
| 17 | `include/lv_conf.h:88-108` | All Montserrat fonts 12 through 48 are enabled, costing roughly 1 MB of flash. Trim to the four sizes the UI actually uses: 20, 24, 32, 48. |
| 18 | `platformio.ini:33` | `board_build.partitions = default.csv` gives ~1.3 MB for the application; the current build is close to or over that. Move to a large-app or custom partition table. |
| 19 | `platformio.ini:30` | `build_src_filter` excludes `lvgl_example/`, a directory that does not exist. |
| 20 | `platformio.ini:13,17` | `build_type = debug` with `CORE_DEBUG_LEVEL=5` is the committed default. Keep debug available, make an optimized release environment the default for flashing. |
| 21 | `ui_manager.cpp:63` | `Serial.println("5");` debug leftover. |
| 22 | `ui_manager.cpp:140` | `create_splash_screen()` has no callers, so `ScreenSplash` is dead code. Wire it: `UIManager` already carries `splash_start_time` and `splash_shown` for exactly this, and the screen itself is finished. Show it at boot, dismiss on timeout. |
| 23 | repo root | No `.gitignore` (PlatformIO's `.pio/` is untracked only by luck) and no `LICENSE` file, despite the README's MIT badge. |
| 24 | `device_emulator.cpp:133-142` | `load_profile()` / `save_profile()` are `TODO` stubs returning `false`. They stay stubs in Phase A and are implemented in sub-project C; the stubs are marked explicitly as deferred rather than looking unfinished. |

### Documentation corrections

`docs/MEMORY_MANAGEMENT.md` recommends the create/destroy screen pattern that
causes defect #4, and quotes 32 MB of PSRAM while `README.md` says 8 MB. The
README also describes features as working that have never executed.
Documentation is corrected at the end of Phase A, when the true state is known,
not before.

## 8. Testing

### Host tests

`pio test -e native` runs Unity under `platform = native`, compiling only
`src/core/`, `src/utils/`, and `src/services/` — no Arduino, no TWAI, no LVGL.

```cpp
// test/native/fake_can_bus.h
class FakeCanBus : public ICANBus {
public:
    std::deque<CANMessage>  rx_queue;   // frames the test feeds in
    std::vector<CANMessage> sent;       // frames the code under test emitted
    CANStats stats{};
    bool bus_error = false;
    bool recover_called = false;

    bool transmit(const CANMessage& m) override { sent.push_back(m); return true; }
    bool receive(CANMessage& m) override;       // pop from rx_queue, or false
    CANStats get_stats() const override { return stats; }
    bool is_bus_error() const override { return bus_error; }
    bool recover() override { recover_called = true; bus_error = false; return true; }
};
```

Each test below is written against a specific defect from §7 and must fail before
the fix and pass after it:

| Test | Guards defect |
|---|---|
| `filter_accept_list_rejects_unmatched` | #3 |
| `filter_reject_list_blocks_matched` | #3 (the complementary direction) |
| `emulator_fires_across_millis_wrap` | #7 — `now` near `UINT32_MAX` |
| `transmitter_periodic_interval_and_wrap` | confirms the correct pattern stays correct |
| `ring_buffer_wraps_and_reports_size` | overwrite semantics at capacity |
| `ring_buffer_push_returns_documented_value` | `push()` always returns `true`; its doc comment claims `false` when full |
| `can_timing_table_is_complete_and_distinct` | #9, #10 — all nine rates present, correct, distinct |
| `can_service_respects_frame_budget` | a flooded fake bus yields after `MAX_FRAMES_PER_TICK` |
| `can_service_routes_filtered_frames_to_history_and_emulator` | the pipeline of §5 |
| `bus_load_matches_known_traffic` | #12 — a known frame sequence produces a known percentage |

### What host tests deliberately do not cover

The TWAI driver, LVGL, the display pipeline, and touch. These have no meaningful
host representation, and a mock of them would test the mock. They are verified on
the device, which is why every phase ends with a hardware gate.

## 9. Phases

Each phase is independently committable and ends at a gate that must pass before
the next begins.

### A1 — Build hygiene

Delete root `lv_conf.h` (#8). Trim Montserrat fonts to 20/24/32/48 (#17). Move to
a large-app partition table (#18). Remove the dead `build_src_filter` line (#19).
Add an optimized default environment, keeping debug available (#20). Add
`.gitignore` and `LICENSE` (#23).

**Gate:** `pio run` completes clean; flash and RAM usage are recorded, with the
application comfortably inside its partition.

### A2 — Test harness

Add `env:native`. Extract `can_types.h` and `can_timing.h`. Write `FakeCanBus`
and the seven tests from §8 that exercise code which exists today — the two
filter tests, the emulator wrap test, the transmitter test, both ring buffer
tests, and the timing table test — **failing first**, against the code as it
stands.

The three remaining tests (`can_service_respects_frame_budget`,
`can_service_routes_filtered_frames_to_history_and_emulator`,
`bus_load_matches_known_traffic`) cannot compile until `CanService` and
`CanStatsCollector` exist, so they are written test-first inside A3 as those
classes are built.

**Gate:** `pio test -e native` runs, and the seven tests fail for the expected
reasons. The failures are the audit, reproduced as executable assertions.

### A3 — CAN core

Introduce `ICANBus`. Convert `can_hal` into `Esp32CanBus`. Build `CanService` and
`CanStatsCollector`. Fix defects #2, #3, #7, #9, #10, #11, #12, #13.

**Gate:** all host tests green; the firmware still compiles for the board.

### A4 — Composition

Reduce `main.cpp` to a composition root. Wire `CanService` into `StateManager`.
Replace `while(1);` with on-screen error reporting (#14). Move the history buffer
to PSRAM. Tune `MAX_FRAMES_PER_TICK` against real traffic.

**Gate — the decisive one:** a Tab5 attached to a live CAN bus shows its dashboard
message counter climbing with real traffic, a bus load figure that tracks actual
activity, and a CAN status that reads RUNNING. Disconnecting and shorting the bus
drives it to an error state and recovering the wiring returns it to RUNNING
without a reboot.

### A5 — UI robustness

Fix defects #4, #5, #6, #15, #16, #21, #22. Change-gate screen updates. Correct
the documentation as described in §7.

**Gate:** navigating all four tiles and back, repeatedly and rapidly, never
crashes; touch registers accurately in all four corners of the screen; no tearing
or flicker under continuous CAN traffic.

## 10. Definition of done

Phase A is complete when a Tab5 with this firmware, connected to a real CAN bus:

1. Captures frames and reports an accurate count and bus load.
2. Applies an allow-list filter that actually excludes non-matching traffic.
3. Transmits single and periodic frames that a second node receives correctly.
4. Responds to a trigger ID with an emulated reply within its configured delay.
5. Recovers from a bus-off event without a reboot.
6. Survives sustained navigation and sustained traffic without crashing.
7. Passes `pio test -e native` from a clean checkout.

Placeholder screens behind three tiles are expected and acceptable at this point.
They are sub-project B.

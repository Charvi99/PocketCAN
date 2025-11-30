# PocketCAN Architecture Documentation

## System Overview

PocketCAN is built using a clean, layered architecture that separates concerns and promotes maintainability.

## Architecture Diagram

```
┌───────────────────────────────────────────────────────────────┐
│                    USER INTERFACE                              │
│  ┌──────────────────────────────────────────────────────────┐ │
│  │                  LVGL Screens                             │ │
│  │  ┌─────────┐  ┌─────────┐  ┌──────────┐  ┌──────────┐  │ │
│  │  │ Main    │  │ Sniffer │  │ Transmit │  │ Settings │  │ │
│  │  │Dashboard│  │ Screen  │  │  Screen  │  │  Screen  │  │ │
│  │  └─────────┘  └─────────┘  └──────────┘  └──────────┘  │ │
│  │                                                           │ │
│  │  ┌──────────────────────────────────────────────────┐   │ │
│  │  │            Reusable Widgets                       │   │ │
│  │  │  • Message List  • Status Bar  • Keypad          │   │ │
│  │  └──────────────────────────────────────────────────┘   │ │
│  └──────────────────────────────────────────────────────────┘ │
│                            ↕                                   │
│                    UIManager (Navigation)                      │
└───────────────────────────────────────────────────────────────┘
                             ↕
┌───────────────────────────────────────────────────────────────┐
│                    SERVICES LAYER                              │
│  ┌──────────────────────────────────────────────────────────┐ │
│  │  StateManager: Centralized Application State             │ │
│  │   • Current Mode  • CAN Status  • Statistics             │ │
│  └──────────────────────────────────────────────────────────┘ │
└───────────────────────────────────────────────────────────────┘
                             ↕
┌───────────────────────────────────────────────────────────────┐
│                  CORE BUSINESS LOGIC                           │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐        │
│  │ CANSniffer   │  │  CANFilter   │  │CANTransmitter│        │
│  │ • Capture    │  │ • Rules      │  │ • Single     │        │
│  │ • Buffer     │  │ • Masks      │  │ • Periodic   │        │
│  │ • Stats      │  │ • Accept/Rej │  │ • Queue      │        │
│  └──────────────┘  └──────────────┘  └──────────────┘        │
│                                                                 │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐        │
│  │DeviceEmulator│  │  DataLogger  │  │ProtocolParser│        │
│  │ • Triggers   │  │ • SD/Flash   │  │ • DBC Files  │        │
│  │ • Responses  │  │ • CSV Export │  │ • Decode     │        │
│  │ • Profiles   │  │ • Auto-save  │  │ • Format     │        │
│  └──────────────┘  └──────────────┘  └──────────────┘        │
└───────────────────────────────────────────────────────────────┘
                             ↕
┌───────────────────────────────────────────────────────────────┐
│            HARDWARE ABSTRACTION LAYER (HAL)                    │
│  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐        │
│  │ DisplayHAL   │  │  TouchHAL    │  │   CANHAL     │        │
│  │ • M5GFX      │  │ • Input      │  │ • TWAI       │        │
│  │ • LVGL       │  │ • Gestures   │  │ • TX/RX      │        │
│  │ • Brightness │  │ • Calibrate  │  │ • Errors     │        │
│  └──────────────┘  └──────────────┘  └──────────────┘        │
│                                                                 │
│  ┌──────────────┐                                              │
│  │ StorageHAL   │                                              │
│  │ • SPIFFS     │                                              │
│  │ • SD Card    │                                              │
│  └──────────────┘                                              │
└───────────────────────────────────────────────────────────────┘
                             ↕
┌───────────────────────────────────────────────────────────────┐
│                        HARDWARE                                │
│  • ESP32-P4 MCU  • 7" Display  • CAN Transceiver              │
│  • SD Card Slot  • Touch Panel • PSRAM                        │
└───────────────────────────────────────────────────────────────┘
```

## Component Relationships

### Data Flow: CAN Message Reception

```
1. CAN Bus Physical Layer
   ↓
2. CAN Transceiver (e.g., MCP2551)
   ↓
3. ESP32 TWAI Controller (Hardware)
   ↓
4. CANHAL::receive()
   • Reads from TWAI FIFO
   • Converts to CANMessage struct
   • Returns to caller
   ↓
5. CANSniffer::update()
   • Polls CANHAL for new messages
   • Applies filters
   • Stores in ring buffer
   • Updates statistics
   • Triggers callback
   ↓
6. UI Update
   • Callback notifies UI
   • Message added to list widget
   • Statistics updated
```

### Data Flow: CAN Message Transmission

```
1. User Input (UI Screen)
   ↓
2. CANTransmitter::send()
   • Validates message
   • Adds to queue (if periodic)
   ↓
3. CANHAL::transmit()
   • Formats for TWAI
   • Writes to TX FIFO
   • Updates stats
   ↓
4. ESP32 TWAI Controller
   ↓
5. CAN Bus
```

## Module Descriptions

### HAL Layer

#### DisplayHAL
- **Purpose**: Abstracts M5Stack Tab5 display hardware
- **Dependencies**: M5GFX, LVGL
- **Key Functions**:
  - `init()`: Initialize display and LVGL
  - `flush_cb()`: LVGL flush callback (DMA transfer)
  - `set_brightness()`: Display brightness control

#### TouchHAL
- **Purpose**: Handles touchscreen input
- **Dependencies**: M5GFX, LVGL
- **Key Functions**:
  - `init()`: Register LVGL input device
  - `read_cb()`: LVGL touch read callback

#### CANHAL
- **Purpose**: ESP32 TWAI (CAN) controller interface
- **Dependencies**: ESP-IDF TWAI driver
- **Key Functions**:
  - `init()`: Configure and install TWAI driver
  - `start()/stop()`: Control CAN bus
  - `transmit()/receive()`: Message I/O
  - `set_baudrate()`: Change baud rate
  - `get_stats()`: Statistics

#### StorageHAL
- **Purpose**: File system abstraction
- **Dependencies**: SPIFFS, SD library
- **Key Functions**:
  - `init()`: Mount filesystems
  - `get_fs()`: Get filesystem reference
  - `get_free_space()`: Check available space

### Core Layer

#### CANSniffer
- **Purpose**: Capture and buffer CAN messages
- **Key Features**:
  - Ring buffer (1000 messages)
  - Message rate calculation
  - Callback system
- **Usage**:
  ```cpp
  sniffer.start();
  sniffer.on_message_received([](const CANMessage& msg) {
      // Handle new message
  });
  ```

#### CANFilter
- **Purpose**: Filter messages by ID and mask
- **Key Features**:
  - Up to 8 simultaneous rules
  - Accept/reject logic
  - Enable/disable individual filters
- **Usage**:
  ```cpp
  FilterRule rule = {
      .id = 0x123,
      .mask = 0x7FF,
      .enabled = true,
      .accept = true
  };
  filter.add_rule(rule);
  ```

#### CANTransmitter
- **Purpose**: Send CAN messages
- **Key Features**:
  - Single-shot transmission
  - Periodic transmission (up to 10 concurrent)
  - Configurable intervals
- **Usage**:
  ```cpp
  CANMessage msg = {
      .id = 0x123,
      .data = {0x01, 0x02, 0x03},
      .dlc = 3,
      .type = CANFrameType::STANDARD
  };
  transmitter.send(msg);

  // Or periodic:
  transmitter.add_periodic(msg, 100); // Every 100ms
  ```

#### DeviceEmulator
- **Purpose**: Simulate CAN devices
- **Key Features**:
  - Trigger-response rules
  - Configurable delays
  - Profile save/load
- **Usage**:
  ```cpp
  EmulationRule rule = {
      .trigger_id = 0x7DF,  // OBD-II request
      .response = {...},     // Response message
      .delay_ms = 10,
      .enabled = true
  };
  emulator.add_rule(rule);
  emulator.start();
  ```

### Services Layer

#### StateManager
- **Purpose**: Centralized application state
- **Key Features**:
  - Single source of truth
  - Thread-safe access
  - State change notifications
- **State Includes**:
  - Current mode (Sniffer/Transmit/Emulator)
  - CAN bus status
  - Message counts
  - Error counts
  - Bus load percentage

### UI Layer

#### UIManager
- **Purpose**: Screen management and navigation
- **Key Features**:
  - Screen lifecycle management
  - Smooth transitions
  - Back stack
- **Usage**:
  ```cpp
  UIManager::navigate_to(Screen::SNIFFER);
  UIManager::show_notification("Message sent!");
  ```

## Utilities

### RingBuffer<T>
Template-based circular buffer with overflow protection.

### HexUtils
Hex conversion and formatting utilities.

### TimeUtils
Timestamp formatting and relative time calculations.

## Configuration

### hardware_config.h
Hardware-specific settings:
- GPIO pin assignments
- Display resolution
- Buffer sizes

### can_config.h
CAN bus configuration:
- Supported baud rates
- Buffer sizes
- Filter limits

### app_config.h
Application-level settings:
- UI animation timing
- Theme selection
- Logging intervals

## Memory Management

### PSRAM Usage
- LVGL display buffer: ~7MB (full framebuffer)
- Allocated in `heap_caps_malloc()` with SPIRAM flag

### Ring Buffers
- CAN RX buffer: 1000 messages × ~20 bytes = 20KB
- Automatic overflow handling (oldest overwritten)

### Stack Considerations
- LVGL tasks require adequate stack
- Adjust in `platformio.ini` if needed

## Build System

### PlatformIO Configuration
```ini
platform = espressif32
framework = arduino
lib_deps = LVGL, M5GFX, M5Unified
build_flags = -std=c++17
```

### Dependencies
- **LVGL 8.3.11**: UI framework
- **M5GFX**: Display driver
- **M5Unified**: M5Stack hardware support
- **ESP-IDF**: CAN (TWAI) driver

## Error Handling

### Strategy
- Return `bool` for success/failure
- Log errors to Serial
- Update StateManager with error states
- UI shows error notifications

### CAN Bus Errors
- Bus-off detection
- Error counter monitoring
- Automatic recovery attempts

## Performance Considerations

### CAN Message Rate
- Theoretical max: ~8000 msg/sec @ 1Mbps
- Ring buffer prevents overflow
- Filter reduces processing load

### UI Refresh
- LVGL updates at ~30 FPS
- Message list uses virtual scrolling
- Only visible items rendered

## Security Considerations

### File System
- Validate file paths
- Check available space before write
- Handle SD card removal gracefully

### CAN Bus
- No automatic message relay (prevent flooding)
- Filter validation before apply
- Rate limiting on transmission

## Testing Strategy

### Unit Tests
- Core modules (Sniffer, Filter, Transmitter)
- Ring buffer edge cases
- Hex conversion utilities

### Integration Tests
- HAL layer with mock hardware
- Full message flow
- State management

### Hardware Tests
- CAN bus communication
- Touch input accuracy
- Display performance

---

**Last Updated**: [Current Date]
**Version**: 1.0.0

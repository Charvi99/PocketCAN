# PocketCAN Project Structure

## Overview
Professional embedded CAN bus analyzer with clean separation between backend (business logic) and frontend (LVGL UI).

## Directory Structure

```
PocketCAN/
├── src/
│   ├── main.cpp                    # Main entry point
│   ├── config/
│   │   ├── hardware_config.h       # Hardware pin definitions
│   │   ├── can_config.h            # CAN bus configurations
│   │   └── app_config.h            # Application settings
│   │
│   ├── hal/                        # Hardware Abstraction Layer
│   │   ├── display_hal.h/cpp       # Display initialization & control
│   │   ├── touch_hal.h/cpp         # Touch input handling
│   │   ├── can_hal.h/cpp           # CAN bus hardware interface
│   │   └── storage_hal.h/cpp       # SD card / flash storage
│   │
│   ├── core/                       # Core Business Logic (Backend)
│   │   ├── can_sniffer.h/cpp       # CAN message capture & logging
│   │   ├── can_filter.h/cpp        # Message filtering & masking
│   │   ├── can_transmitter.h/cpp   # CAN message transmission
│   │   ├── device_emulator.h/cpp   # Device emulation profiles
│   │   ├── oscilloscope.h/cpp      # Signal analysis & waveform capture
│   │   ├── data_logger.h/cpp       # Data logging to storage
│   │   └── protocol_parser.h/cpp   # CAN protocol decoding (DBC support)
│   │
│   ├── services/                   # Application Services
│   │   ├── state_manager.h/cpp     # Application state management
│   │   ├── event_bus.h/cpp         # Internal event system
│   │   └── settings_manager.h/cpp  # Persistent settings
│   │
│   ├── ui/                         # Frontend (LVGL UI)
│   │   ├── ui_manager.h/cpp        # UI initialization & screen management
│   │   ├── screens/
│   │   │   ├── screen_main.h/cpp       # Main dashboard
│   │   │   ├── screen_sniffer.h/cpp    # CAN sniffer view
│   │   │   ├── screen_transmit.h/cpp   # Message transmit interface
│   │   │   ├── screen_emulator.h/cpp   # Device emulator
│   │   │   ├── screen_scope.h/cpp      # Oscilloscope view
│   │   │   ├── screen_filter.h/cpp     # Filter configuration
│   │   │   └── screen_settings.h/cpp   # Settings screen
│   │   │
│   │   ├── widgets/
│   │   │   ├── can_message_list.h/cpp  # Reusable CAN message list widget
│   │   │   ├── waveform_chart.h/cpp    # Oscilloscope waveform widget
│   │   │   ├── status_bar.h/cpp        # Status bar component
│   │   │   └── keypad.h/cpp            # Numeric/hex keypad
│   │   │
│   │   ├── themes/
│   │   │   ├── theme_dark.h/cpp        # Dark theme
│   │   │   └── theme_colors.h          # Color definitions
│   │   │
│   │   └── assets/
│   │       ├── fonts/                  # Custom fonts
│   │       └── icons/                  # UI icons
│   │
│   └── utils/
│       ├── ring_buffer.h/cpp       # Circular buffer for CAN messages
│       ├── hex_utils.h/cpp         # Hex conversion utilities
│       └── time_utils.h/cpp        # Timestamp utilities
│
├── lib/                            # External libraries (managed by PlatformIO)
│   ├── LVGL/
│   ├── M5Unified/
│   └── M5GFX/
│
├── test/                           # Unit tests
│   ├── test_can_filter/
│   ├── test_ring_buffer/
│   └── test_protocol_parser/
│
├── data/                           # Data files to upload to device
│   ├── emulation_profiles/         # Device emulation presets
│   └── dbc_files/                  # CAN database files
│
├── docs/
│   ├── ARCHITECTURE.md
│   ├── API.md
│   └── USER_GUIDE.md
│
├── platformio.ini                  # PlatformIO configuration
└── README.md
```

## Architecture Principles

### 1. Separation of Concerns
- **HAL Layer**: Hardware-specific code, easy to port to different platforms
- **Core Layer**: Pure business logic, independent of UI
- **UI Layer**: LVGL-based interface, communicates with core via clean API

### 2. Communication Pattern
```
UI Layer (LVGL)
    ↕ (calls functions)
Core Services
    ↕ (uses)
HAL Layer
    ↕ (controls)
Hardware
```

### 3. Event-Driven Architecture
- Core modules publish events (e.g., "CAN message received")
- UI subscribes to events and updates display
- Decouples UI from business logic

### 4. State Management
- Centralized state manager
- UI reads state, core modifies state
- Single source of truth

## Module Responsibilities

### Core Modules (Backend)

#### CAN Sniffer
- Capture CAN messages from bus
- Store in ring buffer
- Filter based on active rules
- Publish events for new messages

#### CAN Filter
- Define filter rules (ID, data pattern, rate)
- Apply masks
- Pass/block messages

#### CAN Transmitter
- Send single messages
- Periodic transmission
- Support for different CAN modes (Standard/Extended)

#### Device Emulator
- Load emulation profiles
- Respond to specific CAN messages
- Simulate complete ECU behavior

#### Oscilloscope
- Capture CAN H/L voltage levels
- Trigger on conditions
- Waveform storage

### UI Modules (Frontend)

#### Screen Manager
- Navigate between screens
- Smooth transitions
- Back stack management

#### Widgets
- Reusable components
- Self-contained logic
- Event callbacks to core

## Data Flow Example: CAN Sniffing

1. **Hardware**: CAN controller receives message → HAL callback
2. **HAL**: `can_hal.cpp` captures raw data → calls `can_sniffer.process_message()`
3. **Core**: `can_sniffer.cpp` applies filters → stores in ring buffer → publishes event
4. **Service**: Event bus notifies subscribers
5. **UI**: `screen_sniffer.cpp` receives event → updates LVGL table widget

## Coding Standards

- Use C++17 features where beneficial
- Header guards: `#pragma once`
- Naming:
  - Classes: `PascalCase`
  - Functions: `snake_case()`
  - Constants: `UPPER_SNAKE_CASE`
- Comments: Doxygen style for public APIs
- Error handling: Return error codes, avoid exceptions (embedded context)

## Build Configuration

### PlatformIO Environments
- `env:debug` - Debug build with logging
- `env:release` - Optimized release build
- `env:test` - Unit test environment

### Compiler Flags
- `-std=c++17`
- `-Wall -Wextra` - Enable warnings
- `-Os` - Optimize for size (release)

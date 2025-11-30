# PocketCAN

Professional pocket-sized CAN bus analyzer for automotive diagnostics, testing, and development.

![Version](https://img.shields.io/badge/version-1.0.0-blue)
![Platform](https://img.shields.io/badge/platform-ESP32--P4-green)
![License](https://img.shields.io/badge/license-MIT-brightgreen)

## Features

### Core Functionality
- **CAN Sniffing** - Real-time CAN bus monitoring with high-speed capture
- **Message Filtering** - Advanced ID and mask-based filtering
- **CAN Transmission** - Single-shot and periodic message transmission
- **Device Emulation** - Simulate CAN devices with programmable responses
- **Adjustable Baud Rate** - Support for 10K to 1M baud rates
- **Oscilloscope** - Voltage level monitoring on CAN lines (planned)

### Hardware
- **Platform**: M5Stack Tab5 (ESP32-P4)
- **Display**: 7" 720x1280 MIPI-DSI touchscreen
- **CAN Controller**: Built-in ESP32 TWAI (CAN 2.0)
- **Storage**: Internal flash + SD card support

## Architecture

PocketCAN follows a clean, layered architecture:

```
┌─────────────────────────────────────────┐
│         UI Layer (LVGL)                  │
│  ┌──────────┬──────────┬──────────┐    │
│  │ Screens  │ Widgets  │  Themes  │    │
│  └──────────┴──────────┴──────────┘    │
├─────────────────────────────────────────┤
│         Services Layer                   │
│  ┌─────────────┬──────────────────┐    │
│  │ StateManager│ Event Bus        │    │
│  └─────────────┴──────────────────┘    │
├─────────────────────────────────────────┤
│         Core Business Logic              │
│  ┌──────────┬──────────┬──────────┐    │
│  │ Sniffer  │ Filter   │Transmitter│   │
│  │ Emulator │ Logger   │  Parser  │    │
│  └──────────┴──────────┴──────────┘    │
├─────────────────────────────────────────┤
│    Hardware Abstraction Layer (HAL)     │
│  ┌──────────┬──────────┬──────────┐    │
│  │ Display  │  Touch   │   CAN    │    │
│  │ Storage  │          │          │    │
│  └──────────┴──────────┴──────────┘    │
└─────────────────────────────────────────┘
```

### Key Design Principles

1. **Separation of Concerns** - Clean separation between UI, business logic, and hardware
2. **Modularity** - Independent, testable components
3. **Event-Driven** - Reactive architecture for real-time updates
4. **Hardware Abstraction** - Easy to port to different platforms

## Project Structure

```
PocketCAN/
├── src/
│   ├── config/              # Configuration headers
│   │   ├── hardware_config.h
│   │   ├── can_config.h
│   │   └── app_config.h
│   ├── hal/                 # Hardware Abstraction Layer
│   │   ├── display_hal.h/cpp
│   │   ├── touch_hal.h/cpp
│   │   ├── can_hal.h/cpp
│   │   └── storage_hal.h/cpp
│   ├── core/                # Core Business Logic
│   │   ├── can_sniffer.h/cpp
│   │   ├── can_filter.h/cpp
│   │   ├── can_transmitter.h/cpp
│   │   └── device_emulator.h/cpp
│   ├── services/            # Application Services
│   │   └── state_manager.h/cpp
│   ├── ui/                  # User Interface
│   │   ├── ui_manager.h/cpp
│   │   ├── screens/
│   │   ├── widgets/
│   │   └── themes/
│   ├── utils/               # Utilities
│   │   ├── ring_buffer.h
│   │   ├── hex_utils.h
│   │   └── time_utils.h
│   └── main.cpp            # Entry point
├── data/                   # Data files
│   ├── emulation_profiles/
│   └── dbc_files/
├── test/                   # Unit tests
├── docs/                   # Documentation
├── platformio.ini
└── README.md
```

## Getting Started

### Prerequisites

- [PlatformIO](https://platformio.org/) IDE or CLI
- M5Stack Tab5 device
- USB cable for programming

### Building

```bash
# Clone repository
git clone <repository-url>
cd PocketCAN

# Build project
pio run

# Upload to device
pio run --target upload

# Monitor serial output
pio device monitor
```

### Configuration

#### CAN Bus Settings

Edit `src/config/can_config.h`:
```cpp
#define DEFAULT_CAN_BAUD    CANBaudRate::BAUD_500K
#define DEFAULT_FRAME_TYPE  CANFrameType::STANDARD
```

#### Hardware Pin Configuration

Edit `src/config/hardware_config.h`:
```cpp
#define CAN_TX_PIN  GPIO_NUM_17
#define CAN_RX_PIN  GPIO_NUM_18
```

## Usage

### CAN Sniffing

1. Connect CAN H/L lines to your CAN bus
2. Navigate to **Sniffer** screen
3. Select baud rate
4. Press **Start** to begin capturing

### Message Filtering

1. Navigate to **Filter** screen
2. Add filter rules by ID and mask
3. Enable/disable filters as needed
4. Filters apply to all modes

### CAN Transmission

1. Navigate to **Transmit** screen
2. Enter CAN ID and data
3. Choose single-shot or periodic transmission
4. Press **Send**

### Device Emulation

1. Navigate to **Emulator** screen
2. Load emulation profile or create custom rules
3. Define trigger IDs and responses
4. Start emulation

## Development

### Adding New Features

1. **Backend Logic**: Add module to `src/core/`
2. **UI Screen**: Add screen to `src/ui/screens/`
3. **Configuration**: Update relevant config header

### Coding Standards

- C++17
- Header guards: `#pragma once`
- Naming:
  - Classes: `PascalCase`
  - Functions: `snake_case()`
  - Constants: `UPPER_SNAKE_CASE`

### Testing

```bash
# Run unit tests
pio test
```

## Features Roadmap

- [ ] DBC file support for protocol decoding
- [ ] Data logging to SD card / CSV export
- [ ] USB CAN interface mode (act as USB-CAN adapter)
- [ ] WiFi connectivity for remote monitoring
- [ ] OBD-II protocol support
- [ ] CAN FD support
- [ ] Oscilloscope functionality

## Technical Specifications

| Feature | Specification |
|---------|--------------|
| CAN Protocols | CAN 2.0A/B |
| Baud Rates | 10K - 1M baud |
| Frame Types | Standard (11-bit), Extended (29-bit) |
| Buffer Size | 1000 messages |
| Display | 720x1280, 16-bit color |
| CPU | ESP32-P4 dual-core |
| Memory | 512KB SRAM + 8MB PSRAM |

## Contributing

Contributions are welcome! Please:
1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Submit a pull request

## License

This project is licensed under the MIT License.

## Acknowledgments

- M5Stack for the Tab5 hardware
- LVGL for the UI framework
- ESP-IDF for the CAN (TWAI) driver

## Support

For issues, questions, or feature requests, please open an issue on GitHub.

---

**Made with ❤️ for the automotive and embedded community**

# PocketCAN UI Wireframes

## Main Dashboard Screen (Implemented)

```
┌────────────────────────────────────────────────────────────┐
│  12:45    [🔌] CAN: 500K    [🔋] 85%                       │  ← Status Bar
├────────────────────────────────────────────────────────────┤
│                                                             │
│  [☰]                                                        │  ← Hamburger Menu
│                                                             │
│                      PocketCAN                              │  ← App Title
│                                                             │
│                                                             │
│  ┌─────────────────────┐    ┌─────────────────────┐       │
│  │                     │    │                     │       │
│  │         👁          │    │         ⬆          │       │
│  │                     │    │                     │       │
│  │    CAN Sniffer      │    │      Transmit       │       │
│  │   Monitor bus       │    │   Send messages     │       │
│  │                     │    │                     │       │
│  └─────────────────────┘    └─────────────────────┘       │
│                                                             │
│  ┌─────────────────────┐    ┌─────────────────────┐       │
│  │                     │    │                     │       │
│  │         🔀          │    │         ⚙          │       │
│  │                     │    │                     │       │
│  │      Emulator       │    │      Settings       │       │
│  │  Device simulation  │    │   Configuration     │       │
│  │                     │    │                     │       │
│  └─────────────────────┘    └─────────────────────┘       │
│                                                             │
└────────────────────────────────────────────────────────────┘
```

### Features Implemented:

1. **Status Bar (Top)**
   - Time display (hours:minutes)
   - CAN status indicator with baud rate
   - Battery level with icon
   - Updates automatically every loop

2. **Hamburger Menu Button**
   - Top-left corner
   - Quick access to menu (ready for expansion)

3. **Navigation Grid (2x2)**
   - Large, touch-friendly tiles (300x200px each)
   - Icon + Title + Subtitle
   - Color-coded for quick recognition:
     - Sniffer: Cyan (#00BCD4)
     - Transmit: Orange (#FF5722)
     - Emulator: Green (#4CAF50)
     - Settings: Grey
   - Shadow effects for depth
   - Press animations

4. **Visual Design**
   - Dark theme background (#1a1a1a)
   - Modern card-based layout
   - 20px spacing between elements
   - Rounded corners (12px radius)
   - Professional shadows

## Color Scheme

### Status Indicators
- 🟢 **Green** (#4CAF50): Active/Success (CAN connected)
- 🟡 **Yellow** (#FFC107): Warning (Low battery)
- 🔴 **Red** (#F44336): Error (CAN error)
- ⚪ **Grey** (#B0B0B0): Inactive/Disabled

### Navigation Tiles
- **Cyan** (#00BCD4): Primary function (Sniffer)
- **Orange** (#FF5722): Action (Transmit)
- **Green** (#4CAF50): Simulation (Emulator)
- **Grey** (#606060): Settings

## Upcoming Screens (To Be Implemented)

### CAN Sniffer Screen
```
┌────────────────────────────────────────────────────────────┐
│  12:45    [🔌] CAN: 500K    [🔋] 85%                       │
├────────────────────────────────────────────────────────────┤
│  [←] CAN Sniffer                            [▶] [⏸] [🗑]   │
│                                                             │
│  Messages: 1,234    Rate: 45 msg/s    Errors: 0           │
│                                                             │
│  ┌─────────────────────────────────────────────────────┐  │
│  │ ID     │  Data            │ DLC │  Time            │  │
│  ├─────────────────────────────────────────────────────┤  │
│  │ 0x123  │ 01 02 03 04     │  4  │ 12:45:30.123     │  │
│  │ 0x456  │ FF EE DD CC BB  │  5  │ 12:45:30.145     │  │
│  │ 0x789  │ 11 22 33        │  3  │ 12:45:30.167     │  │
│  │ 0x123  │ 01 02 03 05     │  4  │ 12:45:30.189     │  │
│  │ ...                                                 │  │
│  └─────────────────────────────────────────────────────┘  │
│                                                             │
│  [Filter]  [Export]  [Statistics]                          │
└────────────────────────────────────────────────────────────┘
```

### Transmit Screen
```
┌────────────────────────────────────────────────────────────┐
│  12:45    [🔌] CAN: 500K    [🔋] 85%                       │
├────────────────────────────────────────────────────────────┤
│  [←] Transmit Message                                      │
│                                                             │
│  CAN ID:  [0x123        ▼]    ○ Standard  ● Extended      │
│                                                             │
│  Data Length: [4 ▼]                                        │
│                                                             │
│  Data:                                                      │
│  ┌──────────────────────────────────────────────────────┐ │
│  │  [01] [02] [03] [04] [ ] [ ] [ ] [ ]                 │ │
│  └──────────────────────────────────────────────────────┘ │
│                                                             │
│  ┌─────────────────────────────────┐                      │
│  │  1  2  3   A  B  C              │                      │
│  │  4  5  6   D  E  F              │  ← Hex Keypad       │
│  │  7  8  9   ⌫  0  ✓              │                      │
│  └─────────────────────────────────┘                      │
│                                                             │
│  ┌─────────────────┐  ┌─────────────────┐                │
│  │  Send Once      │  │  Send Periodic  │                │
│  └─────────────────┘  └─────────────────┘                │
│                                                             │
│  Interval: [100 ms ▼]    □ Enable                         │
└────────────────────────────────────────────────────────────┘
```

### Settings Screen
```
┌────────────────────────────────────────────────────────────┐
│  12:45    [🔌] CAN: 500K    [🔋] 85%                       │
├────────────────────────────────────────────────────────────┤
│  [←] Settings                                              │
│                                                             │
│  CAN Configuration                                          │
│  ├─ Baud Rate:    [500K ▼]                                │
│  ├─ Frame Type:   [Standard ▼]                            │
│  └─ Auto Start:   [✓]                                     │
│                                                             │
│  Display                                                    │
│  ├─ Brightness:   [████████░░] 80%                        │
│  ├─ Theme:        [● Dark  ○ Light]                       │
│  └─ Timeout:      [Never ▼]                               │
│                                                             │
│  Data Logging                                               │
│  ├─ Auto Save:    [✓]                                     │
│  ├─ Storage:      [SD Card ▼]                             │
│  └─ Format:       [CSV ▼]                                 │
│                                                             │
│  System                                                     │
│  ├─ Version:      1.0.0                                    │
│  ├─ Storage Used: 2.3 GB / 16 GB                          │
│  └─ [Factory Reset]                                        │
└────────────────────────────────────────────────────────────┘
```

## Touch Interaction Guidelines

### Button Sizes
- Minimum touch target: 50x50px
- Navigation tiles: 300x200px
- Action buttons: 120x50px
- Icons: 24px - 48px

### Feedback
- Visual press state (darker background)
- Haptic feedback (if hardware supports)
- Animation duration: 200ms
- Ripple effect on press

### Gestures
- Tap: Select/Activate
- Long press: Context menu
- Swipe: Navigate between screens
- Pinch: (reserved for future use)

## Accessibility

- High contrast text (white on dark)
- Large, readable fonts (14pt minimum)
- Clear visual hierarchy
- Color + icon indicators (not color alone)
- Touch-friendly spacing (20px minimum)

---

**Implementation Status**:
- ✅ Main Dashboard (Complete)
- ⏳ Sniffer Screen (Planned)
- ⏳ Transmit Screen (Planned)
- ⏳ Emulator Screen (Planned)
- ⏳ Settings Screen (Planned)

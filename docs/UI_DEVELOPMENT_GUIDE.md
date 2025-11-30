# PocketCAN UI Development Guide

## How the UI Architecture Works

### The Flow: From Power-On to Display

```
1. Device Powers On
   ↓
2. setup() in main.cpp runs
   ↓
3. DisplayHAL::init() - Initializes M5GFX and LVGL
   ↓
4. TouchHAL::init() - Sets up touch input
   ↓
5. UIManager::init() - Creates initial screen (Main Dashboard)
   ↓
6. loop() starts running
   ↓
7. Every loop iteration:
   - lv_timer_handler() - LVGL processes UI updates
   - UIManager::update() - Updates current screen
   - delay(1ms)
```

---

## Understanding the Loop Function

### What Happens in main.cpp loop()

```cpp
void loop() {
    // 1. LVGL processes all UI updates, animations, touch events
    lv_timer_handler();

    // 2. Update core modules (CAN sniffer, transmitter, etc.)
    sniffer.update();
    transmitter.update();
    emulator.update();

    // 3. Update application state
    StateManager::set_message_count(sniffer.get_message_count());
    // ... more state updates ...

    // 4. Update the UI based on new state
    UIManager::update();

    // 5. Small delay to prevent watchdog issues
    delay(SYSTEM_TICK_MS);  // 1ms
}
```

**Key Point**: This loop runs ~1000 times per second (every 1ms).

---

## UIManager - The Screen Controller

### What UIManager Does

The `UIManager` is responsible for:
1. **Creating screens** - Builds the UI for each screen
2. **Switching screens** - Handles navigation between screens
3. **Updating screens** - Refreshes dynamic content (time, stats, etc.)

### File Location
- Header: `src/ui/ui_manager.h`
- Implementation: `src/ui/ui_manager.cpp`

### Key Functions

#### 1. `UIManager::init()`
Called once during `setup()`. Creates the initial screen.

```cpp
bool UIManager::init() {
    create_main_dashboard();  // Create main screen
    return true;
}
```

#### 2. `UIManager::navigate_to(Screen screen)`
Switches to a different screen.

```cpp
// Example: Navigate to sniffer screen
UIManager::navigate_to(Screen::SNIFFER);
```

**What it does:**
- Deletes the old screen from memory
- Creates the new screen
- Loads it to the display

#### 3. `UIManager::update()`
Called every loop iteration. Updates the current screen.

```cpp
void UIManager::update() {
    // Only update if we're on the main dashboard
    if (active_screen_obj && current_screen == Screen::MAIN_DASHBOARD) {
        ScreenMain::update(active_screen_obj);
    }
}
```

**What it does:**
- Checks which screen is currently active
- Calls that screen's `update()` function
- The screen updates dynamic content (time, CAN stats, etc.)

---

## Screen Structure

Each screen is a separate class. Let's look at the Main Dashboard as an example.

### Main Dashboard Files
- Header: `src/ui/screens/screen_main.h`
- Implementation: `src/ui/screens/screen_main.cpp`

### Screen Lifecycle

```
1. create() - Called once when navigating to this screen
   ↓
2. Screen is displayed
   ↓
3. update() - Called every loop iteration (1000 times/sec)
   ↓
4. Screen is deleted when navigating away
```

### Example: ScreenMain::create()

This function builds the entire screen:

```cpp
lv_obj_t* ScreenMain::create() {
    // 1. Create the screen container
    lv_obj_t* screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(screen, lv_color_hex(THEME_COLOR_BACKGROUND), 0);

    // 2. Create status bar at top
    status_bar = StatusBar::create(screen);

    // 3. Create home button
    lv_obj_t* menu_btn = create_menu_button(screen);

    // 4. Create content area
    lv_obj_t* content = lv_obj_create(screen);
    // ... set size, position, style ...

    // 5. Create title
    lv_obj_t* title = lv_label_create(content);
    lv_label_set_text(title, "PocketCAN");
    // ... set font, color ...

    // 6. Create navigation tiles
    lv_obj_t* grid = create_nav_grid(content);

    return screen;  // Return the screen object
}
```

**Key Points:**
- `screen` is the main container
- All UI elements are children of this screen
- Elements are positioned using `lv_obj_align()`
- Returns the screen object so UIManager can display it

### Example: ScreenMain::update()

This function updates dynamic content:

```cpp
void ScreenMain::update(lv_obj_t* screen) {
    if (status_bar) {
        // Update time display
        StatusBar::update(status_bar);

        // Update CAN status (green if running, grey if stopped)
        const ApplicationState& state = StateManager::get_state();
        bool can_connected = (state.can_state == CANBusState::RUNNING);
        StatusBar::set_can_status(status_bar, can_connected,
                                   static_cast<uint32_t>(state.can_baudrate));

        // Update battery level
        StatusBar::set_battery(status_bar, 85, false);
    }
}
```

**What happens:**
- Reads current state from `StateManager`
- Updates widgets (status bar) with new values
- This runs 1000 times/sec, but LVGL only redraws when values change

---

## Widgets - Reusable UI Components

Widgets are reusable UI components like status bars, buttons, lists, etc.

### Example: StatusBar Widget

**Location**: `src/ui/widgets/status_bar.h` and `.cpp`

### Widget Structure

```cpp
class StatusBar {
public:
    // Create the widget
    static lv_obj_t* create(lv_obj_t* parent);

    // Update the widget
    static void update(lv_obj_t* bar);

    // Setter functions to change widget content
    static void set_battery(lv_obj_t* bar, uint8_t percent, bool charging);
    static void set_can_status(lv_obj_t* bar, bool connected, uint32_t baudrate);
    static void set_time(lv_obj_t* bar, uint8_t hours, uint8_t minutes);
};
```

### How StatusBar::create() Works

```cpp
lv_obj_t* StatusBar::create(lv_obj_t* parent) {
    // 1. Create container for status bar
    lv_obj_t* bar = lv_obj_create(parent);
    lv_obj_set_size(bar, LV_PCT(100), STATUS_BAR_HEIGHT);  // Full width, 40px height
    lv_obj_align(bar, LV_ALIGN_TOP_MID, 0, 0);            // Top of screen

    // 2. Create time label (left side)
    lv_obj_t* time_label = lv_label_create(bar);
    lv_label_set_text(time_label, "--:--");
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_24, 0);

    // 3. Create CAN status label (center)
    lv_obj_t* can_status = lv_label_create(bar);
    lv_label_set_text(can_status, LV_SYMBOL_USB " CAN: --");

    // 4. Create battery label (right side)
    lv_obj_t* battery_label = lv_label_create(bar);
    lv_label_set_text(battery_label, LV_SYMBOL_BATTERY_FULL " --%");

    // 5. Use flex layout to position labels
    lv_obj_set_flex_flow(bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(bar, LV_FLEX_ALIGN_SPACE_BETWEEN, ...);

    return bar;
}
```

### Widget Child Access

Widgets store their child elements in order. We access them by index:

```cpp
static constexpr int IDX_TIME = 0;        // First child
static constexpr int IDX_CAN_STATUS = 1;  // Second child
static constexpr int IDX_BATTERY = 2;     // Third child

void StatusBar::set_time(lv_obj_t* bar, uint8_t hours, uint8_t minutes) {
    lv_obj_t* time_label = lv_obj_get_child(bar, IDX_TIME);  // Get first child
    lv_label_set_text_fmt(time_label, "%02d:%02d", hours, minutes);
}
```

---

## How to Edit the UI Yourself

### Example 1: Change Status Bar Colors

**File**: `src/ui/widgets/status_bar.cpp`

Find the `create()` function and change colors:

```cpp
// Change status bar background color
lv_obj_set_style_bg_color(bar, lv_color_hex(0x2d2d2d), 0);  // Dark grey
// Or use theme color:
lv_obj_set_style_bg_color(bar, lv_color_hex(THEME_COLOR_SURFACE), 0);

// Change text color
lv_obj_set_style_text_color(time_label, lv_color_hex(0xFF5722), 0);  // Orange
```

**Colors are in hex format**: `0xRRGGBB`
- `0xFF0000` = Red
- `0x00FF00` = Green
- `0x0000FF` = Blue
- `0xFFFFFF` = White
- `0x000000` = Black

### Example 2: Change Text Size

**File**: `src/ui/screens/screen_main.cpp`

Find where labels are created:

```cpp
// Make title bigger
lv_obj_t* title = lv_label_create(content);
lv_label_set_text(title, "PocketCAN");
lv_obj_set_style_text_font(title, &lv_font_montserrat_48, 0);  // Change to 48
```

**Available font sizes** (defined in lv_conf.h):
- `lv_font_montserrat_12` through `lv_font_montserrat_48`
- Defined in increments: 12, 14, 16, 18, 20, 22, 24, 26, 28, 30, 32, etc.

### Example 3: Change Navigation Tile Colors

**File**: `src/ui/screens/screen_main.cpp`

Find the `create_nav_grid()` function where tiles are created:

```cpp
// Sniffer tile - change from cyan to purple
lv_obj_t* tile1 = create_nav_tile(grid_cont, LV_SYMBOL_EYE_OPEN, "CAN Sniffer",
                                   "Monitor bus", 0x9C27B0, sniffer_tile_cb);
                                   //           ^^^^^^^^ Change this color
```

### Example 4: Add a New Label to Status Bar

**File**: `src/ui/widgets/status_bar.cpp`

In `StatusBar::create()`, add after the battery label:

```cpp
// 4. Create custom status label
lv_obj_t* custom_label = lv_label_create(bar);
lv_label_set_text(custom_label, "Ready");
lv_obj_set_style_text_font(custom_label, &lv_font_montserrat_20, 0);
lv_obj_set_style_text_color(custom_label, lv_color_hex(THEME_COLOR_SUCCESS), 0);
```

### Example 5: Change Tile Size

**File**: `src/ui/screens/screen_main.cpp`

In `create_nav_tile()` function:

```cpp
lv_obj_t* btn = lv_btn_create(parent);
lv_obj_set_size(btn, 300, 200);  // Change width and height (in pixels)
//                   ^^^  ^^^
//                   width height
```

---

## LVGL Basics You Need to Know

### 1. Creating Objects

```cpp
// Create a container
lv_obj_t* container = lv_obj_create(parent);

// Create a label
lv_obj_t* label = lv_label_create(parent);

// Create a button
lv_obj_t* button = lv_btn_create(parent);

// Create an image
lv_obj_t* img = lv_img_create(parent);
```

### 2. Positioning Objects

```cpp
// Align to center
lv_obj_align(obj, LV_ALIGN_CENTER, 0, 0);

// Align to top-left with offset
lv_obj_align(obj, LV_ALIGN_TOP_LEFT, 10, 20);
//                                   ^^  ^^
//                                   x   y offset

// Common alignments:
LV_ALIGN_TOP_LEFT       LV_ALIGN_TOP_MID        LV_ALIGN_TOP_RIGHT
LV_ALIGN_LEFT_MID       LV_ALIGN_CENTER         LV_ALIGN_RIGHT_MID
LV_ALIGN_BOTTOM_LEFT    LV_ALIGN_BOTTOM_MID     LV_ALIGN_BOTTOM_RIGHT
```

### 3. Sizing Objects

```cpp
// Set exact size
lv_obj_set_size(obj, 200, 100);  // width, height in pixels

// Set size as percentage of parent
lv_obj_set_size(obj, LV_PCT(50), LV_PCT(100));  // 50% width, 100% height

// Auto size based on content
lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
```

### 4. Styling Objects

```cpp
// Background color
lv_obj_set_style_bg_color(obj, lv_color_hex(0x2d2d2d), 0);

// Text color
lv_obj_set_style_text_color(obj, lv_color_hex(0xFFFFFF), 0);

// Border
lv_obj_set_style_border_width(obj, 2, 0);
lv_obj_set_style_border_color(obj, lv_color_hex(0xFF0000), 0);

// Rounded corners
lv_obj_set_style_radius(obj, 12, 0);  // 12px radius

// Padding (space inside)
lv_obj_set_style_pad_all(obj, 20, 0);  // 20px on all sides
lv_obj_set_style_pad_top(obj, 10, 0);  // 10px on top only
```

### 5. Event Callbacks (Button Clicks)

```cpp
// Add click handler
lv_obj_add_event_cb(button, button_clicked, LV_EVENT_CLICKED, NULL);

// The callback function:
static void button_clicked(lv_event_t* e) {
    Serial.println("Button was clicked!");
    // Do something...
}
```

---

## How State Flows Through the System

```
1. Hardware Event (e.g., CAN message received)
   ↓
2. HAL Layer captures it (CANHAL::receive())
   ↓
3. Core Module processes it (CANSniffer::update())
   ↓
4. State Manager updated (StateManager::set_message_count())
   ↓
5. UI reads state (StateManager::get_state())
   ↓
6. UI updates display (ScreenMain::update())
   ↓
7. LVGL redraws changed elements
```

### Example: Showing CAN Message Count

**In loop (main.cpp):**
```cpp
// Update state from core module
StateManager::set_message_count(sniffer.get_message_count());
```

**In UI update (screen_main.cpp):**
```cpp
// Read state and display it
const ApplicationState& state = StateManager::get_state();
lv_label_set_text_fmt(msg_count_label, "Messages: %d", state.message_count);
```

---

## Common UI Tasks

### Task 1: Add a New Screen

1. **Create files**: `src/ui/screens/screen_yourname.h` and `.cpp`

2. **Define the class** (in .h):
```cpp
class ScreenYourName {
public:
    static lv_obj_t* create();
    static void update(lv_obj_t* screen);
};
```

3. **Implement create()** (in .cpp):
```cpp
lv_obj_t* ScreenYourName::create() {
    lv_obj_t* screen = lv_obj_create(NULL);
    // Build your UI here...
    return screen;
}
```

4. **Add to UIManager enum** (`ui_manager.h`):
```cpp
enum class Screen {
    MAIN_DASHBOARD,
    SNIFFER,
    YOUR_SCREEN,  // Add this
    // ...
};
```

5. **Add to navigation** (`ui_manager.cpp`):
```cpp
case Screen::YOUR_SCREEN:
    create_your_screen();
    break;
```

### Task 2: Add Dynamic Text that Updates

1. **Store the label as a variable**:
```cpp
static lv_obj_t* status_label = nullptr;
```

2. **Create it once**:
```cpp
status_label = lv_label_create(screen);
lv_label_set_text(status_label, "Initial text");
```

3. **Update it in update() function**:
```cpp
void ScreenMain::update(lv_obj_t* screen) {
    if (status_label) {
        lv_label_set_text_fmt(status_label, "Count: %d", some_value);
    }
}
```

### Task 3: Navigate Between Screens

In your button callback:

```cpp
static void settings_button_cb(lv_event_t* e) {
    UIManager::navigate_to(Screen::SETTINGS);
}
```

---

## Debugging Tips

### 1. Serial Output
Add debug prints to see what's happening:
```cpp
Serial.println("Creating main screen...");
Serial.printf("CAN messages: %d\n", count);
```

### 2. Check Null Pointers
Always check before accessing widgets:
```cpp
if (status_bar) {
    StatusBar::update(status_bar);
}
```

### 3. LVGL Object Tree
Print the object hierarchy for debugging:
```cpp
lv_obj_tree_walk(screen, LV_OBJ_TREE_WALK_NEXT, print_obj_info, NULL);
```

---

## Summary

**Key Concepts:**
1. **UIManager** controls which screen is shown
2. **Screens** are created once and contain UI elements
3. **update()** functions refresh dynamic content every loop
4. **Widgets** are reusable components
5. **StateManager** holds application data
6. **LVGL** is the graphics library that draws everything

**To Edit UI:**
1. Find the right screen file in `src/ui/screens/`
2. Modify `create()` to change layout
3. Modify `update()` to change dynamic content
4. Use LVGL functions to style and position elements
5. Compile and test!

**Next Steps:**
- Experiment with colors and sizes
- Add your own labels and buttons
- Create callback functions for button clicks
- Build new screens

---

Need help with a specific UI task? Just ask!

# Memory Management in PocketCAN UI

## How Screen Navigation Currently Works

### Current Approach: Create/Destroy Pattern

Every time you navigate to a screen:

```cpp
void UIManager::navigate_to(Screen screen) {
    current_screen = screen;

    // 1. DELETE the old screen (frees memory)
    if (active_screen_obj) {
        lv_obj_del(active_screen_obj);  // ← This frees ALL memory
        active_screen_obj = nullptr;
    }

    // 2. CREATE a new screen from scratch
    switch (screen) {
        case Screen::MAIN_DASHBOARD:
            create_main_dashboard();  // ← Allocates new memory
            break;
    }
}
```

### What Happens to Memory?

**When you delete a screen (`lv_obj_del()`):**
- ✅ LVGL automatically frees the screen object
- ✅ ALL child objects are deleted too (widgets, labels, buttons, etc.)
- ✅ Memory is returned to the heap
- ✅ No memory leaks (LVGL handles cleanup)

**When you create a screen:**
- New memory is allocated from PSRAM
- Widgets are created
- Styles are applied

### Is This a Problem?

**Short Answer: No, not for PocketCAN**

**Why it's okay:**
1. **Plenty of PSRAM**: You have 32MB, currently using only ~3.6MB
2. **LVGL is efficient**: Screen objects are relatively small (few KB each)
3. **Automatic cleanup**: `lv_obj_del()` handles all freeing
4. **No fragmentation issues**: LVGL uses a memory pool

**Current Memory Usage:**
```
SPIRAM Memory Info:
  Total Size        : 33554432 B (32768.0 KB)  ← Total available
  Allocated Bytes   :  3726812 B (3639.5 KB)  ← Currently used
  Free Bytes        : 29824644 B (29125.6 KB) ← Still free
```

You're using only **11% of available PSRAM**. Plenty of room!

---

## When You SHOULD Worry About Memory

### Scenario 1: Very Complex Screens
If a screen has hundreds of widgets or large images:
- Creating/destroying becomes slow
- May cause noticeable lag

### Scenario 2: Frequent Navigation
If user rapidly switches between screens:
- Constant allocation/deallocation
- May cause stuttering

### Scenario 3: Limited Memory
On devices with less RAM than M5Stack Tab5:
- Need to be more careful
- May need screen caching

---

## Alternative Approach: Screen Caching (If Needed)

### Option 1: Keep All Screens in Memory

**Pros:**
- Instant navigation (no creation delay)
- Screens retain their state

**Cons:**
- Uses more memory
- All screens consume RAM even when not visible

**Implementation:**

```cpp
class UIManager {
private:
    // Store all screens
    static lv_obj_t* screen_main;
    static lv_obj_t* screen_sniffer;
    static lv_obj_t* screen_transmit;
    static lv_obj_t* screen_settings;

    static bool screens_created;
};

bool UIManager::init() {
    // Create ALL screens once at startup
    screen_main = ScreenMain::create();
    screen_sniffer = ScreenSniffer::create();
    screen_transmit = ScreenTransmit::create();
    screen_settings = ScreenSettings::create();

    // Hide all except main
    lv_obj_add_flag(screen_sniffer, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(screen_transmit, LV_OBJ_FLAG_HIDDEN);
    lv_obj_add_flag(screen_settings, LV_OBJ_FLAG_HIDDEN);

    // Show main screen
    lv_scr_load(screen_main);
    screens_created = true;

    return true;
}

void UIManager::navigate_to(Screen screen) {
    // Hide current screen
    lv_obj_add_flag(active_screen_obj, LV_OBJ_FLAG_HIDDEN);

    // Show target screen
    switch (screen) {
        case Screen::MAIN_DASHBOARD:
            active_screen_obj = screen_main;
            break;
        case Screen::SNIFFER:
            active_screen_obj = screen_sniffer;
            break;
        // ...
    }

    lv_obj_clear_flag(active_screen_obj, LV_OBJ_FLAG_HIDDEN);
    lv_scr_load(active_screen_obj);
}
```

**Memory Impact:**
- Main Dashboard: ~50KB
- Sniffer Screen: ~30KB
- Transmit Screen: ~30KB
- Settings Screen: ~20KB
- **Total: ~130KB** (still only 0.4% of PSRAM!)

### Option 2: Lazy Loading with Cache

Create screens on-demand but keep them:

```cpp
lv_obj_t* UIManager::get_or_create_screen(Screen screen) {
    switch (screen) {
        case Screen::MAIN_DASHBOARD:
            if (!screen_main) {
                screen_main = ScreenMain::create();
            }
            return screen_main;

        case Screen::SNIFFER:
            if (!screen_sniffer) {
                screen_sniffer = ScreenSniffer::create();
            }
            return screen_sniffer;
        // ...
    }
}

void UIManager::navigate_to(Screen screen) {
    // Get screen (creates if doesn't exist)
    lv_obj_t* new_screen = get_or_create_screen(screen);

    // Switch to it
    lv_scr_load(new_screen);
    active_screen_obj = new_screen;
}
```

**Pros:**
- First navigation creates screen
- Subsequent navigations are instant
- Lower initial memory footprint

**Cons:**
- First visit to each screen has small delay
- Memory grows as user explores

---

## What You Should Do for PocketCAN

### Recommendation: Keep Current Approach

**Why:**
1. ✅ Simple and clean code
2. ✅ No memory leaks to worry about
3. ✅ Plenty of PSRAM available (32MB)
4. ✅ Screens are small (~30-50KB each)
5. ✅ Navigation is fast enough

### When to Switch to Caching

**Consider caching if:**
- [ ] Navigation feels slow (>100ms delay)
- [ ] Screens need to remember their state
- [ ] You want instant screen switching

**For now:** The current approach is perfect!

---

## Memory Management Best Practices

### DO ✅

1. **Use `lv_obj_del()` to delete screens**
   ```cpp
   if (active_screen_obj) {
       lv_obj_del(active_screen_obj);  // Frees all memory
   }
   ```

2. **Check for null pointers**
   ```cpp
   if (status_bar) {
       StatusBar::update(status_bar);
   }
   ```

3. **Use static variables for persistent widgets**
   ```cpp
   static lv_obj_t* status_bar = nullptr;
   ```

4. **Let LVGL manage memory**
   - Don't manually `free()` LVGL objects
   - LVGL has its own memory pool

### DON'T ❌

1. **Don't manually free LVGL objects**
   ```cpp
   free(screen);  // ❌ WRONG! Use lv_obj_del()
   ```

2. **Don't store raw pointers without cleanup**
   ```cpp
   lv_obj_t* label = lv_label_create(screen);
   // If screen is deleted, label is auto-deleted ✅
   ```

3. **Don't create objects in update() loop**
   ```cpp
   void update() {
       lv_obj_t* label = lv_label_create(screen);  // ❌ Memory leak!
       // This creates a NEW label every millisecond!
   }
   ```

   Instead, create once:
   ```cpp
   static lv_obj_t* label = nullptr;

   void create() {
       label = lv_label_create(screen);  // ✅ Create once
   }

   void update() {
       if (label) {
           lv_label_set_text(label, "Update text");  // ✅ Just update
       }
   }
   ```

---

## How to Monitor Memory Usage

### Check PSRAM Usage

Add to your `update()` function (temporarily for debugging):

```cpp
void UIManager::update() {
    static uint32_t last_print = 0;

    // Print memory stats every 5 seconds
    if (millis() - last_print > 5000) {
        size_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
        size_t total_psram = heap_caps_get_total_size(MALLOC_CAP_SPIRAM);
        size_t used_psram = total_psram - free_psram;

        Serial.printf("PSRAM: %d KB used / %d KB total (%.1f%%)\n",
                      used_psram / 1024,
                      total_psram / 1024,
                      (used_psram * 100.0) / total_psram);

        last_print = millis();
    }

    // ... rest of update
}
```

### What to Look For

**Good:**
```
PSRAM: 3640 KB used / 32768 KB total (11.1%)  ✅
```

**Warning:**
```
PSRAM: 28000 KB used / 32768 KB total (85.4%)  ⚠️
// Consider optimizing or caching
```

**Critical:**
```
PSRAM: 32000 KB used / 32768 KB total (97.6%)  🔴
// Definitely need to optimize!
```

---

## Summary

### Current PocketCAN Approach

```
Navigate to Screen A → Create Screen A
         ↓
Navigate to Screen B → Delete Screen A, Create Screen B
         ↓
Navigate to Screen A → Delete Screen B, Create Screen A again
```

**Memory is properly freed each time!** ✅

### Key Points

1. **No memory leaks**: `lv_obj_del()` frees everything automatically
2. **Plenty of PSRAM**: Using only 11% of 32MB
3. **Current approach is fine**: Don't need to change anything
4. **Simple is good**: Less code = fewer bugs

### When to Revisit

- If navigation feels slow
- If you want screens to remember their state (e.g., scroll position)
- If adding many large images/data to screens

For now, **you're good to go!** The architecture handles memory properly. 🎉

---

## Quick Answer to Your Question

**Q: Do I have to work with freeing allocated memory?**

**A: No!** LVGL does it automatically when you call `lv_obj_del()`.

**Q: Or is screen used again?**

**A: No, screens are created fresh each time.** But this is intentional and works fine!

**Memory is managed correctly** - you don't need to worry about it unless you run into performance issues (which you won't with 32MB PSRAM). 😊

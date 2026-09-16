#pragma once

/**
 * Ring Buffer (Circular Buffer)
 * NOT thread-safe. All access is from the main loop by design.
 * Storage comes from PSRAM on the device via pc_alloc().
 */

#include <cstdint>
#include <cstring>

#include "psram_alloc.h"

template<typename T>
class RingBuffer {
public:
    RingBuffer(size_t capacity) : capacity(capacity), head(0), tail(0), count(0) {
        buffer = static_cast<T*>(pc_alloc(sizeof(T) * capacity));
    }

    ~RingBuffer() {
        pc_free(buffer);
    }

    /**
     * Push item to buffer. When full, silently overwrites the oldest item.
     * @return always true - kept for call-site compatibility
     */
    bool push(const T& item) {
        if (is_full()) {
            // Overwrite oldest item
            tail = (tail + 1) % capacity;
        } else {
            count++;
        }

        buffer[head] = item;
        head = (head + 1) % capacity;
        return true;
    }

    /**
     * Pop item from buffer
     * @param item Output item
     * @return true if successful, false if buffer empty
     */
    bool pop(T& item) {
        if (is_empty()) {
            return false;
        }

        item = buffer[tail];
        tail = (tail + 1) % capacity;
        count--;
        return true;
    }

    /**
     * Peek at item without removing
     * @param index Index from tail (0 = oldest)
     * @param item Output item
     * @return true if successful
     */
    bool peek(size_t index, T& item) const {
        if (index >= count) {
            return false;
        }

        size_t pos = (tail + index) % capacity;
        item = buffer[pos];
        return true;
    }

    /**
     * Get item by index (read-only reference)
     */
    const T* get(size_t index) const {
        if (index >= count) {
            return nullptr;
        }
        size_t pos = (tail + index) % capacity;
        return &buffer[pos];
    }

    /**
     * Clear buffer
     */
    void clear() {
        head = 0;
        tail = 0;
        count = 0;
    }

    /**
     * Check if buffer is empty
     */
    bool is_empty() const {
        return count == 0;
    }

    /**
     * Check if buffer is full
     */
    bool is_full() const {
        return count == capacity;
    }

    /**
     * Get number of items in buffer
     */
    size_t size() const {
        return count;
    }

    /**
     * Get buffer capacity
     */
    size_t get_capacity() const {
        return capacity;
    }

private:
    T* buffer;
    size_t capacity;
    size_t head;
    size_t tail;
    size_t count;
};

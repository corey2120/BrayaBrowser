#ifndef GOBJECT_PTR_H
#define GOBJECT_PTR_H

#include <glib-object.h>
#include <utility>

/**
 * RAII wrapper for GObject pointers with automatic reference counting
 *
 * This class manages GObject lifecycle automatically:
 * - Increments reference count on construction/copy
 * - Decrements reference count on destruction
 * - Prevents memory leaks from forgetting g_object_unref()
 *
 * Usage:
 *   GObjectPtr<GdkTexture> texture = gdk_texture_new_from_file(...);
 *   // No need to call g_object_unref - happens automatically
 */
template<typename T>
class GObjectPtr {
private:
    T* ptr;

    void ref() {
        if (ptr) {
            g_object_ref(ptr);
        }
    }

    void unref() {
        if (ptr) {
            g_object_unref(ptr);
            ptr = nullptr;
        }
    }

public:
    // Default constructor
    GObjectPtr() : ptr(nullptr) {}

    // Constructor from raw pointer (takes ownership, adds ref)
    explicit GObjectPtr(T* p, bool add_ref = true) : ptr(p) {
        if (add_ref && ptr) {
            g_object_ref(ptr);
        }
    }

    // Destructor - automatically unrefs
    ~GObjectPtr() {
        unref();
    }

    // Copy constructor - increments ref count
    GObjectPtr(const GObjectPtr& other) : ptr(other.ptr) {
        ref();
    }

    // Move constructor - transfers ownership without ref counting
    GObjectPtr(GObjectPtr&& other) noexcept : ptr(other.ptr) {
        other.ptr = nullptr;
    }

    // Copy assignment
    GObjectPtr& operator=(const GObjectPtr& other) {
        if (this != &other) {
            unref();
            ptr = other.ptr;
            ref();
        }
        return *this;
    }

    // Move assignment
    GObjectPtr& operator=(GObjectPtr&& other) noexcept {
        if (this != &other) {
            unref();
            ptr = other.ptr;
            other.ptr = nullptr;
        }
        return *this;
    }

    // Assignment from raw pointer
    GObjectPtr& operator=(T* p) {
        if (ptr != p) {
            unref();
            ptr = p;
            ref();
        }
        return *this;
    }

    // Get raw pointer (doesn't transfer ownership)
    T* get() const {
        return ptr;
    }

    // Release ownership without unreffing (for transferring to C API)
    T* release() {
        T* tmp = ptr;
        ptr = nullptr;
        return tmp;
    }

    // Reset to new pointer
    void reset(T* p = nullptr, bool add_ref = true) {
        if (ptr != p) {
            unref();
            ptr = p;
            if (add_ref && ptr) {
                g_object_ref(ptr);
            }
        }
    }

    // Boolean conversion (for if checks)
    explicit operator bool() const {
        return ptr != nullptr;
    }

    // Pointer dereference
    T* operator->() const {
        return ptr;
    }

    // Implicit conversion to raw pointer (for C API compatibility)
    operator T*() const {
        return ptr;
    }

    // Equality operators
    bool operator==(const GObjectPtr& other) const {
        return ptr == other.ptr;
    }

    bool operator!=(const GObjectPtr& other) const {
        return ptr != other.ptr;
    }

    bool operator==(T* p) const {
        return ptr == p;
    }

    bool operator!=(T* p) const {
        return ptr != p;
    }
};

/**
 * Helper function to create GObjectPtr without adding extra ref
 * Use when function already returns a ref-counted object
 *
 * Example:
 *   auto texture = adopt(gdk_texture_new_from_file(...));
 */
template<typename T>
GObjectPtr<T> adopt(T* ptr) {
    return GObjectPtr<T>(ptr, false);
}

/**
 * Helper function to create GObjectPtr with extra ref
 * Use when you want to keep a reference to an existing object
 *
 * Example:
 *   auto texture = retain(existing_texture);
 */
template<typename T>
GObjectPtr<T> retain(T* ptr) {
    return GObjectPtr<T>(ptr, true);
}

#endif // GOBJECT_PTR_H

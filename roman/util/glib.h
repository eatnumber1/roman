#ifndef CDMANIP_UTIL_GLIB_H_
#define CDMANIP_UTIL_GLIB_H_

#include <memory.h>

#include <glib.h>
#include <glib-object.h>

#include "absl/memory/memory.h"

namespace roman {

struct GFree {
  void operator()(void *ptr) const;
};

struct GHashTableDestroy {
  void operator()(GHashTable *ptr) const;
};

template <typename T>
class GObjectPtr {
 public:
  GObjectPtr() = default;
  GObjectPtr(nullptr_t) {};

  explicit GObjectPtr(T *obj) : obj_(obj) {}

  ~GObjectPtr() {
    Reset();
  }

  GObjectPtr(GObjectPtr<T> &&o) : obj_(o.obj_) {
    o.obj_ = nullptr;
  }

  GObjectPtr(const GObjectPtr<T> &o) : obj_(o.obj_) {
    g_object_ref(obj_);
  }

  GObjectPtr<T> &operator=(const GObjectPtr<T> &o) {
    if (this == &o) return *this;
    Reset();
    obj_ = o.obj_;
    g_object_ref(obj_);
    return *this;
  }

  T &operator*() const { return *obj_; }
  T *operator->() const { return obj_; }
  T *Get() const { return obj_; }
  explicit operator bool () const { return obj_; }
  bool operator==(nullptr_t) { return obj_ == nullptr; }

  T *Release() {
    T *ret = obj_;
    obj_ = nullptr;
    return ret;
  }

  void Reset(nullptr_t = nullptr) { Reset(static_cast<T*>(nullptr)); }

  void Reset(T *obj) {
    if (obj_ != nullptr) {
      g_object_unref(obj_);
    }
    obj_ = obj;
  }

 private:
  T *obj_ = nullptr;
};

template <typename T, typename... Args>
GObjectPtr<T> NewGObject(
    GType object_type, Args... args) {
  T *obj = static_cast<T*>(g_object_new(object_type, args..., nullptr));
  return GObjectPtr<T>(obj);
}

template <typename T, typename... Args>
GObjectPtr<T> WrapGObject(T *obj) {
  return GObjectPtr<T>(obj);
}

}  // namespace roman

#endif  // CDMANIP_UTIL_GLIB_H_

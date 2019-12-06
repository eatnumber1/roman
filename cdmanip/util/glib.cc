#include "cdmanip/util/glib.h"

namespace cdmanip {

void GFree::operator()(void *ptr) const {
  g_free(ptr);
}

void GHashTableDestroy::operator()(GHashTable *ptr) const {
  g_hash_table_destroy(ptr);
}

}  // namespace cdmanip

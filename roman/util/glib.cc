#include "roman/util/glib.h"

namespace roman {

void GFree::operator()(void *ptr) const {
  g_free(ptr);
}

void GHashTableDestroy::operator()(GHashTable *ptr) const {
  g_hash_table_destroy(ptr);
}

}  // namespace roman

#include <glib.h>

#include "roman/cdmap/mirage/cdm_writer.h"
#include "mirage/plugin.h"
#include "mirage/version.h"

extern "C" {

// TODO(eatnumber1): Give this a version number.
G_MODULE_EXPORT guint mirage_plugin_soversion_major = MIRAGE_SOVERSION_MAJOR;
G_MODULE_EXPORT guint mirage_plugin_soversion_minor = MIRAGE_SOVERSION_MINOR;

G_MODULE_EXPORT void mirage_plugin_load_plugin(MiragePlugin *plugin) {
  //mirage_parser_cdm_type_register(G_TYPE_MODULE(plugin));
  mirage_writer_cdm_type_register(G_TYPE_MODULE(plugin));
}

G_MODULE_EXPORT void mirage_plugin_unload_plugin(MiragePlugin *plugin) {}

}  // extern "C"

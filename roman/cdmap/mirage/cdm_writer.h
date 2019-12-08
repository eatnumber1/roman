#ifndef ROMAN_CDMAP_MIRAGE_WRITER_H_
#define ROMAN_CDMAP_MIRAGE_WRITER_H_

#include <gmodule.h>
#include <glib-object.h>
#include <gio/gio.h>

#include "mirage/object.h"
#include "mirage/writer.h"

G_BEGIN_DECLS

#define MIRAGE_TYPE_WRITER_CDM (mirage_writer_cdm_get_type())
#define MIRAGE_WRITER_CDM(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), MIRAGE_TYPE_WRITER_CDM, MirageWriterCdm))
#define MIRAGE_WRITER_CDM_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), \
                           MIRAGE_TYPE_WRITER_CDM, MirageWriterCdmClass))
#define MIRAGE_IS_WRITER_CDM(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), MIRAGE_TYPE_WRITER_CDM))
#define MIRAGE_IS_WRITER_CDM_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), MIRAGE_TYPE_WRITER_CDM))
#define MIRAGE_WRITER_CDM_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS((obj), \
                             MIRAGE_TYPE_WRITER_CDM, MirageWriterCdmClass))

typedef struct MirageWriterCdmPrivate MirageWriterCdmPrivate;

typedef struct MirageWriterCdm {
  MirageWriter parent_instance;

  MirageWriterCdmPrivate *priv;
} MirageWriterCdm;

typedef struct MirageWriterCdmClass {
  MirageWriterClass parent_class;
} MirageWriterCdmClass;

GType mirage_writer_cdm_get_type();
void mirage_writer_cdm_type_register(GTypeModule *type_module);

G_END_DECLS

#endif  // ROMAN_CDMAP_MIRAGE_WRITER_H_

#ifndef ROMAN_CDMAP_MIRAGE_FLAC_FILTER_H_
#define ROMAN_CDMAP_MIRAGE_FLAC_FILTER_H_

#include <gmodule.h>
#include <glib-object.h>
#include <gio/gio.h>

#include "mirage/mirage.h"
#include "mirage/object.h"
#include "mirage/filter-stream.h"

G_BEGIN_DECLS

#define MIRAGE_TYPE_FILTER_STREAM_FLACFILE (mirage_filter_stream_flacfile_get_type())
#define MIRAGE_FILTER_STREAM_FLACFILE(obj) \
  (G_TYPE_CHECK_INSTANCE_CAST((obj), MIRAGE_TYPE_FILTER_STREAM_FLACFILE, MirageFilterStreamFlacfile))
#define MIRAGE_FILTER_STREAM_FLACFILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_CAST((klass), \
                           MIRAGE_TYPE_FILTER_STREAM_FLACFILE, MirageFilterStreamFlacfileClass))
#define MIRAGE_IS_FILTER_STREAM_FLACFILE(obj) \
  (G_TYPE_CHECK_INSTANCE_TYPE((obj), MIRAGE_TYPE_FILTER_STREAM_FLACFILE))
#define MIRAGE_IS_FILTER_STREAM_FLACFILE_CLASS(klass) \
  (G_TYPE_CHECK_CLASS_TYPE((klass), MIRAGE_TYPE_FILTER_STREAM_FLACFILE))
#define MIRAGE_FILTER_STREAM_FLACFILE_GET_CLASS(obj) \
  (G_TYPE_INSTANCE_GET_CLASS((obj), \
                             MIRAGE_TYPE_FILTER_STREAM_FLACFILE, MirageFilterStreamFlacfileClass))

typedef struct MirageFilterStreamFlacfilePrivate MirageFilterStreamFlacfilePrivate;

typedef struct MirageFilterStreamFlacfile {
  MirageFilterStream parent_instance;

  MirageFilterStreamFlacfilePrivate *priv;
} MirageFilterStreamFlacfile;

typedef struct MirageFilterStreamFlacfileClass {
  MirageFilterStreamClass parent_class;
} MirageFilterStreamFlacfileClass;

GType mirage_filter_stream_flacfile_get_type();

G_END_DECLS

#endif  // ROMAN_CDMAP_MIRAGE_FLAC_FILTER_H_

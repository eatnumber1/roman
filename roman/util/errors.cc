#include "roman/util/errors.h"

namespace roman {

GQuark ErrorQuark() {
  return g_quark_from_static_string("roman-error-quark");
}

}  // namespace roman

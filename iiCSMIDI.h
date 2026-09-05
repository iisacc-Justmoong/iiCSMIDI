#pragma once

#include <QtCore/QString>
#include <QtCore/QtGlobal>

#if defined(IICSMIDI_BUILDING_LIBRARY)
#    define IICSMIDI_EXPORT Q_DECL_EXPORT
#else
#    define IICSMIDI_EXPORT Q_DECL_IMPORT
#endif

namespace iiCSMIDI {

[[nodiscard]] IICSMIDI_EXPORT QString helloWorld();

} // namespace iiCSMIDI

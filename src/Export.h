#pragma once
#include <QtCore/QtGlobal>
#if defined(IICSMIDI_BUILDING_LIBRARY)
#    define IICSMIDI_EXPORT Q_DECL_EXPORT
#else
#    define IICSMIDI_EXPORT Q_DECL_IMPORT
#endif


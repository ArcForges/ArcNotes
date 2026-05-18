#include <QtGlobal>

#define RELEASE "Self-build"

#ifdef Q_OS_LINUX
#define PLATFORM "linux"
#endif

#ifdef Q_OS_MAC
#define PLATFORM "macosx"
#endif

#ifdef Q_OS_WIN32
#define PLATFORM "windows"
#endif

#ifndef PLATFORM
#define PLATFORM "other"
#endif

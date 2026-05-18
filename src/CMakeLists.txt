cmake_minimum_required(VERSION 3.5)
project(ArcNotes)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# BEGIN options
option(ARC_QT6_BUILD "Build ArcNotes with Qt6" OFF)
option(DEV_MODE "Build ArcNotes in developer mode" OFF)
# END options

if(ARC_QT6_BUILD)
  set(QT_VERSION_MAJOR 6)
  set(QT_DEFAULT_MAJOR_VERSION 6)
else()
  set(QT_VERSION_MAJOR 5)
endif()

# put the ArcNotes.exe in build directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

# for intellisence
set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)

add_executable(ArcNotes "")

if(CMAKE_BUILD_TYPE MATCHES Debug)
  message("Debug build configuration detected")
  add_definitions(-DQT_DEBUG)
else()
  message("Non-debug build configuration detected")
endif()

set_target_properties(
  ArcNotes
  PROPERTIES CXX_STANDARD 20
             CXX_STANDARD_REQUIRED ON
             AUTOMOC ON
             AUTOUIC OFF
             AUTORCC ON)

if(UNIX AND DEV_MODE)
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    target_compile_options(
      ArcNotes
      PRIVATE -Wall -Wextra -pedantic -Wno-gnu-zero-variadic-macro-arguments
              -Wno-error=deprecated-declarations)
  else()
    target_compile_options(ArcNotes PRIVATE -Wall -Wextra -pedantic
                                            -Wno-error=deprecated-declarations)
  endif()

  # enable werror on linux
  if(NOT APPLE)
    message("Werror enabled on linux")
    target_compile_options(ArcNotes PRIVATE -Werror)
  endif()
endif()

target_include_directories(ArcNotes PRIVATE
  ${CMAKE_CURRENT_LIST_DIR}
  ${CMAKE_CURRENT_BINARY_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/libraries/qtcsv/src)

# Disable shared library building of libraries
set(BUILD_SHARED_LIBS OFF)
# QHotKey library - disable tests
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(BUILD_WITH_QT6 ${ARC_QT6_BUILD})
set(BUILD_TRANSLATIONS
    OFF
    CACHE BOOL "" FORCE)
set(BUILD_TEST_APPLICATION
    OFF
    CACHE BOOL "" FORCE)
set(BUILD_QTQUICK_DEMO
    OFF
    CACHE BOOL "" FORCE)
set(BUILD_TESTING
    OFF
    CACHE BOOL "" FORCE)

if(NOT ARC_QT6_BUILD)
  find_package(
    Qt5
    COMPONENTS Core
    REQUIRED)
endif()

set(QHOTKEY_EXAMPLES OFF)
add_subdirectory(libraries/qhotkey EXCLUDE_FROM_ALL)

# FakeVim library
set(CREATE_STATIC_LIBRARY ON)
if(MSVC)
  add_compile_definitions(FAKEVIM_STATIC_DEFINE QTCREATOR_UTILS_STATIC_LIB)
endif()
add_subdirectory(libraries/fakevim EXCLUDE_FROM_ALL)
add_subdirectory(libraries/diff_match_patch EXCLUDE_FROM_ALL)

option(USE_QLITEHTML "Build using QLiteHtml for preview" ON)
# QLiteHtml requires Qt 6+
if(USE_QLITEHTML AND QT_VERSION_MAJOR LESS 6)
  message(WARNING "QLiteHtml requires Qt 6+, disabling USE_QLITEHTML")
  set(USE_QLITEHTML OFF)
endif()
if(USE_QLITEHTML)
  add_definitions(-DUSE_QLITEHTML=1)
  # Build qlitehtml as a static library by default to avoid shared library issues
  if(NOT QLITEHTML_LIBRARY_TYPE)
    set(QLITEHTML_LIBRARY_TYPE STATIC)
  endif()
  # ArcNotes relies on the bundled litehtml sources.
  set(CMAKE_DISABLE_FIND_PACKAGE_litehtml ON)
  add_subdirectory(libraries/qlitehtml EXCLUDE_FROM_ALL)
endif()

message("Using Qt version ${QT_VERSION_MAJOR}")
set(QT_NO_CREATE_VERSIONLESS_FUNCTIONS ON)

find_package(
  Qt${QT_VERSION_MAJOR}
  COMPONENTS
  REQUIRED
  LinguistTools
  Core
  Widgets
  Gui
  Sql
  Svg
  Network
  Xml
  PrintSupport
  Concurrent)

find_program(CCACHE_FOUND ccache)

if(CCACHE_FOUND)
  message(STATUS "ccache found")
  set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
  set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
endif(CCACHE_FOUND)

# ─── Resource files ───────────────────────────────────────────────────────────
# First-party QRC files live at the src/ root; glob them non-recursively so
# library QRC files are not accidentally pulled in.
file(GLOB ARCNOTES_FIRST_PARTY_RESOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/*.qrc)

set(RESOURCE_FILES
    ${ARCNOTES_FIRST_PARTY_RESOURCES}
    libraries/qmarkdowntextedit/media.qrc
    libraries/qdarkstyle/style.qrc
    libraries/qkeysequencewidget/qkeysequencewidget/qkeysequencewidget.qrc)

# ─── First-party C++ sources ──────────────────────────────────────────────────
# Recurse the entire src/ tree then strip out the libraries/ subtree and the
# Linux-only XDG shortcut manager (added back conditionally below).
file(GLOB_RECURSE ARCNOTES_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/*.h
  ${CMAKE_CURRENT_SOURCE_DIR}/*.c)
list(FILTER ARCNOTES_SOURCES EXCLUDE REGEX "/libraries/")
list(FILTER ARCNOTES_SOURCES EXCLUDE REGEX "xdgglobalshortcutmanager")

# ─── Bundled library sources compiled into ArcNotes ──────────────────────────

# qmarkdowntextedit — exclude the library's own standalone demo app files
file(GLOB QMARKDOWNTEXTEDIT_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/libraries/qmarkdowntextedit/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/libraries/qmarkdowntextedit/*.h)
list(FILTER QMARKDOWNTEXTEDIT_SOURCES EXCLUDE REGEX "/(main|mainwindow)\\.(cpp|h)$")

# SingleApplication
file(GLOB SINGLEAPPLICATION_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/libraries/singleapplication/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/libraries/singleapplication/*.h)

# Qt Toolbar Editor — only the toolbar_editor component; demo and
# designer_plugin subdirectories are not compiled into ArcNotes
file(GLOB QTTOOLBAREDITOR_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/libraries/qttoolbareditor/src/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/libraries/qttoolbareditor/src/*.hpp)

# qtcsv — only the CSV reader path; writer/stringdata/variantdata use Qt5-only
# QTextCodec and contentiterator has a Qt5-style QStringList forward declaration
# that conflicts with Qt6. Keep this list explicit to avoid Qt5/Qt6 issues.
set(QTCSV_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/libraries/qtcsv/src/include/qtcsv_global.h
    ${CMAKE_CURRENT_SOURCE_DIR}/libraries/qtcsv/src/include/abstractdata.h
    ${CMAKE_CURRENT_SOURCE_DIR}/libraries/qtcsv/src/include/reader.h
    ${CMAKE_CURRENT_SOURCE_DIR}/libraries/qtcsv/src/sources/reader.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/libraries/qtcsv/src/sources/filechecker.h
    ${CMAKE_CURRENT_SOURCE_DIR}/libraries/qtcsv/src/sources/symbols.h)

# QKeySequenceWidget
file(GLOB QKEYSEQUENCEWIDGET_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/libraries/qkeysequencewidget/qkeysequencewidget/src/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/libraries/qkeysequencewidget/qkeysequencewidget/src/*.h)

# Fuzzy matcher
file(GLOB FUZZY_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/libraries/fuzzy/*.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/libraries/fuzzy/*.h)

# md4c — C library; only the three implementation files in src/
file(GLOB MD4C_SOURCES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/libraries/md4c/src/*.c
  ${CMAKE_CURRENT_SOURCE_DIR}/libraries/md4c/src/*.h)

set(SOURCE_FILES
    ${ARCNOTES_SOURCES}
    ${QMARKDOWNTEXTEDIT_SOURCES}
    ${SINGLEAPPLICATION_SOURCES}
    ${QTTOOLBAREDITOR_SOURCES}
    ${QTCSV_SOURCES}
    ${QKEYSEQUENCEWIDGET_SOURCES}
    ${FUZZY_SOURCES}
    ${MD4C_SOURCES})

# XDG Desktop Portal global shortcuts — Linux/Wayland only
if(UNIX AND NOT APPLE)
  find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS DBus)
  list(APPEND SOURCE_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/services/xdgglobalshortcutmanager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/services/xdgglobalshortcutmanager.h)
endif()

# ─── Translation files ────────────────────────────────────────────────────────
file(GLOB ARC_TS_FILES CONFIGURE_DEPENDS
  ${CMAKE_CURRENT_SOURCE_DIR}/languages/ArcNotes_*.ts)

if(ARC_QT6_BUILD)
  qt6_add_translation(ARC_QM_FILES ${ARC_TS_FILES})
else()
  qt5_add_translation(ARC_QM_FILES ${ARC_TS_FILES})
endif()

# ─── Vendor .ui files ─────────────────────────────────────────────────────────
# First-party UI is code-built (no .ui files). These are retained only for
# bundled third-party libraries that still ship .ui forms.
set(ARC_VENDOR_UI_FILES
    libraries/qmarkdowntextedit/qplaintexteditsearchwidget.ui
    libraries/qttoolbareditor/src/toolbar_editor.ui)

if(ARC_QT6_BUILD)
  qt6_wrap_ui(ARC_VENDOR_UI_HEADERS ${ARC_VENDOR_UI_FILES})
else()
  qt5_wrap_ui(ARC_VENDOR_UI_HEADERS ${ARC_VENDOR_UI_FILES})
endif()

add_custom_target(
  translations
  DEPENDS ${ARC_QM_FILES}
  COMMENT "Generate translation files")

target_sources(ArcNotes PRIVATE ${SOURCE_FILES} ${RESOURCE_FILES}
                                ${ARC_QM_FILES} ${ARC_VENDOR_UI_HEADERS})

# The Qt5::Widgets target will also link Qt::Gui and Qt::Core targets
target_link_libraries(
  ArcNotes
  PRIVATE Qt${QT_VERSION_MAJOR}::Widgets
          Qt${QT_VERSION_MAJOR}::Sql
          Qt${QT_VERSION_MAJOR}::Svg
          Qt${QT_VERSION_MAJOR}::Network
          Qt${QT_VERSION_MAJOR}::Xml
          Qt${QT_VERSION_MAJOR}::PrintSupport
          Qt${QT_VERSION_MAJOR}::Concurrent
          QHotkey::QHotkey
          fakevim
          Google::DiffMatchPatch)

if(USE_QLITEHTML)
  target_link_libraries(ArcNotes PRIVATE qlitehtml::qlitehtml)
endif()

if(UNIX AND NOT APPLE)
  target_link_libraries(ArcNotes PRIVATE Qt${QT_VERSION_MAJOR}::DBus)
endif()

if(MINGW OR MSVC)
  # Qt 5 does not define UNICODE for CMake only qmake automatically define UNICODE
  target_compile_definitions(ArcNotes PRIVATE UNICODE _UNICODE)
endif()

if(MSVC)
  target_compile_definitions(ArcNotes PRIVATE NOMINMAX)
endif()

add_compile_definitions(QAPPLICATION_CLASS=QApplication)

# Installation
install(TARGETS ArcNotes DESTINATION bin)
foreach(
  size
  16x16
  24x24
  32x32
  48x48
  64x64
  96x96
  128x128
  256x256
  512x512
  scalable)
  install(DIRECTORY images/icons/${size} DESTINATION share/icons/hicolor)
endforeach()
install(FILES PBE.ArcNotes.desktop DESTINATION share/applications)
install(FILES ${ARC_QM_FILES} DESTINATION share/ArcNotes/translations)

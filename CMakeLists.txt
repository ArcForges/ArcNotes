cmake_minimum_required(VERSION 4.2 FATAL_ERROR)

project(ArcNotesRoot LANGUAGES C CXX)

option(ENABLE_ASAN "Enable address sanitizer for native targets." OFF)
option(ENABLE_CLANG_TIDY "Enable clang-tidy during native builds." OFF)
option(USE_WERROR "Error on compile warning" OFF)

# Embed MSVC/UCRT/SDK system include paths into compile_commands.json so that
# clang-tidy can resolve Windows toolchain headers outside the vcvars shell.
if(MSVC AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND DEFINED ENV{INCLUDE})
  set(CMAKE_CXX_STANDARD_INCLUDE_DIRECTORIES $ENV{INCLUDE})
  set(CMAKE_C_STANDARD_INCLUDE_DIRECTORIES $ENV{INCLUDE})
endif()

if(CMAKE_EXPORT_COMPILE_COMMANDS)
  add_custom_target(
    sync-compile-commands ALL
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/build"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_BINARY_DIR}/compile_commands.json"
            "${CMAKE_SOURCE_DIR}/build/compile_commands.json"
    COMMENT "Syncing compile_commands.json -> build/"
    VERBATIM)
endif()

if(ENABLE_ASAN AND (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
                    OR CMAKE_CXX_COMPILER_ID MATCHES "Clang"))
  set(ADDRESS_SANITIZER_FLAGS "-fsanitize=address -fno-omit-frame-pointer")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ADDRESS_SANITIZER_FLAGS}")
endif()

if(MSVC)
  add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:/EHsc>")
endif()

add_subdirectory(src)

if(MSVC)
  set(ARC_COMPILE_OPTIONS
      /W4
      "$<$<COMPILE_LANGUAGE:CXX>:/MP>"
      "$<$<COMPILE_LANGUAGE:CXX>:/permissive->"
      "$<$<CONFIG:RELEASE>:/O2>")

  if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    list(APPEND ARC_COMPILE_OPTIONS -Wno-unused-command-line-argument
         -Wno-unused-parameter -Wno-microsoft-include)
  else()
    list(APPEND ARC_COMPILE_OPTIONS /external:anglebrackets /external:W0
         /wd4100)
  endif()

  if(USE_WERROR)
    list(APPEND ARC_COMPILE_OPTIONS /WX)
  endif()

  target_compile_options(ArcNotes PRIVATE ${ARC_COMPILE_OPTIONS})
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES
                                                 "Clang")
  set(ARC_COMPILE_OPTIONS -Wuninitialized -Wall -Wextra -Wno-unused-parameter
                          -Wshadow)

  if(USE_WERROR)
    list(APPEND ARC_COMPILE_OPTIONS -Werror)
  endif()

  target_compile_options(ArcNotes PRIVATE ${ARC_COMPILE_OPTIONS})
endif()

if(ENABLE_CLANG_TIDY)
  find_program(CLANG_TIDY_EXECUTABLE NAMES clang-tidy)
  if(CLANG_TIDY_EXECUTABLE)
    set_target_properties(ArcNotes PROPERTIES CXX_CLANG_TIDY
                                              "${CLANG_TIDY_EXECUTABLE};--quiet")
  else()
    message(WARNING "ENABLE_CLANG_TIDY is ON but clang-tidy was not found in PATH.")
  endif()
endif()

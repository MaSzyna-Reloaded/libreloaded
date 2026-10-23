CMAKE_MINIMUM_REQUIRED(VERSION 3.17)
SET(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_EXTENSIONS ON)
SET(CMAKE_COLOR_DIAGNOSTICS ON)
# scripts/style-check binds the same double-precision API as the build, so clang-tidy sees the
# same types; it is dumped by the configure below and never versioned
set(GODOTCPP_PRECISION "double" CACHE STRING "" FORCE)
set(GODOTCPP_CUSTOM_API_FILE "${CMAKE_SOURCE_DIR}/extension_api.json" CACHE FILEPATH "" FORCE)
# godot-cpp ships the single-precision API only, so the double one is dumped once from a
# double-precision Godot and not versioned; delete it to dump it again after a GODOT_VERSION bump
IF (NOT EXISTS "${GODOTCPP_CUSTOM_API_FILE}")
    FIND_PROGRAM(
        GODOT_EXECUTABLE NAMES godot-double godot REQUIRED
        DOC "A double-precision Godot, used to dump extension_api.json"
    )
    MESSAGE(STATUS "Dumping extension_api.json with ${GODOT_EXECUTABLE}")
    # --dump-extension-api writes the file into the working directory
    EXECUTE_PROCESS(
        COMMAND "${GODOT_EXECUTABLE}" --headless --dump-extension-api
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        RESULT_VARIABLE GODOT_DUMP_RESULT
        OUTPUT_QUIET
    )
    IF (EXISTS "${GODOTCPP_CUSTOM_API_FILE}")
        # "precision" sits in the "header" object at the top of the dump
        FILE(READ "${GODOTCPP_CUSTOM_API_FILE}" GODOT_API_HEADER LIMIT 1024)
        STRING(FIND "${GODOT_API_HEADER}" "\"precision\": \"double\"" GODOT_API_DOUBLE)
    ENDIF ()
    IF (NOT GODOT_DUMP_RESULT EQUAL 0 OR NOT GODOT_API_DOUBLE GREATER -1)
        FILE(REMOVE "${GODOTCPP_CUSTOM_API_FILE}")
        MESSAGE(FATAL_ERROR
            "${GODOT_EXECUTABLE} did not dump a double-precision extension API; "
            "pass -DGODOT_EXECUTABLE=<path to a double-precision Godot>")
    ENDIF ()
ENDIF ()
OPTION(LIBRELOADED_DEBUG "If the build should contain custom library logger" OFF)
# Do not hard‑force a generator; allow users to select one suitable for cross‑compiling.
# Silence unused variable warning when specified from toolchain
IF (CMAKE_C_COMPILER)
ENDIF ()

# Allow selecting platform via -DGODOTCPP_PLATFORM=android|windows|linux|macos|ios|web
IF (DEFINED GODOTCPP_PLATFORM AND NOT CMAKE_SYSTEM_NAME)
    STRING(TOLOWER "${GODOTCPP_PLATFORM}" _req_platform)
    IF (_req_platform STREQUAL "android")
        # Prefer the official Android NDK toolchain if not explicitly set
        # Allow manual override via ANDROID_NDK_TOOLCHAIN (full path) or ANDROID_NDK_ROOT (NDK dir)
        IF (NOT DEFINED CMAKE_TOOLCHAIN_FILE)
            # 1) Explicit toolchain file provided by user
            IF (DEFINED ANDROID_NDK_TOOLCHAIN)
                SET(CMAKE_TOOLCHAIN_FILE "${ANDROID_NDK_TOOLCHAIN}" CACHE FILEPATH "Android NDK toolchain (manual)" FORCE)
                # 2) NDK root directory provided by user
            ELSEIF (DEFINED ANDROID_NDK_ROOT)
                SET(CMAKE_TOOLCHAIN_FILE "${ANDROID_NDK_ROOT}/build/cmake/android.toolchain.cmake" CACHE FILEPATH "Android NDK toolchain (from ANDROID_NDK_ROOT)" FORCE)
                # 3) Environment variables commonly used
            ELSEIF (DEFINED ENV{ANDROID_NDK})
                SET(CMAKE_TOOLCHAIN_FILE "$ENV{ANDROID_NDK}/build/cmake/android.toolchain.cmake" CACHE FILEPATH "Android NDK toolchain (from ANDROID_NDK)" FORCE)
            ELSEIF (DEFINED ENV{ANDROID_NDK_HOME})
                SET(CMAKE_TOOLCHAIN_FILE "$ENV{ANDROID_NDK_HOME}/build/cmake/android.toolchain.cmake" CACHE FILEPATH "Android NDK toolchain (from ANDROID_NDK_HOME)" FORCE)
            ENDIF ()
        ENDIF ()

        # Tell CMake we are targeting Android when using the NDK toolchain
        SET(CMAKE_SYSTEM_NAME Android CACHE STRING "" FORCE)

        # Provide reasonable defaults if not supplied by user
        IF (NOT DEFINED ANDROID_ABI)
            SET(ANDROID_ABI arm64-v8a CACHE STRING "Android ABI")
        ENDIF ()
        IF (NOT DEFINED ANDROID_PLATFORM)
            SET(ANDROID_PLATFORM 34 CACHE STRING "Android API level")
        ENDIF ()

        IF (DEFINED CMAKE_TOOLCHAIN_FILE)
            MESSAGE(STATUS "Configuring cross‑compile for Android (ABI=${ANDROID_ABI}, API=${ANDROID_PLATFORM}) using toolchain: ${CMAKE_TOOLCHAIN_FILE}")
        ELSE ()
            MESSAGE(STATUS "Configuring cross‑compile for Android (ABI=${ANDROID_ABI}, API=${ANDROID_PLATFORM}).")
        ENDIF ()
    ELSEIF (_req_platform STREQUAL "windows")
        # Windows cross‑compile via MinGW (recommended from non‑Windows hosts)
        IF (NOT DEFINED CMAKE_TOOLCHAIN_FILE AND NOT CMAKE_HOST_WIN32)
            # Best effort: if cross compilers are present, set them. This must be before project().
            FIND_PROGRAM(MINGW_C "x86_64-w64-mingw32-gcc")
            FIND_PROGRAM(MINGW_CXX "x86_64-w64-mingw32-g++")
            IF (MINGW_C AND MINGW_CXX)
                SET(CMAKE_SYSTEM_NAME Windows CACHE STRING "" FORCE)
                SET(CMAKE_C_COMPILER ${MINGW_C} CACHE FILEPATH "" FORCE)
                SET(CMAKE_CXX_COMPILER ${MINGW_CXX} CACHE FILEPATH "" FORCE)
                MESSAGE(STATUS "Configuring cross‑compile for Windows using MinGW (x86_64).")
            ELSE ()
                # Fallback: still set system name; user must provide a toolchain or compilers.
                SET(CMAKE_SYSTEM_NAME Windows CACHE STRING "" FORCE)
                MESSAGE(WARNING "GODOTCPP_PLATFORM=windows set but no MinGW cross compiler auto‑detected. Provide a toolchain file or set CMAKE_C_COMPILER/CMAKE_CXX_COMPILER.")
            ENDIF ()
        ELSEIF (NOT CMAKE_HOST_WIN32)
            SET(CMAKE_SYSTEM_NAME Windows CACHE STRING "" FORCE)
            MESSAGE(STATUS "Configuring cross‑compile for Windows using specified toolchain: ${CMAKE_TOOLCHAIN_FILE}")
        ENDIF ()
    ENDIF ()
ENDIF ()

SET(LIBNAME "libreloaded" CACHE STRING "The name of the library")
SET(GODOT_PROJECT_DIR "demo" CACHE STRING "The directory of a Godot project folder - relative to this one, or absolute (a game using libreloaded as a submodule builds into its own project)")
GET_FILENAME_COMPONENT(GODOT_PROJECT_PATH "${GODOT_PROJECT_DIR}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
SET(IS_DEBUG_BUILD)

FIND_PACKAGE(Python3 3.4 REQUIRED)
ADD_SUBDIRECTORY(godot-cpp SYSTEM)

# Add godot-cpp's module path and include the exported functions.
# This is made available for documentation generation
SET(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${godot-cpp_SOURCE_DIR}/cmake")
INCLUDE(GodotCPPModule)

# The godot-cpp target has some of useful properties attached that can be retrieved like so.
GET_TARGET_PROPERTY(GODOTCPP_SUFFIX godot::cpp GODOTCPP_SUFFIX)
GET_TARGET_PROPERTY(GODOTCPP_PLATFORM godot::cpp GODOTCPP_PLATFORM)

PROJECT(Maszyna-API-Wrapper
        VERSION 1.0
        DESCRIPTION "MaSzyna API wrapper for Godot Engine written in C++ as GDExtension"
        HOMEPAGE_URL "https://github.com/MaSzyna-Reloaded/MaSzyna-API-wrapper"
        LANGUAGES CXX
)

IF (MSVC)
    # /utf-8: Set source and execution character sets to UTF-8
    # /bigobj: Increase the number of addressable sections in an .obj file
    ADD_COMPILE_OPTIONS(/utf-8 /bigobj)
ENDIF ()

FILE(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS "src/*.cpp" "src/*.hpp")

# Scenario scripts (ScenarioScriptServer) run on Lua, the vendor/lua submodule. Without it the
# server stays, and refuses every script as the original's build without Lua does
OPTION(LIBRELOADED_LUA "Run scenario scripts in Lua (vendor/lua)" ON)
IF (NOT LIBRELOADED_LUA)
    LIST(FILTER SRC_FILES EXCLUDE REGEX "src/scripting/lua/")
ENDIF ()

ADD_LIBRARY(${LIBNAME} SHARED ${SRC_FILES})
TARGET_INCLUDE_DIRECTORIES(${LIBNAME} PRIVATE src)
TARGET_SOURCES(${LIBNAME} PRIVATE)

TARGET_LINK_LIBRARIES(${LIBNAME} godot-cpp)

IF (LIBRELOADED_LUA)
    # The core and the libraries a scenario script may use - not the interpreters, the test
    # harness, the amalgamation, nor io/os/debug/package, which a script has no business with
    SET(LUA_SRC_FILES)
    FOREACH (LUA_MODULE lapi lauxlib lbaselib lcode lcorolib lctype ldebug ldo ldump lfunc lgc llex
             lmathlib lmem lobject lopcodes lparser lstate lstring lstrlib ltable ltablib ltm lundump
             lutf8lib lvm lzio)
        LIST(APPEND LUA_SRC_FILES "vendor/lua/${LUA_MODULE}.c")
    ENDFOREACH ()
    # Compiled as C++: a script error then unwinds as an exception (LUAI_THROW, ldo.c) instead of a
    # longjmp over the String and Variant locals of the wrapper's own functions
    SET_SOURCE_FILES_PROPERTIES(${LUA_SRC_FILES} PROPERTIES LANGUAGE CXX)
    ADD_LIBRARY(maszyna_lua STATIC ${LUA_SRC_FILES})
    SET_TARGET_PROPERTIES(maszyna_lua PROPERTIES POSITION_INDEPENDENT_CODE ON CXX_VISIBILITY_PRESET hidden)
    TARGET_COMPILE_OPTIONS(maszyna_lua PRIVATE $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-w>)
    TARGET_INCLUDE_DIRECTORIES(${LIBNAME} PRIVATE vendor/lua)
    TARGET_COMPILE_DEFINITIONS(${LIBNAME} PRIVATE LIBRELOADED_LUA)
    TARGET_LINK_LIBRARIES(${LIBNAME} maszyna_lua)
ENDIF ()

# The build number is not stamped here. A stamp written while the library links is overwritten by
# the next build of any other configuration, and the export then packs whichever one happened to
# be on disk - which is how a release shipped a library and a project two hours apart. The
# Makefile's `build-number` target writes it when asked, so it describes a release and so the
# Linux and the Windows package, built by separate invocations, carry the same one.

# A MinGW-built DLL otherwise needs libstdc++-6.dll, libgcc_s_seh-1.dll and libwinpthread-1.dll
# sitting next to the game, and Godot only ships the executable and this library - the extension
# would simply fail to load on a clean Windows. Godot's own MinGW builds do the same
# (use_static_cpp).
IF (MINGW)
    TARGET_LINK_OPTIONS(${LIBNAME} PRIVATE -static-libgcc -static-libstdc++ -static)
ENDIF ()

SET_TARGET_PROPERTIES(${LIBNAME}
        PROPERTIES
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
        RUNTIME_OUTPUT_DIRECTORY "$<1:${GODOT_PROJECT_PATH}/bin/libreloaded/${GODOTCPP_PLATFORM}>"
        PREFIX ""
        OUTPUT_NAME "${LIBNAME}"
)

# Determine architecture suffix for output filename
SET(ARCH_SUFFIX "")
IF (APPLE)
    SET(ARCH_SUFFIX "")
ELSE ()
    # For Android, prefer ANDROID_ABI when available
    IF (DEFINED ANDROID_ABI)
        STRING(TOLOWER "${ANDROID_ABI}" _proc_lc)
        IF (_proc_lc MATCHES "^arm64\-v8a$")
            SET(_proc_lc "arm64")
        ELSEIF (_proc_lc MATCHES "^armeabi\-v7a$")
            SET(_proc_lc "arm32")
        ELSEIF (_proc_lc STREQUAL "x86_64")
            SET(_proc_lc "x86_64")
        ELSEIF (_proc_lc STREQUAL "x86")
            SET(_proc_lc "x86")
        ENDIF ()
    ELSE ()
        STRING(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" _proc_lc)
    ENDIF ()
    IF (_proc_lc MATCHES "^(x86_64|amd64)$")
        SET(ARCH_SUFFIX ".64")
    ELSEIF (_proc_lc MATCHES "^(aarch64|arm64)$")
        SET(ARCH_SUFFIX ".arm64")
    ELSEIF (_proc_lc MATCHES "^(riscv64|rv64)$")
        SET(ARCH_SUFFIX ".rv64")
    ELSEIF (_proc_lc MATCHES "^arm(32)?$")
        SET(ARCH_SUFFIX ".arm32")
    ELSEIF (_proc_lc MATCHES "^x86$")
        SET(ARCH_SUFFIX ".32")
    ELSE ()
        # Fallback: use pointer size to detect generic 64-bit x86 when processor string is unknown
        IF (CMAKE_SIZEOF_VOID_P EQUAL 8)
            SET(ARCH_SUFFIX ".64")
        ENDIF ()
    ENDIF ()
ENDIF ()

# Compose final output name using GODOTCPP_TARGET for debug/release and architecture suffix
# GODOTCPP_TARGET values: template_debug | template_release
SET(DEBUG_SUFFIX "")
IF (GODOTCPP_TARGET STREQUAL "template_debug")
    SET(DEBUG_SUFFIX ".debug")
ENDIF ()

IF (LIBRELOADED_DEBUG)
    TARGET_COMPILE_DEFINITIONS(${LIBNAME} PRIVATE LIBRELOADED_DEBUG)
ENDIF ()

IF (GODOTCPP_TARGET STREQUAL "template_debug")
    GET_TARGET_PROPERTY(_libreloaded_link_options ${LIBNAME} LINK_OPTIONS)
    IF (_libreloaded_link_options)
        LIST(REMOVE_ITEM _libreloaded_link_options "-s")
        SET_TARGET_PROPERTIES(${LIBNAME} PROPERTIES LINK_OPTIONS "${_libreloaded_link_options}")
    ENDIF ()
ENDIF ()

SET(_OUTPUT_NAME "${LIBNAME}${DEBUG_SUFFIX}${ARCH_SUFFIX}")
SET(GODOT_PROJECT_BINARY_DIR "${GODOT_PROJECT_PATH}/bin/libreloaded/${GODOTCPP_PLATFORM}")

SET_TARGET_PROPERTIES(${LIBNAME} PROPERTIES OUTPUT_NAME "${_OUTPUT_NAME}" LIBRARY_OUTPUT_DIRECTORY "${GODOT_PROJECT_BINARY_DIR}")

# Links (UNIX, a relative link) or copies an addon into the Godot project's addons/<p_name> when it
# has none. An addons/<p_name> already there - a link, a directory, a copy - is the project's: never
# replaced, so a project that versions its links, or leads one to a checkout of its own, keeps them
SET(ADDONS_DEMO_DIR "${GODOT_PROJECT_PATH}/addons")
FILE(MAKE_DIRECTORY "${ADDONS_DEMO_DIR}")
FUNCTION(reloaded_link_addon p_source p_name)
    SET(_destination "${ADDONS_DEMO_DIR}/${p_name}")
    IF (EXISTS "${_destination}" OR IS_SYMLINK "${_destination}")
        RETURN ()
    ENDIF ()
    IF (UNIX)
        FILE(RELATIVE_PATH _relative_source "${ADDONS_DEMO_DIR}" "${p_source}")
        FILE(CREATE_LINK "${_relative_source}" "${_destination}" SYMBOLIC)
    ELSE ()
        FILE(COPY "${p_source}/" DESTINATION "${_destination}")
    ENDIF ()
ENDFUNCTION()

MESSAGE(NOTICE "Copying/linking addons and submodules to ${ADDONS_DEMO_DIR}...")
reloaded_link_addon("${PROJECT_SOURCE_DIR}/addons/libreloaded" libreloaded)
reloaded_link_addon("${PROJECT_SOURCE_DIR}/vendor/gut/addons/gut" gut)
reloaded_link_addon("${PROJECT_SOURCE_DIR}/vendor/gnd-sfx/addons/gnd_sfx" gnd_sfx)

# Plugins of Rails Reloaded compiled into this library (ReloadedPlugin). Each directory has a
# plugin.cmake, included here: it adds its sources to ${LIBNAME}, its doc_classes to DOC_XML, what
# it ships beside the library to RELOADED_PLUGIN_DEPENDENCIES ([dependencies] of the
# .gdextension), links its addon (reloaded_link_addon) and names its ReloadedPlugin class
# (reloaded_plugin)
SET(LIBRELOADED_PLUGINS "" CACHE STRING
        "Plugin directories compiled into the library, ;-separated - relative to this one, or absolute")
SET(RELOADED_PLUGIN_INCLUDES "")
SET(RELOADED_PLUGIN_CREATES "")
SET(RELOADED_PLUGIN_DEPENDENCIES "")
FILE(GLOB_RECURSE DOC_XML LIST_DIRECTORIES NO CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/doc_classes/*.xml")
MACRO(reloaded_plugin p_class p_header)
    STRING(APPEND RELOADED_PLUGIN_INCLUDES "#include \"${p_header}\"\n")
    STRING(APPEND RELOADED_PLUGIN_CREATES
            "        GDREGISTER_CLASS(${p_class});\n        created.push_back(memnew(${p_class}));\n")
ENDMACRO()
FOREACH (_plugin_dir ${LIBRELOADED_PLUGINS})
    GET_FILENAME_COMPONENT(_plugin_path "${_plugin_dir}" ABSOLUTE BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
    INCLUDE("${_plugin_path}/plugin.cmake")
ENDFOREACH ()
CONFIGURE_FILE(cmake/reloaded_plugins.gen.cpp.in "${CMAKE_CURRENT_BINARY_DIR}/reloaded_plugins.gen.cpp" @ONLY)
TARGET_SOURCES(${LIBNAME} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}/reloaded_plugins.gen.cpp")
CONFIGURE_FILE(addons/libreloaded/libreloaded.gdextension.in
        "${PROJECT_SOURCE_DIR}/addons/libreloaded/libreloaded.gdextension" @ONLY)

IF (DOC_XML)
    IF (GODOTCPP_TARGET MATCHES "editor|template_debug")
        TARGET_DOC_SOURCES(${LIBNAME} "${DOC_XML}") #Must be in order for Godot to properly read all the documentation
    ENDIF ()
ENDIF ()

# For multi-config generators (like Visual Studio), ensure output directory doesn't have a config subfolder
GET_PROPERTY(IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
IF (IS_MULTI_CONFIG)
    FOREACH (CONFIG ${CMAKE_CONFIGURATION_TYPES})
        STRING(TOUPPER ${CONFIG} CONFIG)
        SET_TARGET_PROPERTIES(${LIBNAME} PROPERTIES
                LIBRARY_OUTPUT_DIRECTORY_${CONFIG} "${GODOT_PROJECT_BINARY_DIR}"
                RUNTIME_OUTPUT_DIRECTORY_${CONFIG} "${GODOT_PROJECT_BINARY_DIR}"
                ARCHIVE_OUTPUT_DIRECTORY_${CONFIG} "${GODOT_PROJECT_BINARY_DIR}"
        )
    ENDFOREACH ()
ENDIF ()


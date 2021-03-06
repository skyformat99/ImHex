cmake_minimum_required(VERSION 3.16)

project(imhex VERSION 1.6.1)

if (WIN32)
    set(EXTRA_MAGICDBS "" CACHE STRING "List of compiled magicdb files separated with a ;")
else()
    set(EXTRA_MAGICDBS "" CACHE STRING "List of compiled magicdb files separated with a :")
endif()

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Using Release build type as it was left unset" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release")
endif()

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")

option (CREATE_PACKAGE "Create a package with CPack" OFF)

if (APPLE)
    option (CREATE_BUNDLE "Create a bundle on macOS" OFF)
endif()

set(CMAKE_FIND_PACKAGE_SORT_ORDER NATURAL)
set(CMAKE_FIND_PACKAGE_SORT_DIRECTION DEC)
set(CMAKE_CXX_STANDARD 20)

# Enforce that we use non system Python 3 on macOS.
set(Python_FIND_FRAMEWORK NEVER)

# Find packages
find_package(PkgConfig REQUIRED)
pkg_search_module(CRYPTO libcrypto)
if(NOT CRYPTO_FOUND)
    find_library(CRYPTO crypto REQUIRED)
endif()
pkg_search_module(CAPSTONE REQUIRED capstone)
find_package(OpenGL REQUIRED)
find_package(Python COMPONENTS Development)

add_subdirectory(external/llvm)

# Plugins
add_subdirectory(plugins/libimhex)

add_subdirectory(plugins/example)


if(Python_VERSION LESS 3)
    message(STATUS ${PYTHON_VERSION_MAJOR_MINOR})
    message(FATAL_ERROR "No valid version of Python 3 was found.")
endif()

pkg_search_module(MAGIC libmagic)
if(NOT MAGIC_FOUND)
    find_library(MAGIC magic REQUIRED)
endif()

# Add include directories
include_directories(include ${CRYPTO_INCLUDE_DIRS} ${CAPSTONE_INCLUDE_DIRS} ${MAGIC_INCLUDE_DIRS} ${Python_INCLUDE_DIRS})

# Get Python major and minor
string(REPLACE "." ";" PYTHON_VERSION_MAJOR_MINOR ${Python_VERSION})
list(REMOVE_AT PYTHON_VERSION_MAJOR_MINOR 2)
list(JOIN PYTHON_VERSION_MAJOR_MINOR "." PYTHON_VERSION_MAJOR_MINOR)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS} -DPYTHON_VERSION_MAJOR_MINOR=\"\\\"${PYTHON_VERSION_MAJOR_MINOR}\"\\\"")

# Detect current OS / System
if (WIN32)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DOS_WINDOWS -static-libstdc++ -static-libgcc -Wl,--allow-multiple-definition -static")
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -Wl,-subsystem,windows")
elseif(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DOS_MACOS")
elseif(UNIX AND NOT APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DOS_LINUX")
else()
    message(FATAL_ERROR "Unknown / unsupported system!")
endif()

# Detect 32 vs. 64 bit system
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DARCH_64_BIT")
elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DARCH_32_BIT")
endif()

if (IS_DIRECTORY "${CMAKE_SOURCE_DIR}/.git")
    # Get the current working branch
    execute_process(
            COMMAND git rev-parse --abbrev-ref HEAD
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_BRANCH
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # Get the latest abbreviated commit hash of the working branch
    execute_process(
            COMMAND git log -1 --format=%h
            WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
            OUTPUT_VARIABLE GIT_COMMIT_HASH
            OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGIT_COMMIT_HASH=\"\\\"${GIT_COMMIT_HASH}\"\\\"")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DGIT_BRANCH=\"\\\"${GIT_BRANCH}\"\\\"")
endif()

set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DRELEASE -DIMHEX_VERSION=\"\\\"${PROJECT_VERSION}\"\\\"")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -DDEBUG -DIMHEX_VERSION=\"\\\"${PROJECT_VERSION}-Debug\"\\\"")

if (WIN32)
    set(application_type WIN32)
    set(imhex_icon "${PROJECT_SOURCE_DIR}/res/resource.rc")

    if (CREATE_PACKAGE)
        set(CPACK_GENERATOR "WIX")
        set(CPACK_PACKAGE_NAME "ImHex")
        set(CPACK_WIX_UPGRADE_GUID "05000E99-9659-42FD-A1CF-05C554B39285")
        set(CPACK_WIX_PRODUCT_ICON "${PROJECT_SOURCE_DIR}/res/icon.ico")
        set(CPACK_PACKAGE_INSTALL_DIRECTORY "ImHex")
        set_property(INSTALL "$<TARGET_FILE_NAME:imhex>"
            PROPERTY CPACK_START_MENU_SHORTCUTS "ImHex"
        )
        set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/res/LICENSE.rtf")
    endif()
elseif (APPLE)
    set (imhex_icon "${PROJECT_SOURCE_DIR}/res/mac/AppIcon.icns")

    if (CREATE_BUNDLE)
        set(application_type MACOSX_BUNDLE)
        set_source_files_properties(${imhex_icon} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
        set(MACOSX_BUNDLE_ICON_FILE "AppIcon.icns")
        set(MACOSX_BUNDLE_INFO_STRING "WerWolv")
        set(MACOSX_BUNDLE_BUNDLE_NAME "ImHex")
        set(MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}.${PROJECT_VERSION_PATCH}")
        set(MACOSX_BUNDLE_GUI_IDENTIFIER "WerWolv.ImHex")
        set(MACOSX_BUNDLE_LONG_VERSION_STRING "${PROJECT_VERSION}-${GIT_COMMIT_HASH}")
        set(MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}")
        set(MACOSX_BUNDLE_COPYRIGHT "Copyright © 2020 WerWolv and Thog. All rights reserved." )
        if ("${CMAKE_GENERATOR}" STREQUAL "Xcode")
            set ( bundle_path "${CMAKE_BINARY_DIR}/${CMAKE_BUILD_TYPE}/imhex.app" )
        else ()
            set ( bundle_path "${CMAKE_BINARY_DIR}/imhex.app" )
        endif()
    endif()
endif()

add_executable(imhex ${application_type}
        source/main.cpp
        source/window.cpp

        source/helpers/crypto.cpp
        source/helpers/patches.cpp
        source/helpers/math_evaluator.cpp
        source/helpers/project_file_handler.cpp
        source/helpers/loader_script_handler.cpp
        source/helpers/plugin_handler.cpp

        source/providers/file_provider.cpp

        source/views/view_hexeditor.cpp
        source/views/view_pattern.cpp
        source/views/view_pattern_data.cpp
        source/views/view_hashes.cpp
        source/views/view_information.cpp
        source/views/view_help.cpp
        source/views/view_tools.cpp
        source/views/view_strings.cpp
        source/views/view_data_inspector.cpp
        source/views/view_disassembler.cpp
        source/views/view_bookmarks.cpp
        source/views/view_patches.cpp
        source/views/view_command_palette.cpp
        source/views/view_settings.cpp

        ${imhex_icon}
        )

set_target_properties(imhex PROPERTIES CXX_VISIBILITY_PRESET hidden)

target_link_directories(imhex PRIVATE ${CRYPTO_LIBRARY_DIRS} ${CAPSTONE_LIBRARY_DIRS} ${MAGIC_LIBRARY_DIRS})

if (WIN32)
    target_link_libraries(imhex libdl.a libmagic.a libgnurx.a libtre.a libintl.a libiconv.a libshlwapi.a libcrypto.a libwinpthread.a libcapstone.a LLVMDemangle libimhex ${Python_LIBRARIES})
elseif (UNIX)
    target_link_libraries(imhex magic crypto ${CMAKE_DL_LIBS} capstone LLVMDemangle libimhex ${Python_LIBRARIES} dl)
endif()

add_custom_command(TARGET imhex POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:libimhex>
        $<TARGET_FILE_DIR:imhex>)

if (WIN32)
    # Install binaries directly in the prefix, usually C:\Program Files\ImHex.
    set(CMAKE_INSTALL_BINDIR ".")

    # Grab all dynamically linked dependencies.
    INSTALL(CODE "set(CMAKE_INSTALL_BINDIR \"${CMAKE_INSTALL_BINDIR}\")")
    INSTALL(CODE "get_filename_component(PY_PARENT ${Python_LIBRARIES} DIRECTORY)")
    INSTALL(CODE "LIST(APPEND DEP_FOLDERS \${PY_PARENT})")
    install(CODE [[
        file(GET_RUNTIME_DEPENDENCIES
            EXECUTABLES $<TARGET_FILE:imhex>
            RESOLVED_DEPENDENCIES_VAR _r_deps
            UNRESOLVED_DEPENDENCIES_VAR _u_deps
            CONFLICTING_DEPENDENCIES_PREFIX _c_deps
            DIRECTORIES ${DEP_FOLDERS}
            POST_EXCLUDE_REGEXES ".*system32/.*\\.dll"
        )

        if(_u_deps)
            message(WARNING "There were unresolved dependencies for binary $<TARGET_FILE:imhex>: \"${_u_deps}\"!")
        endif()
        if(_c_deps_FILENAMES)
            message(WARNING "There were conflicting dependencies for library $<TARGET_FILE:imhex>: \"${_c_deps}\"!")
        endif()

        foreach(_file ${_r_deps})
            file(INSTALL
                DESTINATION "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}"
                TYPE SHARED_LIBRARY
                FOLLOW_SYMLINK_CHAIN
                FILES "${_file}"
                )
        endforeach()
    ]])
endif()

# Compile the imhex-specific magicdb
add_custom_target(magic_dbs ALL
    SOURCES magic_dbs/nintendo_magic
    )
add_custom_command(TARGET magic_dbs
    COMMAND file -C -m ${CMAKE_SOURCE_DIR}/magic_dbs
    )

# Install the magicdb files.
install(FILES ${CMAKE_CURRENT_BINARY_DIR}/magic_dbs.mgc DESTINATION magic/ RENAME imhex.mgc)
install(FILES ${EXTRA_MAGICDBS} DESTINATION magic/)

if (CREATE_BUNDLE)
    include(PostprocessBundle)

    # Fix rpath
    add_custom_command(TARGET imhex POST_BUILD COMMAND ${CMAKE_INSTALL_NAME_TOOL} -add_rpath "@executable_path/../Frameworks/" $<TARGET_FILE:imhex>)

    # FIXME: Remove this once we move/integrate the plugins directory.
    add_custom_target(build-time-make-plugins-directory ALL COMMAND ${CMAKE_COMMAND} -E make_directory "${bundle_path}/Contents/MacOS/plugins")

    # Update library references to make the bundle portable
    postprocess_bundle(imhex)

    # Enforce DragNDrop packaging.
    set(CPACK_GENERATOR "DragNDrop")

    install(TARGETS imhex BUNDLE DESTINATION .)
else()
    install(TARGETS imhex RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()


if (CREATE_PACKAGE)
    include(apple)
    include(CPack)
endif()
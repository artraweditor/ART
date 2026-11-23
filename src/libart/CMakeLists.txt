# libART: a C library to run the commands of the server mode of ART
# (see doc/libart.md). The remote backend is plain C99 with no dependencies;
# the in-process backend (ART_LIBART_EMBED) links the ART engine.

set(ART_LIBART_VERSION "1.26.12")

set(ART_LIBART_C_SOURCES art_core.c art_remote.c)

find_package(Threads REQUIRED)

# the library: shared (for the bindings) and static
add_library(art-lib SHARED ${ART_LIBART_C_SOURCES})
add_library(art-lib-static STATIC ${ART_LIBART_C_SOURCES})

foreach(t art-lib art-lib-static)
    set_target_properties(${t} PROPERTIES
        OUTPUT_NAME ART
        C_STANDARD 99
        C_STANDARD_REQUIRED ON
        C_VISIBILITY_PRESET hidden
        CXX_VISIBILITY_PRESET hidden
        VISIBILITY_INLINES_HIDDEN ON
        POSITION_INDEPENDENT_CODE ON)
    target_include_directories(${t} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
    target_compile_definitions(${t} PRIVATE
        ART_LIBART_VERSION="${ART_LIBART_VERSION}")
    target_link_libraries(${t} PRIVATE Threads::Threads)
    if(WIN32)
        target_link_libraries(${t} PRIVATE ws2_32) # the remote backend
    endif()
endforeach()
target_compile_definitions(art-lib PRIVATE ART_BUILDING_LIB)
# export only the ART_* functions of the shared library: with the engine
# inside, everything else must stay private
if(APPLE)
    target_link_options(art-lib PRIVATE
        "LINKER:-exported_symbols_list,${CMAKE_CURRENT_SOURCE_DIR}/art.exports")
elseif(UNIX)
    target_link_options(art-lib PRIVATE
        "LINKER:--version-script=${CMAKE_CURRENT_SOURCE_DIR}/art.map")
endif()
target_compile_definitions(art-lib-static PUBLIC ART_STATIC_LIB)

# the in-process backend: the engine of ART inside the shared library (the
# static library stays remote-only, as its users would have to link the
# engine themselves)
option(ART_LIBART_EMBED "libART: build the in-process backend into the shared library" ON)
if(ART_LIBART_EMBED)
    # the sources and libraries of ART-cli, which need not be built itself
    set(_embed_srcs art_embed.cc)
    foreach(f ${ART_CLI_SOURCES})
        if(NOT f MATCHES "main-cli\\.cc|servermode\\.cc|\\.rc$")
            list(APPEND _embed_srcs ${CMAKE_SOURCE_DIR}/src/gui/${f})
        endif()
    endforeach()
    target_sources(art-lib PRIVATE ${_embed_srcs})
    target_compile_definitions(art-lib PRIVATE ART_LIBART_EMBED)
    # include_directories() and link_directories() are per directory: take
    # those of the programs
    get_directory_property(_gui_incdirs DIRECTORY ${CMAKE_SOURCE_DIR}/src/gui INCLUDE_DIRECTORIES)
    target_include_directories(art-lib PRIVATE ${_gui_incdirs})
    get_directory_property(_gui_linkdirs DIRECTORY ${CMAKE_SOURCE_DIR}/src/gui LINK_DIRECTORIES)
    target_link_directories(art-lib PRIVATE ${_gui_linkdirs})
    target_link_libraries(art-lib PRIVATE ${ART_CLI_LIBRARIES})
    set_target_properties(art-lib PROPERTIES LINKER_LANGUAGE CXX)
    add_dependencies(art-lib UpdateInfo)
endif()

# tests: `ctest -R libart` (build them with `ninja art-libart-test`)
add_executable(art-libart-test EXCLUDE_FROM_ALL test_libart.c)
set_target_properties(art-libart-test PROPERTIES C_STANDARD 99)
target_link_libraries(art-libart-test PRIVATE art-lib-static Threads::Threads)
add_test(NAME libart COMMAND art-libart-test)

# test of the in-process backend: needs data files and an image, see the file
if(ART_LIBART_EMBED)
    add_executable(art-libart-embed-test EXCLUDE_FROM_ALL test_embed.c)
    set_target_properties(art-libart-embed-test PROPERTIES C_STANDARD 99)
    target_link_libraries(art-libart-embed-test PRIVATE art-lib)
    add_test(NAME libart-embed COMMAND art-libart-embed-test)
    set_tests_properties(libart-embed PROPERTIES SKIP_RETURN_CODE 77)
endif()

# a C++ translation unit including the header: it must compile as C++ too
add_library(art-libart-header-test OBJECT EXCLUDE_FROM_ALL test_header.cc)
target_include_directories(art-libart-header-test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})

# an example, and a tool to test the library
add_executable(art-lib-cmd EXCLUDE_FROM_ALL examples/art_cmd.c)
set_target_properties(art-lib-cmd PROPERTIES C_STANDARD 99 OUTPUT_NAME art_cmd)
target_link_libraries(art-lib-cmd PRIVATE art-lib)

# Installation: the shared library, its header, its documentation and a
# pkg-config file.
#  - bundles (macOS, Windows, Linux AppImage-style): a relocatable tree
#      <DATADIR>/libART/{lib,include,doc}   (DATADIR is Contents/Resources on macOS)
#    with the pkg-config file in libART/lib/pkgconfig, which locates everything
#    relative to itself (${pcfiledir}); the tools/*/bundle_ART.py scripts add
#    the libraries the library depends on
#  - otherwise (system installs): lib/, include/, share/doc/ART/ and
#    lib/pkgconfig of the prefix
option(ART_INSTALL_LIBART "Install libART, its header, documentation and pkg-config file" ON)
if(ART_INSTALL_LIBART)
    if(BUILD_BUNDLE)
        set(_libart_root "${DATADIR}/libART")
        set(_libart_lib "${_libart_root}/lib")
        set(_libart_include "${_libart_root}/include")
        set(_libart_doc "${_libart_root}/doc")
        set(ART_PC_PREFIX "\${pcfiledir}/../..")
        set(ART_PC_LIBDIR "\${prefix}/lib")
        # the libraries the bundle copies are found from the installed library
        if(APPLE)
            set(_libart_rpath "@loader_path/../../../Frameworks")
        elseif(UNIX)
            set(_libart_rpath "\$ORIGIN/../../lib")
        endif()
    else()
        set(_libart_lib "${LIBDIR}")
        set(_libart_include "${CMAKE_INSTALL_PREFIX}/include")
        set(_libart_doc "${DOCDIR}")
        set(ART_PC_PREFIX "${CMAKE_INSTALL_PREFIX}")
        set(ART_PC_LIBDIR "${LIBDIR}")
    endif()
    if(WIN32)
        set(ART_PC_RPATH "")
    else()
        set(ART_PC_RPATH "-Wl,-rpath,\${libdir}")
    endif()
    configure_file(ART.pc.in ${CMAKE_CURRENT_BINARY_DIR}/ART.pc @ONLY)

    if(_libart_rpath)
        set_target_properties(art-lib PROPERTIES INSTALL_RPATH "${_libart_rpath}")
    endif()
    install(TARGETS art-lib
        LIBRARY DESTINATION ${_libart_lib}
        ARCHIVE DESTINATION ${_libart_lib}
        RUNTIME DESTINATION ${_libart_lib})
    install(FILES ART.h DESTINATION ${_libart_include})
    install(FILES ${CMAKE_SOURCE_DIR}/doc/libart.md
                  ${CMAKE_SOURCE_DIR}/doc/server_mode.md
            DESTINATION ${_libart_doc})
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/ART.pc
            DESTINATION ${_libart_lib}/pkgconfig)
endif()

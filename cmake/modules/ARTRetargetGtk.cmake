# macOS: load only one GTK.
#
# The linker takes libgtk-3/libgdk-3 from the first link directory that has
# them (a self-built GTK in /opt/local, say: what PKG_CONFIG_PATH selects), but
# gtkmm, gdkmm and gtk-mac-integration record the absolute path of the GTK they
# were built against (Homebrew's): both GTKs are then loaded, their types clash
# and the GUI hangs at start-up. When that is the case, tools/osx/retarget_gtk.py
# makes private copies of those libraries that use the chosen GTK, in
# ${CMAKE_BINARY_DIR}/gtk-retarget, and that directory goes first in the link
# path. Nothing happens if the libraries already agree (a plain Homebrew or
# MacPorts install), or with -DART_RETARGET_GTK=OFF.
#
# art_retarget_gtk(): call it once in src/gui, after the link directories are
# set and before the targets are created. If there is something to do it
# defines the target art_gtk_retarget, which what links GTK must depend on.

option(ART_RETARGET_GTK "macOS: make sure that only the GTK picked by CMake is loaded" ON)

function(art_retarget_gtk)
    if(NOT APPLE OR NOT ART_RETARGET_GTK)
        return()
    endif()
    find_package(Python3 QUIET COMPONENTS Interpreter)
    if(NOT Python3_Interpreter_FOUND)
        message(STATUS "GTK: no Python 3, not checking for several GTKs")
        return()
    endif()
    set(_script "${CMAKE_SOURCE_DIR}/tools/osx/retarget_gtk.py")
    get_directory_property(_dirs LINK_DIRECTORIES)
    string(REPLACE ";" ":" _dirs "${_dirs}")
    execute_process(
        COMMAND ${Python3_EXECUTABLE} -I ${_script} --libdirs "${_dirs}" --list
        RESULT_VARIABLE _rc
        OUTPUT_VARIABLE _libs
        ERROR_VARIABLE _info
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_STRIP_TRAILING_WHITESPACE)
    if(NOT _rc EQUAL 0)
        message(WARNING "GTK: cannot check for several GTKs: ${_info}")
        return()
    endif()
    string(REPLACE "\n" ";" _libs "${_libs}")
    if(NOT _libs)
        message(STATUS "${_info} (the GUI libraries agree)")
        return()
    endif()
    set(_dir "${CMAKE_BINARY_DIR}/gtk-retarget")
    message(STATUS "${_info}: the GUI libraries that use another GTK are copied to ${_dir}")
    add_custom_command(
        OUTPUT "${_dir}/.stamp"
        COMMAND ${Python3_EXECUTABLE} -I ${_script} --libdirs "${_dirs}" --build "${_dir}"
        DEPENDS ${_libs} ${_script}
        COMMENT "Making the GUI libraries use the GTK that is linked"
        VERBATIM)
    add_custom_target(art_gtk_retarget DEPENDS "${_dir}/.stamp")
    # first: what is linked by name (-lgtkmm-3.0) is taken from here
    link_directories(BEFORE "${_dir}")
endfunction()

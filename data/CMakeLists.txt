file(GLOB LANGUAGEFILES "languages/*")
file(GLOB SOUNDFILES "sounds/*")
file(GLOB INPUTICCFILES "iccprofiles/input/*")
file(GLOB OUTPUTICCFILES "iccprofiles/output/*")
file(GLOB DCPFILES "dcpprofiles/*")
file(GLOB FONTS "fonts/*")

set(PROFILESDIR "profiles")
set(THEMEDIR "themes")

set(CTLSCRIPTSDIR "ctlscripts")
set(LUTSDIR "luts")


file(GLOB IMG_SVG LIST_DIRECTORIES false "images/svg/*")
file(GLOB IMG_PNG LIST_DIRECTORIES false "images/png/*")
file(GLOB IMG_ICO LIST_DIRECTORIES false "images/*")

if(WIN32)
    set(OPTIONSFILE "options/options.win")
elseif(APPLE)
    set(OPTIONSFILE "options/options.osx")
else()
    set(OPTIONSFILE "options/options.lin")
endif()

if(WIN32)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/WindowsInnoSetup.iss" DESTINATION ${BINDIR})
endif()

if(UNIX)
    configure_file("${CMAKE_CURRENT_SOURCE_DIR}/ART.desktop.in" "${CMAKE_CURRENT_BINARY_DIR}/ART.desktop")
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/ART.desktop" DESTINATION ${DESKTOPDIR})
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/us.pixls.art.ART.metainfo.xml" DESTINATION "${APPDATADIR}")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/images/png/ART-logo-16.png" DESTINATION "${ICONSDIR}/hicolor/16x16/apps" RENAME ART.png)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/images/png/ART-logo-24.png" DESTINATION "${ICONSDIR}/hicolor/24x24/apps" RENAME ART.png)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/images/png/ART-logo-48.png" DESTINATION "${ICONSDIR}/hicolor/48x48/apps" RENAME ART.png)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/images/png/ART-logo-128.png" DESTINATION "${ICONSDIR}/hicolor/128x128/apps" RENAME ART.png)
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/images/png/ART-logo-256.png" DESTINATION "${ICONSDIR}/hicolor/256x256/apps" RENAME ART.png)
endif()

install(FILES ${LANGUAGEFILES} DESTINATION "${DATADIR}/languages" COMPONENT libart_data)
install(FILES ${SOUNDFILES} DESTINATION "${DATADIR}/sounds")
install(FILES ${INPUTICCFILES} DESTINATION "${DATADIR}/iccprofiles/input" COMPONENT libart_data)
install(FILES ${OUTPUTICCFILES} DESTINATION "${DATADIR}/iccprofiles/output" COMPONENT libart_data)
install(FILES ${DCPFILES} DESTINATION "${DATADIR}/dcpprofiles" COMPONENT libart_data)
install(FILES "cammatrices.json" DESTINATION "${DATADIR}" COMPONENT libart_data)
install(FILES "dcraw.json" DESTINATION "${DATADIR}" COMPONENT libart_data)
install(FILES "rt.json" DESTINATION "${DATADIR}" COMPONENT libart_data)
install(FILES "wbpresets.json" DESTINATION "${DATADIR}" COMPONENT libart_data)
install(FILES ${OPTIONSFILE} DESTINATION "${DATADIR}" PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ RENAME options)
if(WIN32)
    install(FILES ${FONTS} DESTINATION "${DATADIR}/fonts")
endif()
install(FILES "dynamicprofile.cfg" DESTINATION "${DATADIR}" COMPONENT libart_data)


install(DIRECTORY ${PROFILESDIR} DESTINATION "${DATADIR}" COMPONENT libart_data FILES_MATCHING PATTERN "*.arp")
install(DIRECTORY ${THEMEDIR} DESTINATION "${DATADIR}")

install(FILES ${IMG_SVG} DESTINATION "${DATADIR}/images")
install(FILES ${IMG_PNG} DESTINATION "${DATADIR}/images")
install(FILES ${IMG_ICO} DESTINATION "${DATADIR}/images")
install(DIRECTORY "images/icons" DESTINATION "${DATADIR}/images")

install(DIRECTORY ${CTLSCRIPTSDIR} DESTINATION "${DATADIR}" COMPONENT libart_data FILES_MATCHING PATTERN "*.ctl")
install(DIRECTORY ${LUTSDIR} DESTINATION "${DATADIR}" COMPONENT libart_data FILES_MATCHING REGEX ".*[^~]$")

# The lensfun database is not part of the source tree: point
# LENSFUNDB_SOURCE_DIR at a version_1 directory (e.g. the one lensfun-update-data
# fills) to ship it as <DATADIR>/lensfun
set(LENSFUNDB_SOURCE_DIR "" CACHE PATH "Lensfun database (version_1 directory) to install as <DATADIR>/lensfun")
if(LENSFUNDB_SOURCE_DIR)
    install(DIRECTORY "${LENSFUNDB_SOURCE_DIR}/" DESTINATION "${DATADIR}/lensfun" COMPONENT libart_data)
endif()

# Only what libART (and ART-cli) needs at run time -- profiles, LUTs, CTL
# scripts, camera data (camconst.json comes from src/engine), ICC/DCP profiles,
# languages, the lensfun database -- without GUI icons, themes, sounds, fonts
# or the options file, into a plain directory:
#   cmake --install <builddir> --component libart_data --prefix <dir>
# (on macOS/Windows the files land in <dir>/Contents/Resources resp. <dir>, since
# DATADIR is relative to the prefix there)

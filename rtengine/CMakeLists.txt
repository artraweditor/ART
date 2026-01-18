include_directories(${EXTRA_INCDIR}
    ${EXPAT_INCLUDE_DIRS}
    ${FFTW3F_INCLUDE_DIRS}
    ${GLIB2_INCLUDE_DIRS}
    ${GLIBMM_INCLUDE_DIRS}
    ${GOBJECT_INCLUDE_DIRS}
    ${GTHREAD_INCLUDE_DIRS}
    ${GTKMM_INCLUDE_DIRS}
    ${GTK_INCLUDE_DIRS}
    ${LCMS_INCLUDE_DIRS}
    ${LENSFUN_INCLUDE_DIRS}
    ${RSVG_INCLUDE_DIRS}
    ${EXIV2_INCLUDE_DIRS}
    )

link_directories(
    ${EXPAT_LIBRARY_DIRS}
    ${EXTRA_LIBDIR}
    ${FFTW3F_LIBRARY_DIRS}
    ${GLIB2_LIBRARY_DIRS}
    ${GLIBMM_LIBRARY_DIRS}
    ${GOBJECT_LIBRARY_DIRS}
    ${GTHREAD_LIBRARY_DIRS}
    ${LCMS_LIBRARY_DIRS}
    ${LENSFUN_LIBRARY_DIRS}
    ${RSVG_LIBRARY_DIRS}
    )

if(LIBRAW_FOUND)
    include_directories(${LIBRAW_INCLUDE_DIRS})
    link_directories(${LIBRAW_LIBRARY_DIRS})
endif()

if(OCIO_FOUND)
    include_directories(${OCIO_INCLUDE_DIRS})
    link_directories(${OCIO_LIBRARY_DIRS})
endif()

if(CTL_FOUND)
    include_directories(${CTL_INCLUDE_DIRS})
    link_directories(${CTL_LIBRARY_DIRS})
endif()

if(SIMDE_INCLUDE_DIRS)
    include_directories("${SIMDE_INCLUDE_DIRS}")
endif()

set(CAMCONSTSFILE "camconst.json")

set(RTENGINESOURCEFILES
    badpixels.cc
    CA_correct_RT.cc
    FTblockDN.cc
    PF_correct_RT.cc
    alpha.cc
    ahd_demosaic_RT.cc
    amaze_demosaic_RT.cc
    cJSON.c
    calc_distort.cc
    camconst.cc
    cfa_linedn_RT.cc
    ciecam02.cc
    clutstore.cc
    color.cc
    colortemp.cc
    coord.cc
    cplx_wavelet_dec.cc
    curves.cc
    dcp.cc
    dcraw.cc
    dcrop.cc
    demosaic_algos.cc
    dfmanager.cc
    diagonalcurves.cc
    dual_demosaic_RT.cc
    dynamicprofile.cc
    eahd_demosaic.cc
    fast_demo.cc
    ffmanager.cc
    flatcurves.cc
    gauss.cc
    green_equil_RT.cc
    hilite_recon.cc
    hphd_demosaic_RT.cc
    iccjpeg.cc
    iccstore.cc
    iimage.cc
    image16.cc
    image8.cc
    imagedata.cc
    imagedimensions.cc
    imagefloat.cc
    imageio.cc
    improccoordinator.cc
    improcfun.cc
    impulse_denoise.cc
    init.cc
    iprgb2out.cc
    ipresize.cc
    ipsharpen.cc
    iptransform.cc
    rtjpeg.cc
    labimage.cc
    lcp.cc
    lmmse_demosaic.cc
    loadinitial.cc
    myfile.cc
    panasonic_decoders.cc
    pipettebuffer.cc
    pixelshift.cc
    previewimage.cc
    processingjob.cc
    procparams.cc
    profilestore.cc
    rawimage.cc
    rawimagesource.cc
    rcd_demosaic.cc
    refreshmap.cc
    rt_algo.cc
    rt_polygon.cc
    rtthumbnail.cc
    simpleprocess.cc
    ipspot.cc
    slicer.cc
    stdimagesource.cc
    utils.cc
    rtlensfun.cc
    tmo_fattal02.cc
    iplocalcontrast.cc
    histmatching.cc
    pdaflinesfilter.cc
    gamutwarning.cc
    iptoneequalizer.cc    
    ipsoftlight.cc
    xtrans_demosaic.cc
    vng4_demosaic_RT.cc
    ipsoftlight.cc
    guidedfilter.cc
    ipdehaze.cc
    ipcolorcorrection.cc
    lj92.c
    ipsmoothing.cc
    iplogenc.cc
    masks.cc
    ipgrain.cc
    ipdenoise.cc
    iptextureboost.cc
    metadata.cc
    iplabadjustments.cc
    perspectivecorrection.cc
    iphsl.cc
    ipchmixer.cc
    ipexposure.cc
    iprgbcurves.cc
    ipbw.cc
    ipsaturation.cc
    ipfilmsim.cc
    iptonecurve.cc
    deconvautoradius.cc
    filmnegativeproc.cc
    canon_cr3_decoder.cc
    rawimage_gainmap.cc
    subprocess.cc
    bayer_bilinear_demosaic.cc
    gainmap.cc
    base64.cc
    imgiomanager.cc
    lensexif.cc
    nlmeans.cc
    newdelete.cc
    compress.cc
    LUT3D.cc
    clutparams.cc
    )


if(LENSFUN_HAS_LOAD_DIRECTORY)
    set_source_files_properties(rtlensfun.cc PROPERTIES COMPILE_DEFINITIONS RT_LENSFUN_HAS_LOAD_DIRECTORY)
endif()

#set_source_files_properties(perspectivecorrection.cc PROPERTIES COMPILE_FLAGS -fpermissive)

if(WITH_BENCHMARK)
    add_definitions(-DBENCHMARK)
endif()

if(NOT WITH_SYSTEM_KLT)
    set(RTENGINESOURCEFILES ${RTENGINESOURCEFILES}
        klt/convolve.cc
        klt/error.cc
        klt/klt.cc
        klt/klt_util.cc
        klt/pnmio.cc
        klt/pyramid.cc
        klt/selectGoodFeatures.cc
        klt/storeFeatures.cc
        klt/trackFeatures.cc
        klt/writeFeatures.cc
        )
    set(KLT_LIBRARIES)
endif()

if(OCIO_FOUND)
    set(RTENGINESOURCEFILES ${RTENGINESOURCEFILES}
        extclut.cc
        )
endif()

include_directories(BEFORE "${CMAKE_CURRENT_BINARY_DIR}")

add_library(rtengine ${RTENGINESOURCEFILES})
add_dependencies(rtengine UpdateInfo)

# It may be nice to store library version too
if(BUILD_SHARED_LIBS)
    install(TARGETS rtengine DESTINATION ${LIBDIR})
endif()

set_target_properties(rtengine PROPERTIES COMPILE_FLAGS "${RTENGINE_CXX_FLAGS}")

target_link_libraries(rtengine
    ${EXPAT_LIBRARIES}
    ${EXTRA_LIB}
    ${FFTW3F_LIBRARIES}
    ${GLIB2_LIBRARIES}
    ${GLIBMM_LIBRARIES}
    ${GOBJECT_LIBRARIES}
    ${GTHREAD_LIBRARIES}
    ${JPEG_LIBRARIES}
    ${LCMS_LIBRARIES}
    ${PNG_LIBRARIES}
    ${TIFF_LIBRARIES}
    ${ZLIB_LIBRARIES}
    ${LENSFUN_LIBRARIES}
    ${RSVG_LIBRARIES}
    ${EXIV2_LIBRARIES}
    )
if(LCMS_FAST_FLOAT AND NOT LCMS_FAST_FLOAT_SYSTEM)
    target_link_libraries(rtengine lcms2_fast_float)
endif()
if(LIBRAW_FOUND)
    target_link_libraries(rtengine ${LIBRAW_LIBRARIES})
endif()
if(OCIO_FOUND)
    target_link_libraries(rtengine ${OCIO_LIBRARIES})
endif()
if(CTL_FOUND)
    target_link_libraries(rtengine ${CTL_LIBRARIES})
endif()

install(FILES ${CAMCONSTSFILE} DESTINATION "${DATADIR}" PERMISSIONS OWNER_WRITE OWNER_READ GROUP_READ WORLD_READ)

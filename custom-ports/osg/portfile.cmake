set(OSG_VER 3.6.5)

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO LaurensVoerman/OpenSceneGraph
    REF 3fc671e7cc3838c080e080c27c3b3c05724a5970
    SHA512 4cb58a85ba49214618b69facb5d13369fe476facc5146dd0844b9c3dd5f661a1eb91495ce01aadc12ee03cb3fbc6c4297548fb91698cbbc97a97cee14dbd5042
    HEAD_REF mine-3.6
    PATCHES
        link-libraries.patch
        collada.patch
        fix-sdl.patch
        fix-nvtt-squish.patch
        plugin-pdb-install.patch
        use-boost-asio.patch
        osgdb_zip_nozip.patch # This is fix symbol clashes with other libs when built in static-lib mode
        openexr4.patch
        unofficial-export.patch
		dicom_flags.patch
		vncExtraLibs.patch
		FindLzo.patch
		noIlk.patch
		colladaStatic.patch
		sdlStatic.patch
		viewerSDLstatic.patch
)

file(REMOVE
    "${SOURCE_PATH}/CMakeModules/FindFontconfig.cmake"
    "${SOURCE_PATH}/CMakeModules/FindFreetype.cmake"
    "${SOURCE_PATH}/CMakeModules/Findilmbase.cmake"
    "${SOURCE_PATH}/CMakeModules/FindOpenEXR.cmake"
    "${SOURCE_PATH}/CMakeModules/FindSDL2.cmake"
)
# IGNORE TRIPLET and build dll version
#set(OSGPKG_LIBRARY_LINKAGE "${VCPKG_LIBRARY_LINKAGE}")
set(OSGPKG_LIBRARY_LINKAGE "dynamic")
set(VCPKG_POLICY_DLLS_IN_STATIC_LIBRARY enabled)

string(COMPARE EQUAL "${OSGPKG_LIBRARY_LINKAGE}" "dynamic" OSG_DYNAMIC)

set(OPTIONS "")
set(MU_OPTIONS "")
if(VCPKG_TARGET_IS_WINDOWS)
    list(APPEND OPTIONS -DOSG_USE_UTF8_FILENAME=ON)
    list(APPEND MU_OPTIONS -DVCPKG_LIBRARY_LINKAGE=${VCPKG_LIBRARY_LINKAGE})
	
endif()
# Skip try_run checks
if(VCPKG_TARGET_IS_MINGW)
    list(APPEND OPTIONS -D_OPENTHREADS_ATOMIC_USE_WIN32_INTERLOCKED=0 -D_OPENTHREADS_ATOMIC_USE_GCC_BUILTINS=1)
elseif(VCPKG_TARGET_IS_WINDOWS)
    list(APPEND OPTIONS -D_OPENTHREADS_ATOMIC_USE_WIN32_INTERLOCKED=1 -D_OPENTHREADS_ATOMIC_USE_GCC_BUILTINS=0)
elseif(VCPKG_TARGET_IS_IOS)
    # handled by osg
elseif(VCPKG_CROSSCOMPILING)
    message(WARNING "Atomics detection may fail for cross builds. You can set osg cmake variables in a custom triplet.")
endif()

vcpkg_check_features(OUT_FEATURE_OPTIONS FEATURE_OPTIONS
    FEATURES
        tools       BUILD_OSG_APPLICATIONS
        examples    BUILD_OSG_EXAMPLES
        plugins     BUILD_OSG_PLUGINS_BY_DEFAULT
        plugins     CMAKE_REQUIRE_FIND_PACKAGE_CURL
        plugins     CMAKE_REQUIRE_FIND_PACKAGE_Jasper
        plugins     CMAKE_REQUIRE_FIND_PACKAGE_GDAL
        plugins     CMAKE_REQUIRE_FIND_PACKAGE_GTA
        packages    BUILD_OSG_PACKAGES
        docs        BUILD_DOCUMENTATION
        docs        BUILD_REF_DOCS_SEARCHENGINE
        docs        BUILD_REF_DOCS_TAGFILE
        fontconfig  OSG_TEXT_USE_FONTCONFIG
        freetype    BUILD_OSG_PLUGIN_FREETYPE
        freetype    CMAKE_REQUIRE_FIND_PACKAGE_Freetype
        collada     BUILD_OSG_PLUGIN_DAE
        collada     CMAKE_REQUIRE_FIND_PACKAGE_COLLADA
        nvtt        BUILD_OSG_PLUGIN_NVTT
        nvtt        CMAKE_REQUIRE_FIND_PACKAGE_NVTT
        openexr     BUILD_OSG_PLUGIN_EXR
        openexr     CMAKE_REQUIRE_FIND_PACKAGE_OpenEXR
        vnc         BUILD_OSG_PLUGIN_VNC
        vnc         CMAKE_REQUIRE_FIND_PACKAGE_LibVNCServer
        fbx         BUILD_OSG_PLUGIN_FBX
        fbx         CMAKE_REQUIRE_FIND_PACKAGE_FBX
        dicom       BUILD_OSG_PLUGIN_DICOM
        dicom       CMAKE_REQUIRE_FIND_PACKAGE_DCMTK
        lua         BUILD_OSG_PLUGIN_LUA
        rest-http-device BUILD_OSG_PLUGIN_RESTHTTPDEVICE
        sdl1        BUILD_OSG_PLUGIN_SDL
    INVERTED_FEATURES
        sdl1        CMAKE_DISABLE_FIND_PACKAGE_SDL # for apps and examples
)

# The package osg can be configured to use different OpenGL profiles via a custom triplet file:
# Possible values are GLCORE, GL2, GL3, GLES1, GLES2, GLES3, and GLES2+GLES3
if(NOT DEFINED osg_OPENGL_PROFILE)
    set(osg_OPENGL_PROFILE "GL3")
endif()

# Plugin control variables are used only if prerequisites are satisfied.
set(plugin_vars "")
file(STRINGS "${SOURCE_PATH}/src/osgPlugins/CMakeLists.txt" plugin_lines REGEX "ADD_PLUGIN_DIRECTORY")
foreach(line IN LISTS plugin_lines)
    if(NOT line MATCHES "ADD_PLUGIN_DIRECTORY\\(([^)]*)" OR NOT EXISTS "${SOURCE_PATH}/src/osgPlugins/${CMAKE_MATCH_1}/CMakeLists.txt")
        continue()
    endif()
    string(TOUPPER "${CMAKE_MATCH_1}" plugin_upper)
    list(APPEND plugin_vars "BUILD_OSG_PLUGIN_${plugin_upper}")
endforeach()

if(VCPKG_DETECTED_CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    set(VCPKG_C_FLAGS "${VCPKG_C_FLAGS} /source-charset:.1252")
    set(VCPKG_CXX_FLAGS "${VCPKG_CXX_FLAGS} /source-charset:.1252")
endif()

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    NO_CHARSET_FLAG
    OPTIONS
        ${FEATURE_OPTIONS}
        -DDYNAMIC_OPENSCENEGRAPH=${OSG_DYNAMIC}
        -DDYNAMIC_OPENTHREADS=${OSG_DYNAMIC}
        -DOSG_MSVC_VERSIONED_DLL=OFF
        -DOSG_DETERMINE_WIN_VERSION=OFF
        -DUSE_3RDPARTY_BIN=OFF
        -DBUILD_OSG_PLUGIN_OPENCASCADE=OFF
        -DBUILD_OSG_PLUGIN_INVENTOR=OFF
        -DBUILD_OSG_PLUGIN_DIRECTSHOW=OFF
        -DBUILD_OSG_PLUGIN_LAS=OFF
        -DBUILD_OSG_PLUGIN_QTKIT=OFF
        -DBUILD_OSG_PLUGIN_SVG=OFF
        -DOPENGL_PROFILE=${osg_OPENGL_PROFILE}
        -DBUILD_OSG_PLUGIN_ZEROCONFDEVICE=OFF
        -DBUILD_DASHBOARD_REPORTS=OFF
        -DCMAKE_CXX_STANDARD=11
        -DCMAKE_DISABLE_FIND_PACKAGE_FFmpeg=ON
        -DCMAKE_DISABLE_FIND_PACKAGE_GStreamer=ON
        -DCMAKE_DISABLE_FIND_PACKAGE_GLIB=ON
        -DCMAKE_DISABLE_FIND_PACKAGE_Inventor=ON
        -DPKG_CONFIG_USE_CMAKE_PREFIX_PATH=ON
        ${OPTIONS}
    OPTIONS_DEBUG
        -DBUILD_OSG_APPLICATIONS=OFF
        -DBUILD_OSG_EXAMPLES=OFF
        -DBUILD_DOCUMENTATION=OFF
    MAYBE_UNUSED_VARIABLES
        BUILD_REF_DOCS_SEARCHENGINE
        BUILD_REF_DOCS_TAGFILE
        OSG_DETERMINE_WIN_VERSION
        USE_3RDPARTY_BIN
        ${plugin_vars}
		${MU_OPTIONS}
)
vcpkg_cmake_install()
vcpkg_copy_pdbs()
configure_file("${CMAKE_CURRENT_LIST_DIR}/unofficial-osg-config.cmake" "${CURRENT_PACKAGES_DIR}/share/unofficial-osg/unofficial-osg-config.cmake" @ONLY)
vcpkg_cmake_config_fixup(PACKAGE_NAME unofficial-osg)

if(OSGPKG_LIBRARY_LINKAGE STREQUAL "static")
    file(APPEND "${CURRENT_PACKAGES_DIR}/include/osg/Config" "#ifndef OSG_LIBRARY_STATIC\n#define OSG_LIBRARY_STATIC 1\n#endif\n")
endif()

set(osg_plugins_subdir "osgPlugins-${OSG_VER}")
if("tools" IN_LIST FEATURES)
    set(osg_plugin_pattern "${VCPKG_TARGET_SHARED_LIBRARY_PREFIX}osgdb*${VCPKG_TARGET_SHARED_LIBRARY_SUFFIX}")
    file(GLOB osg_plugins "${CURRENT_PACKAGES_DIR}/plugins/${osg_plugins_subdir}/${osg_plugin_pattern}")
    file(INSTALL ${osg_plugins} DESTINATION "${CURRENT_PACKAGES_DIR}/tools/${PORT}/${osg_plugins_subdir}")
    if(NOT VCPKG_BUILD_TYPE)
        file(GLOB osg_plugins "${CURRENT_PACKAGES_DIR}/debug/plugins/${osg_plugins_subdir}/${osg_plugin_pattern}")
        file(INSTALL ${osg_plugins} DESTINATION "${CURRENT_PACKAGES_DIR}/tools/${PORT}/debug/${osg_plugins_subdir}")
    endif()

    set(tools osgversion present3D)
    if(OSGPKG_LIBRARY_LINKAGE STREQUAL "dynamic")
        list(APPEND tools osgviewer osgarchive osgconv osgfilecache)
    endif()
    vcpkg_copy_tools(TOOL_NAMES ${tools} AUTO_CLEAN)
endif()

file(REMOVE_RECURSE
    "${CURRENT_PACKAGES_DIR}/debug/include"
    "${CURRENT_PACKAGES_DIR}/debug/share"
)

vcpkg_replace_string("${CURRENT_PACKAGES_DIR}/lib/pkgconfig/openscenegraph.pc" "\\\n" " ")
if(NOT VCPKG_BUILD_TYPE)
    vcpkg_replace_string("${CURRENT_PACKAGES_DIR}/debug/lib/pkgconfig/openscenegraph.pc" "\\\n" " ")
endif()
vcpkg_fixup_pkgconfig()

file(COPY "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")
file(INSTALL "${SOURCE_PATH}/LICENSE.txt" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)

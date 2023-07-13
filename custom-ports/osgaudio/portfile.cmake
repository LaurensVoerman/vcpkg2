vcpkg_minimum_required(VERSION 2022-10-12) # for ${VERSION}
#vcpkg_find_cuda(OUT_CUDA_TOOLKIT_ROOT CUDA_TOOLKIT_ROOT OUT_CUDA_VERSION CUDA_VERSION)

set(OSG_VER 3.6.5)

#target for osgPlugins
set(osg_plugins_subdir "osgPlugins-${OSG_VER}")

vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO mccdo/osgaudio
    REF be872549c8452cbbf44ec8235dfa5a74566cb06d
    SHA512 a5503cac0346f6a3c2bfd801af626071bbc8602f4b41811065ff6c464219bf68e538a77ae7797687abc6e06554dfa07542f8257d192b280d2b262fd69865cd59
    HEAD_REF master
	PATCHES
	  audiobase.patch
	  example.patch
	  example_multiple.patch
	  findosg.patch

)

#find visual studio generator
set(generator "")
z_vcpkg_get_visual_studio_generator(OUT_GENERATOR generator OUT_ARCH generator_arch)

set(VCPKG_POLICY_DLLS_WITHOUT_EXPORTS enabled)

if(MSVC)
    set(VCPKG_C_FLAGS "${VCPKG_C_FLAGS} /source-charset:.1252")
    set(VCPKG_CXX_FLAGS "${VCPKG_CXX_FLAGS} /source-charset:.1252")
endif()

option( OSGAUDIO_INSTALL_DATA "Enable to add the data directory to the install target" ON )
vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
	NO_CHARSET_FLAG
    GENERATOR ${generator}
    OPTIONS
        -D0_ENABLE_SUBSYSTEM_OPENAL=ON
		-D0_ENABLE_SUBSYSTEM_FMOD=OFF
		-DOSGAUDIO_INSTALL_DATA=OFF
		-D0_BUILD_EXAMPLES_OSGAUDIO=OFF
		-D0_BUILD_EXAMPLES_OSGAUDIO_LOWLEVEL=OFF
	MAYBE_UNUSED_VARIABLES
	    CMAKE_DISABLE_FIND_PACKAGE_FFmpeg
		FETCHCONTENT_FULLY_DISCONNECTED
        CMAKE_INSTALL_BINDIR
        CMAKE_INSTALL_LIBDIR
        FETCHCONTENT_FULLY_DISCONNECTED
        _VCPKG_ROOT_DIR
)


vcpkg_cmake_install()
#vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/osgrc)

vcpkg_copy_pdbs()
vcpkg_fixup_pkgconfig()
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

if(MOVE_TOOLS)
#move exe files to "tools"
#not usefull without plugin dependencies
#set(tools viewer gui osgconvEx mkthumb)
#vcpkg_copy_tools(TOOL_NAMES ${tools} AUTO_CLEAN)

  file(GLOB osgrc_exe "${CURRENT_PACKAGES_DIR}/bin/*.exe")
  file(INSTALL ${osgrc_exe} DESTINATION "${CURRENT_PACKAGES_DIR}/tools/${PORT}")
  file(REMOVE ${osgrc_exe})

#if(NOT VCPKG_BUILD_TYPE)
#not for release only build
  file(GLOB osgrcd_exe "${CURRENT_PACKAGES_DIR}/debug/bin/*.exe")
  #message("osgrcd_exe: ${osgrcd_exe}")
  file(INSTALL ${osgrcd_exe} DESTINATION "${CURRENT_PACKAGES_DIR}/tools/${PORT}/debug/")
  file(REMOVE ${osgrcd_exe})
  #file(WRITE "${CURRENT_PACKAGES_DIR}/tools/notesD.txt" "VCPKG_BUILD_TYPE: ${VCPKG_BUILD_TYPE}\nosgrcd_exe: ${osgrcd_exe}\n")
#endif()
endif()
#move plugins

#set(osg_plugins osgdb_arg.dll osgdb_child.dll osgdb_crn.dll osgdb_dds.dll osgdb_mip.dll osgdb_ptfilter.dll osgdb_wmts.dll)
set(osg_plugin_pattern "${VCPKG_TARGET_SHARED_LIBRARY_PREFIX}osgdb_*${VCPKG_TARGET_SHARED_LIBRARY_SUFFIX}")
set(osg_plugindll_pattern "${VCPKG_TARGET_SHARED_LIBRARY_PREFIX}osg_*${VCPKG_TARGET_SHARED_LIBRARY_SUFFIX}")
file(GLOB osg_plugins "${CURRENT_PACKAGES_DIR}/bin/${osg_plugin_pattern}")
file(GLOB osg_plugin_dll "${CURRENT_PACKAGES_DIR}/bin/${osg_plugindll_pattern}")
file(INSTALL ${osg_plugins} DESTINATION "${CURRENT_PACKAGES_DIR}/tools/${PORT}/${osg_plugins_subdir}")
file(INSTALL ${osg_plugin_dll} DESTINATION "${CURRENT_PACKAGES_DIR}/tools/${PORT}/")
if(NOT VCPKG_BUILD_TYPE)
    file(GLOB osg_plugins "${CURRENT_PACKAGES_DIR}/debug/bin/${osg_plugin_pattern}")
    file(GLOB osg_plugin_dll "${CURRENT_PACKAGES_DIR}/bin/${osg_plugindll_pattern}")
    file(INSTALL ${osg_plugins} DESTINATION "${CURRENT_PACKAGES_DIR}/tools/${PORT}/debug/${osg_plugins_subdir}")
    file(INSTALL ${osg_plugin_dll} DESTINATION "${CURRENT_PACKAGES_DIR}/tools/${PORT}/debug/")
endif()

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/share")

file(INSTALL "${SOURCE_PATH}/COPYING"
     DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright
)

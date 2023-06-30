set(OSGRC_VER 3185)
set(OSG_VER 3.6.5)

#target for osgPlugins
set(osg_plugins_subdir "osgPlugins-${OSG_VER}")
message("${osg_plugins_subdir}")

vcpkg_from_git(
    OUT_SOURCE_PATH SOURCE_PATH
    URL E:\\osg\\36\\laurens\\osgRC
    REF 879005dc609c6cf8faa43529a3613b5a5003c2d4
    HEAD_REF main
)

file(WRITE "${SOURCE_PATH}\\src\\icons\\revision.bat" "EXIT 0")
file(WRITE "${SOURCE_PATH}\\src\\revision.h" "#define OSG_RC_REVISION \"${OSGRC_VER}\"\n#define OSG_RC_REVISION_UNQUOTE 3176\n#define OSG_RC_REVISION_UNQUOTE_SVN_VERSION ${OSG_VER}\n#define OSG_RC_REVISION_MOD 0\n")

string(COMPARE EQUAL "${VCPKG_LIBRARY_LINKAGE}" "dynamic" VNC_DYNAMIC)

#find visual studio generator
set(generator "")
z_vcpkg_get_visual_studio_generator(OUT_GENERATOR generator OUT_ARCH generator_arch)

set(VCPKG_POLICY_DLLS_WITHOUT_EXPORTS enabled)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    GENERATOR ${generator}
    OPTIONS
        -DCMAKE_DISABLE_FIND_PACKAGE_FFmpeg=ON
	MAYBE_UNUSED_VARIABLES
		WITH_SYSTEMD
)


vcpkg_cmake_install()
#vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/osgrc)

vcpkg_copy_pdbs()
vcpkg_fixup_pkgconfig()
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

#move exe files to "tools"
#not usefull without plugin dependencies
#set(tools viewer gui osgconvEx mkthumb)
#vcpkg_copy_tools(TOOL_NAMES ${tools} AUTO_CLEAN)

  file(GLOB osgrc_exe "${CURRENT_PACKAGES_DIR}/bin/*.exe")
  file(INSTALL ${osgrc_exe} DESTINATION "${CURRENT_PACKAGES_DIR}/tools/${PORT}")
  file(REMOVE ${osgrc_exe})
  #file(WRITE "${CURRENT_PACKAGES_DIR}/tools/notes.txt" "VCPKG_BUILD_TYPE: ${VCPKG_BUILD_TYPE}\nosgrc_exe: ${osgrc_exe}\n")

#if(NOT VCPKG_BUILD_TYPE)
#not for release only build
  file(GLOB osgrcd_exe "${CURRENT_PACKAGES_DIR}/debug/bin/*.exe")
  #message("osgrcd_exe: ${osgrcd_exe}")
  file(INSTALL ${osgrcd_exe} DESTINATION "${CURRENT_PACKAGES_DIR}/tools/${PORT}/debug/")
  file(REMOVE ${osgrcd_exe})
  #file(WRITE "${CURRENT_PACKAGES_DIR}/tools/notesD.txt" "VCPKG_BUILD_TYPE: ${VCPKG_BUILD_TYPE}\nosgrcd_exe: ${osgrcd_exe}\n")
#endif()

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

file(INSTALL "${SOURCE_PATH}/todo.txt"
     DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright
)

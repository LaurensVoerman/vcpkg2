vcpkg_minimum_required(VERSION 2022-10-12) # for ${VERSION}

set(OSGRC_VER ${VERSION})
set(OSG_VER 3.6.5)

#target for osgPlugins
set(osg_plugins_subdir "osgPlugins-${OSG_VER}")

if(IS_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/osgplugins/)
  execute_process(
  COMMAND "${CMAKE_COMMAND}" -E tar cjf ${DOWNLOADS}/osgplugins_${OSGRC_VER}.tar.bz2 ${CMAKE_CURRENT_LIST_DIR}/osgplugins/ 
  	WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR} 
  	OUTPUT_VARIABLE VAR_OUT 
  	ERROR_VARIABLE VAR_ERR )
  #message(VAR_OUT "${VAR_OUT}")
  #message(VAR_ERR "${VAR_ERR}")
endif()

find_file(archive osgplugins_${OSGRC_VER}.tar.bz2 PATHS ${DOWNLOADS})

IF(NOT archive)
message("NOT archive ${archive}")
SET(CHECKSUM "d37151be4019ade3643a84bb67501e0b6868fd3ddb3bf0f37dbaed2215a2178bd6b21b4a6d2c102ed529d29fcd37cfe76523e6f6885f344a63bfd683b525aa27")
ELSE()
message("archive ${archive}")
execute_process(COMMAND certutil -hashfile ${archive} SHA512 
COMMAND findstr -V CertUtil
COMMAND findstr -V SHA512
OUTPUT_VARIABLE CHECKSUM
OUTPUT_STRIP_TRAILING_WHITESPACE
)
message(CHECKSUM "-${CHECKSUM}-")
ENDIF()
#error
#message("ERROR HERE...")
#execute_process(COMMAND 7ZA ECHO_OUTPUT_VARIABLE VAR_OUT ERROR_VARIABLE VAR_ERR  ECHO_ERROR_VARIABLE)


#https://drive.google.com/file/d/1FAP4BpKULoBiqgOHTZ68qJzJZrb10AUY/view?usp=sharing
#cannot find a direct download link for drive,
#using unishare...
if(NOT archive)
vcpkg_download_distfile(
    archive # "archive" is set to the path to the downloaded file
    URLS "https://unishare.nl/index.php/s/4L4JkXAeX5kNJ49/download/osgplugins_${OSGRC_VER}.tar.bz2"
    FILENAME "osgplugins_${OSGRC_VER}.tar.bz2"
    SHA512 ${CHECKSUM}
)
else()
message("archive ${archive}")
execute_process(COMMAND certutil -hashfile ${archive} SHA512)
#return()
endif()

vcpkg_extract_source_archive(
    SOURCE_PATH
    ARCHIVE ${archive}
)

file(WRITE "${SOURCE_PATH}\\src\\revision.h" "#define OSG_RC_REVISION \"${OSGRC_VER}\"\n#define OSG_RC_REVISION_UNQUOTE ${OSGRC_VER}\n#define OSG_RC_REVISION_MOD 0\n")

#string(COMPARE EQUAL "${VCPKG_LIBRARY_LINKAGE}" "dynamic" VNC_DYNAMIC)
set(VCPKG_POLICY_DLLS_IN_STATIC_LIBRARY enabled)

#find visual studio generator
set(generator "")
z_vcpkg_get_visual_studio_generator(OUT_GENERATOR generator OUT_ARCH generator_arch)

set(VCPKG_POLICY_DLLS_WITHOUT_EXPORTS enabled)
set(VCPKG_POLICY_DLLS_WITHOUT_LIBS enabled)
set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    GENERATOR ${generator}
    OPTIONS
        -DVCPKG_LIBRARY_LINKAGE=${VCPKG_LIBRARY_LINKAGE}
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

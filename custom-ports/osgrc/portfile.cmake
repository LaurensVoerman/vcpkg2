set(LIBVNCSERVER_FULL_VERSION 0.9.14)

vcpkg_from_git(
    OUT_SOURCE_PATH SOURCE_PATH
    URL E:\\osg\\36\\laurens\\osgRC
    REF 098e296bcd103cee27b9aac042878bc542a26e1a
    HEAD_REF main
)

file(WRITE "${SOURCE_PATH}\\src\\icons\\revision.bat" "EXIT 0")
file(WRITE "${SOURCE_PATH}\\src\\revision.h" "#define OSG_RC_REVISION \"3176\"\n#define OSG_RC_REVISION_UNQUOTE 3176\n#define OSG_RC_REVISION_UNQUOTE_SVN_VERSION 3176\n#define OSG_RC_REVISION_MOD 0\n")

string(COMPARE EQUAL "${VCPKG_LIBRARY_LINKAGE}" "dynamic" VNC_DYNAMIC)
vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DWITH_FFMPEG=OFF
	MAYBE_UNUSED_VARIABLES
		WITH_SYSTEMD
)


vcpkg_cmake_install()
vcpkg_cmake_config_fixup(CONFIG_PATH lib/cmake/osgrc)

vcpkg_copy_pdbs()
vcpkg_fixup_pkgconfig()
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")

file(INSTALL "${SOURCE_PATH}/README.md"
     DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright
)

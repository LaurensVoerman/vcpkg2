
file(WRITE "${SOURCE_PATH}\\CMakeLists.txt" "FIND_PACKAGE(osgAudio)\n")

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


file(INSTALL "${SOURCE_PATH}/todo.txt"
     DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright
)

vcpkg_from_github(
	OUT_SOURCE_PATH SOURCE_PATH
	REPO RuGCiTViz/osgrc
	REF 098e296bcd103cee27b9aac042878bc542a26e1a
	SHA512 0
	HEAD_REF main
    PATCHES
        fixup.patch
)

# Debug build
if (NOT VCPKG_BUILD_TYPE OR VCPKG_BUILD_TYPE STREQUAL "debug")
	set(ENV{PKG_CONFIG_PATH} "${CURRENT_INSTALLED_DIR}/debug/lib/pkgconfig")
endif()

# Release build
if (NOT VCPKG_BUILD_TYPE OR VCPKG_BUILD_TYPE STREQUAL "release")
	set(ENV{PKG_CONFIG_PATH} "${CURRENT_INSTALLED_DIR}/lib/pkgconfig")
endif()
string(COMPARE EQUAL "${VCPKG_LIBRARY_LINKAGE}" "dynamic" FOX_DYNAMIC)
vcpkg_cmake_configure(
	SOURCE_PATH "${SOURCE_PATH}"
	OPTIONS
		-DVCPKG_HOST_TRIPLET=${HOST_TRIPLET} # for host pkgconf in PATH
        -DBUILD_SHARED_LIBS=${FOX_DYNAMIC}
)

vcpkg_cmake_install()
vcpkg_copy_pdbs()
vcpkg_fixup_pkgconfig()
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
configure_file("${SOURCE_PATH}/LICENSE" "${CURRENT_PACKAGES_DIR}/share/fox-toolkit/copyright" COPYONLY)
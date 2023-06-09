set(FOXTOOLKIT_FULL_VERSION 1.6.57)
#LINK ERRORS For static lib if jpeg/tiff etc are ON
if(VCPKG_LIBRARY_LINKAGE STREQUAL "dynamic")
    message("FOX forced to static library linkage")
    set(VCPKG_LIBRARY_LINKAGE static)
endif()

# direct download from fox-toolkit has no CMakeLists
#vcpkg_download_distfile(
#    archive # "archive" is set to the path to the downloaded file
#    URLS "http://fox-toolkit.org/ftp/fox-${FOXTOOLKIT_FULL_VERSION}.zip"
#    FILENAME "fox-${FOXTOOLKIT_FULL_VERSION}.zip"
#    SHA512 585e87449686918de20935e8608d6c3d8c2783803e4114742ef10c0fcbc4431b67fb2dc4eb46d98945ea0cba6be0e6dd958bee31dad63c0ca0dc7f2476610659
#)
#vcpkg_extract_source_archive(
#    src # "src" is set to the path to the extracted files
#    ARCHIVE "${archive}"
#    SOURCE_BASE fox-${FOXTOOLKIT_FULL_VERSION}.zip
#    PATCHES
#        
#)

#could try github mirror: release/1.6 branch
#https://github.com/devinsmith/fox.git

vcpkg_from_github(
	OUT_SOURCE_PATH SOURCE_PATH
	REPO devinsmith/fox
	REF b503d912aeb181e2d456eb09cc47e7af9665f722
	SHA512 80c182aac73c623989125b702c8299b627e47442ddd65eac887a70f7089780ea5eda68f1847b012788c6a3ef6480da9b5a45ab8d89c9fc8289b77ef1eec09531
	HEAD_REF release/1.6
    PATCHES
        lib_cmake.patch
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
-DWITH_XIM=OFF
-DWITH_JPEG=OFF
-DWITH_PNG=OFF
-DWITH_TIFF=OFF
-DWITH_ZLIB=ON
-DWITH_BZ2LIB=OFF
-DWITH_WEBP=OFF
-DWITH_OPENJPEG=OFF
-DWITH_OPENGL=ON
)

vcpkg_cmake_install()
vcpkg_copy_pdbs()
vcpkg_fixup_pkgconfig()
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug/include")
configure_file("${SOURCE_PATH}/LICENSE" "${CURRENT_PACKAGES_DIR}/share/fox-toolkit/copyright" COPYONLY)
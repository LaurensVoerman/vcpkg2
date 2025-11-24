#vcpkg_check_linkage(ONLY_DYNAMIC_LIBRARY)

if(VCPKG_TARGET_IS_WINDOWS)
    vcpkg_download_distfile(ARCHIVE
        URLS "https://damassets.autodesk.net/content/dam/autodesk/www/files/fbx202037_fbxsdk_vs2022_win.exe"
        FILENAME "fbx202037_fbxsdk_vs2022_win.exe"
        SHA512 2f3daf41a7c3a247046291aaf8a21663aa89d64b10fe386a6fa082d799abe840e9d1fbee4ea3f902c4e59010e21e6149132c9b0a8f2c5eca0a1a723e9a25fd16
    )
elseif(VCPKG_TARGET_IS_LINUX)
    vcpkg_download_distfile(ARCHIVE
        URLS "https://www.libxl.com/download/libxl-lin-4.0.3.1.tar.gz"
        FILENAME "libxl-lin-4.0.3.1.tar.gz"
        SHA512 538a2c79a2609f600ef37fd2f5e9664af059a935705f7f3ef1d690a9da79135ca1aee5395f4156ac766cae819487ae99013fb0dacebfa8a109a0c3a85c9334de
    )
endif()

#vcpkg_extract_source_archive_ex(
#    OUT_SOURCE_PATH SOURCE_PATH
#    ARCHIVE "${ARCHIVE}"
#)
set(SOURCE_PATH "${CURRENT_BUILDTREES_DIR}/src")
if(NOT IS_DIRECTORY "${SOURCE_PATH}")
  vcpkg_extract_archive(
  	ARCHIVE "${ARCHIVE}"
  	DESTINATION "${SOURCE_PATH}"
  )
endif()
set(VCPKG_POLICY_EMPTY_INCLUDE_FOLDER enabled)

#file(GLOB_RECURSE headers LIST_DIRECTORIES true "${SOURCE_PATH}/include/*")
#file(COPY ${headers} DESTINATION "${CURRENT_PACKAGES_DIR}/FBX/${VERSION}/include")

#error: install command is not scriptable
#install(DIRECTORY "${SOURCE_PATH}/include" DESTINATION "${CURRENT_PACKAGES_DIR}/FBX/${VERSION}/include")

file(COPY "${SOURCE_PATH}/include" DESTINATION "${CURRENT_PACKAGES_DIR}/FBX/${VERSION}" FILES_MATCHING PATTERN "*.h")

if(VCPKG_TARGET_IS_WINDOWS)
    file(COPY "${SOURCE_PATH}/lib/${VCPKG_TARGET_ARCHITECTURE}/debug/alembic-md.lib" DESTINATION "${CURRENT_PACKAGES_DIR}/FBX/${VERSION}/vs2022/${VCPKG_TARGET_ARCHITECTURE}/debug/")
    file(COPY "${SOURCE_PATH}/lib/${VCPKG_TARGET_ARCHITECTURE}/debug/libfbxsdk-md.lib" DESTINATION "${CURRENT_PACKAGES_DIR}/FBX/${VERSION}/vs2022/${VCPKG_TARGET_ARCHITECTURE}/debug/")
    file(COPY "${SOURCE_PATH}/lib/${VCPKG_TARGET_ARCHITECTURE}/debug/libxml2-md.lib" DESTINATION "${CURRENT_PACKAGES_DIR}/FBX/${VERSION}/vs2022/${VCPKG_TARGET_ARCHITECTURE}/debug/")
    file(COPY "${SOURCE_PATH}/lib/${VCPKG_TARGET_ARCHITECTURE}/debug/zlib-md.lib" DESTINATION "${CURRENT_PACKAGES_DIR}/FBX/${VERSION}/vs2022/${VCPKG_TARGET_ARCHITECTURE}/debug/")
    file(COPY "${SOURCE_PATH}/lib/${VCPKG_TARGET_ARCHITECTURE}/release/alembic-md.lib" DESTINATION "${CURRENT_PACKAGES_DIR}/FBX/${VERSION}/vs2022/${VCPKG_TARGET_ARCHITECTURE}/release/")
    file(COPY "${SOURCE_PATH}/lib/${VCPKG_TARGET_ARCHITECTURE}/release/libfbxsdk-md.lib" DESTINATION "${CURRENT_PACKAGES_DIR}/FBX/${VERSION}/vs2022/${VCPKG_TARGET_ARCHITECTURE}/release/")
    file(COPY "${SOURCE_PATH}/lib/${VCPKG_TARGET_ARCHITECTURE}/release/libxml2-md.lib" DESTINATION "${CURRENT_PACKAGES_DIR}/FBX/${VERSION}/vs2022/${VCPKG_TARGET_ARCHITECTURE}/release/")
    file(COPY "${SOURCE_PATH}/lib/${VCPKG_TARGET_ARCHITECTURE}/release/zlib-md.lib" DESTINATION "${CURRENT_PACKAGES_DIR}/FBX/${VERSION}/vs2022/${VCPKG_TARGET_ARCHITECTURE}/release/")

  if(VCPKG_TARGET_ARCHITECTURE STREQUAL "x64")
    message("fbx x64 OK")
  elseif(VCPKG_TARGET_ARCHITECTURE STREQUAL "arm64")
    message("fbx arm64 untested")
  elseif(VCPKG_TARGET_ARCHITECTURE STREQUAL "x86")
    message( FATAL_ERROR "fbx x64 NOT SUPPORTED")
  else()
    message( FATAL_ERROR "fbx ${VCPKG_TARGET_ARCHITECTURE} NOT SUPPORTED")
  endif()
elseif(VCPKG_TARGET_IS_LINUX)
  if(VCPKG_TARGET_ARCHITECTURE STREQUAL "x64")
    file(COPY "${SOURCE_PATH}/lib64/libxl.so" DESTINATION "${CURRENT_PACKAGES_DIR}/lib")
    file(COPY "${SOURCE_PATH}/lib64/libxl.so" DESTINATION "${CURRENT_PACKAGES_DIR}/debug/lib")
  elseif(VCPKG_TARGET_ARCHITECTURE STREQUAL "x86")
    file(COPY "${SOURCE_PATH}/lib/libxl.so" DESTINATION "${CURRENT_PACKAGES_DIR}/lib")
    file(COPY "${SOURCE_PATH}/lib/libxl.so" DESTINATION "${CURRENT_PACKAGES_DIR}/debug/lib")
  endif()
endif()

# Handle copyright
file(INSTALL "${SOURCE_PATH}/license.rtf" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}" RENAME copyright)

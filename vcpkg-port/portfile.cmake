vcpkg_from_github(
    OUT_SOURCE_PATH SOURCE_PATH
    REPO cleishm/electro-cpp
    REF "v${VERSION}"
    SHA512 d5c11e80af17d126e55a85785c2cb461c34404af7efafda8450b0093e280cd88e4c88733f395ca94bb997ac76edd7a8fdd6a4261e56f061cac6659717e41c3b3
    HEAD_REF main
)

vcpkg_cmake_configure(
    SOURCE_PATH "${SOURCE_PATH}"
    OPTIONS
        -DELECTRO_BUILD_TESTS=OFF
)

vcpkg_cmake_install()
vcpkg_cmake_config_fixup(PACKAGE_NAME electro CONFIG_PATH lib/cmake/electro)

file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/debug")
file(REMOVE_RECURSE "${CURRENT_PACKAGES_DIR}/lib")

file(INSTALL "${CMAKE_CURRENT_LIST_DIR}/usage" DESTINATION "${CURRENT_PACKAGES_DIR}/share/${PORT}")

vcpkg_install_copyright(FILE_LIST "${SOURCE_PATH}/LICENSE")

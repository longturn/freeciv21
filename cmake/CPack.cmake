# This file reconfigures variables at CPack time.
#
# It is called by
# https://cmake.org/cmake/help/latest/module/CPack.html#variable:CPACK_PROJECT_CONFIG_FILE
#

#used to get cmake-time variables at cpack-time
set(CMAKE_SOURCE_DIR @CMAKE_SOURCE_DIR@)

if(CPACK_GENERATOR STREQUAL "NSIS")
    # NSIS doesn't always like forward slashes
    file(TO_NATIVE_PATH "${CPACK_NSIS_MUI_HEADERIMAGE}" CPACK_NSIS_MUI_HEADERIMAGE_NATIVE)
    set(CPACK_NSIS_MUI_HEADERIMAGE "${CPACK_NSIS_MUI_HEADERIMAGE_NATIVE}")
    file(TO_NATIVE_PATH "${CPACK_NSIS_MUI_WELCOMEFINISHPAGE_BITMAP}" CPACK_NSIS_MUI_WELCOMEFINISHPAGE_BITMAP_NATIVE)
    set(CPACK_NSIS_MUI_WELCOMEFINISHPAGE_BITMAP "${CPACK_NSIS_MUI_WELCOMEFINISHPAGE_BITMAP_NATIVE}")
endif()

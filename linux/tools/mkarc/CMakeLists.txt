cmake_minimum_required(VERSION 3.20)
project(mkarc LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(UNARC ${CMAKE_CURRENT_SOURCE_DIR}/../../app/include/unarc)

# apply 64-bit patches
set(UNARC_PATCH ${CMAKE_CURRENT_SOURCE_DIR}/../../patches/unarc-64bit.patch)
if(EXISTS ${UNARC_PATCH})
    execute_process(
        COMMAND git apply --check ${UNARC_PATCH}
        WORKING_DIRECTORY ${UNARC}
        RESULT_VARIABLE PATCH_CHECK ERROR_QUIET
    )
    if(PATCH_CHECK EQUAL 0)
        execute_process(
            COMMAND git apply ${UNARC_PATCH}
            WORKING_DIRECTORY ${UNARC}
        )
        message(STATUS "Applied unarc-64bit.patch")
    endif()
endif()

# freearc compression library (full, compress+decompress)
set(FREEARC_SRCS
    ${UNARC}/Environment.cpp
    ${UNARC}/Compression/Common.cpp
    ${UNARC}/Compression/CompressionLibrary.cpp
    ${UNARC}/Compression/LZMA2/C_LZMA.cpp
    ${UNARC}/Compression/LZMA2/C_BCJ.cpp
    ${UNARC}/Compression/Dict/C_Dict.cpp
    ${UNARC}/Compression/Delta/C_Delta.cpp
    ${UNARC}/Compression/REP/C_REP.cpp
    ${UNARC}/Compression/LZP/C_LZP.cpp
    # TODO: fix PPMD 64-bit static init corruption
    # ${UNARC}/Compression/PPMD/C_PPMD.cpp
    ${UNARC}/Compression/External/C_External.cpp
    ${UNARC}/Compression/MM/C_MM.cpp
    ${UNARC}/Compression/MM/C_TTA.cpp
    ${UNARC}/Compression/Tornado/C_Tornado.cpp
    ${UNARC}/Compression/GRZip/C_GRZip.cpp
)

add_library(freearc STATIC ${FREEARC_SRCS})
target_include_directories(freearc PRIVATE ${UNARC} ${UNARC}/Compression)
target_compile_definitions(freearc PRIVATE
    FREEARC_UNIX FREEARC_INTEL_BYTE_ORDER FREEARC_64BIT _NO_EXCEPTIONS
)
target_compile_options(freearc PRIVATE
    -fno-exceptions -fno-rtti -O2 -fpermissive
    -Wno-write-strings -Wno-sign-compare -Wno-conversion
    -Wno-unknown-pragmas -Wno-unused-variable -Wno-unused-but-set-variable
    -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast -Wno-narrowing
)

add_executable(mkarc mkarc.cpp)
target_include_directories(mkarc PRIVATE ${UNARC} ${UNARC}/Compression)
target_compile_definitions(mkarc PRIVATE
    FREEARC_UNIX FREEARC_INTEL_BYTE_ORDER FREEARC_64BIT
)
target_compile_options(mkarc PRIVATE -fpermissive -fno-rtti -fno-exceptions -Wno-write-strings)
target_link_libraries(mkarc PRIVATE
    -Wl,--whole-archive freearc -Wl,--no-whole-archive
    pthread rt
)

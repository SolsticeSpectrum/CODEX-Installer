cmake_minimum_required(VERSION 3.20)
project(codex-installer LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2       REQUIRED sdl2)
pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
pkg_check_modules(SDL2_TTF   REQUIRED SDL2_ttf)
pkg_check_modules(SDL2_MIXER REQUIRED SDL2_mixer)

# unarc (FreeArc .bin decompressor) — apply 64-bit patches
set(UNARC ${CMAKE_CURRENT_SOURCE_DIR}/include/unarc)
set(UNARC_PATCH ${CMAKE_CURRENT_SOURCE_DIR}/../patches/unarc-64bit.patch)
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
add_library(unarc STATIC
    ${UNARC}/Environment.cpp
    ${UNARC}/Compression/Common.cpp
    ${UNARC}/Compression/CompressionLibrary.cpp
    ${UNARC}/Compression/LZMA2/C_LZMA.cpp
    ${UNARC}/Compression/LZMA2/C_BCJ.cpp
    ${UNARC}/Compression/Dict/C_Dict.cpp
    ${UNARC}/Compression/Delta/C_Delta.cpp
    ${UNARC}/Compression/REP/C_REP.cpp
    ${UNARC}/Compression/LZP/C_LZP.cpp
    ${UNARC}/Compression/PPMD/C_PPMD.cpp
    ${UNARC}/Compression/External/C_External.cpp
    ${UNARC}/Compression/MM/C_MM.cpp
    ${UNARC}/Compression/MM/C_TTA.cpp
    ${UNARC}/Compression/Tornado/C_Tornado.cpp
    ${UNARC}/Compression/GRZip/C_GRZip.cpp
    ${UNARC}/Compression/4x4/C_4x4.cpp
)
target_include_directories(unarc PRIVATE ${UNARC} ${UNARC}/Compression)
target_compile_definitions(unarc PRIVATE
    FREEARC_UNIX FREEARC_INTEL_BYTE_ORDER FREEARC_64BIT
    FREEARC_DECOMPRESS_ONLY _NO_EXCEPTIONS UNARC
    _FILE_OFFSET_BITS=64 _LARGEFILE_SOURCE
)
target_compile_options(unarc PRIVATE
    -fno-exceptions -fno-rtti -O2
    -Wno-write-strings -Wno-sign-compare -Wno-conversion
    -Wno-unknown-pragmas -Wno-unused-variable -Wno-unused-but-set-variable
)

# unrar (RAR decompressor) — files from unrar/makefile OBJECTS
set(UNRAR ${CMAKE_CURRENT_SOURCE_DIR}/include/unrar)
set(UNRAR_SRCS
    ${UNRAR}/rar.cpp       ${UNRAR}/strlist.cpp   ${UNRAR}/strfn.cpp
    ${UNRAR}/pathfn.cpp    ${UNRAR}/smallfn.cpp   ${UNRAR}/global.cpp
    ${UNRAR}/file.cpp      ${UNRAR}/filefn.cpp    ${UNRAR}/filcreat.cpp
    ${UNRAR}/archive.cpp   ${UNRAR}/arcread.cpp   ${UNRAR}/unicode.cpp
    ${UNRAR}/system.cpp    ${UNRAR}/crypt.cpp     ${UNRAR}/crc.cpp
    ${UNRAR}/rawread.cpp   ${UNRAR}/encname.cpp   ${UNRAR}/resource.cpp
    ${UNRAR}/match.cpp     ${UNRAR}/timefn.cpp    ${UNRAR}/rdwrfn.cpp
    ${UNRAR}/consio.cpp    ${UNRAR}/options.cpp   ${UNRAR}/errhnd.cpp
    ${UNRAR}/rarvm.cpp     ${UNRAR}/secpassword.cpp
    ${UNRAR}/rijndael.cpp  ${UNRAR}/getbits.cpp   ${UNRAR}/sha1.cpp
    ${UNRAR}/sha256.cpp    ${UNRAR}/blake2s.cpp   ${UNRAR}/hash.cpp
    ${UNRAR}/extinfo.cpp   ${UNRAR}/extract.cpp   ${UNRAR}/volume.cpp
    ${UNRAR}/list.cpp      ${UNRAR}/find.cpp      ${UNRAR}/unpack.cpp
    ${UNRAR}/headers.cpp   ${UNRAR}/threadpool.cpp ${UNRAR}/rs16.cpp
    ${UNRAR}/cmddata.cpp   ${UNRAR}/ui.cpp        ${UNRAR}/largepage.cpp
    ${UNRAR}/dll.cpp       ${UNRAR}/filestr.cpp   ${UNRAR}/scantree.cpp
    ${UNRAR}/qopen.cpp
)
add_library(unrar STATIC ${UNRAR_SRCS})
target_include_directories(unrar PRIVATE ${UNRAR})
target_compile_definitions(unrar PRIVATE
    _FILE_OFFSET_BITS=64 _LARGEFILE_SOURCE RAR_SMP
    RARDLL SILENT
)
target_compile_options(unrar PRIVATE
    -std=c++11 -O1
    -include ${UNRAR}/rar.hpp
    -Wno-switch -Wno-dangling-else -Wno-parentheses
    -Wno-unused-variable -Wno-unused-result -Wno-unused-function
)
set_source_files_properties(${UNRAR}/global.cpp PROPERTIES
    COMPILE_FLAGS "-DINCLUDEGLOBAL"
)

# 7zip (ANSI-C 7z decoder from official source)
set(SZ ${CMAKE_CURRENT_SOURCE_DIR}/include/7zip/C)
add_library(7zdec STATIC
    ${SZ}/7zAlloc.c   ${SZ}/7zArcIn.c  ${SZ}/7zBuf.c   ${SZ}/7zBuf2.c
    ${SZ}/7zCrc.c     ${SZ}/7zCrcOpt.c ${SZ}/7zDec.c   ${SZ}/7zFile.c
    ${SZ}/7zStream.c  ${SZ}/Bcj2.c     ${SZ}/Bra.c     ${SZ}/Bra86.c
    ${SZ}/BraIA64.c   ${SZ}/CpuArch.c  ${SZ}/Delta.c   ${SZ}/Lzma2Dec.c
    ${SZ}/LzmaDec.c   ${SZ}/Ppmd7.c    ${SZ}/Ppmd7Dec.c ${SZ}/Sha256.c
    ${SZ}/Alloc.c
)
target_include_directories(7zdec PRIVATE ${SZ})
target_compile_definitions(7zdec PRIVATE _7ZIP_ST)
target_compile_options(7zdec PRIVATE -O2 -Wno-unused-parameter)

# xdelta3 — external tool for FreeArc pipeline
set(XD ${CMAKE_CURRENT_SOURCE_DIR}/include/xdelta/xdelta3)
add_executable(xdelta3 ${XD}/xdelta3.c)
target_include_directories(xdelta3 PRIVATE ${XD})
target_compile_definitions(xdelta3 PRIVATE
    XD3_MAIN=1 XD3_POSIX=1 SECONDARY_DJW=0 SECONDARY_FGK=0
    EXTERNAL_COMPRESSION=0 XD3_USE_LARGEFILE64=1
    SIZEOF_SIZE_T=8 SIZEOF_UNSIGNED_LONG_LONG=8 SIZEOF_UNSIGNED_LONG=8
)
target_compile_options(xdelta3 PRIVATE -O2 -Wno-unused-parameter -Wno-unused-function)

# srep — external tool for FreeArc pipeline
set(SREP ${CMAKE_CURRENT_SOURCE_DIR}/include/srep)
add_executable(srep
    ${SREP}/Compression/SREP/srep.cpp
    ${SREP}/Compression/Common.cpp
)
target_include_directories(srep PRIVATE
    ${SREP}/Compression ${SREP}/Compression/SREP
    ${SREP}/Compression/_Encryption ${SREP}/Compression/_Encryption/headers
    ${SREP}/Compression/_Encryption/hashes
)
target_compile_definitions(srep PRIVATE
    FREEARC_UNIX FREEARC_INTEL_BYTE_ORDER FREEARC_64BIT _FILE_OFFSET_BITS=64
)
target_compile_options(srep PRIVATE
    -O2 -Wno-write-strings -Wno-sign-compare -Wno-unused-result
    -Wno-unused-variable -Wno-unknown-pragmas -Wno-deprecated
)
target_link_libraries(srep PRIVATE pthread)

# precomp — external tool for FreeArc pipeline
set(CMAKE_POLICY_VERSION_MINIMUM 3.5 CACHE STRING "" FORCE)
add_subdirectory(include/precomp EXCLUDE_FROM_ALL)
set_target_properties(precomp PROPERTIES EXCLUDE_FROM_ALL FALSE)
target_compile_options(precomp PRIVATE -Wno-implicit-function-declaration -include assert.h)

# installer
add_executable(installer
    src/main.cpp
    src/TSeStyleSource.cpp
    src/TSeBitmapObject.cpp
    src/DrawStyleEdge.cpp
    src/TWizardForm.cpp
    src/ISExtractor.cpp
    src/TSelectFolderForm.cpp
    include/miniz.c
)

# ISExtractor uses unarc headers with UB casts that break under -O2
set_source_files_properties(src/ISExtractor.cpp PROPERTIES COMPILE_FLAGS "-O0")

target_include_directories(installer PRIVATE
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_IMAGE_INCLUDE_DIRS}
    ${SDL2_TTF_INCLUDE_DIRS}
    ${SDL2_MIXER_INCLUDE_DIRS}
    src
    include
    ${UNARC}
    ${UNARC}/Compression
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)
target_compile_definitions(installer PRIVATE
    FREEARC_UNIX FREEARC_INTEL_BYTE_ORDER FREEARC_64BIT
    FREEARC_DECOMPRESS_ONLY _NO_EXCEPTIONS UNARC
    _FILE_OFFSET_BITS=64 _LARGEFILE_SOURCE
    _7ZIP_ST
)

target_link_libraries(installer PRIVATE
    ${SDL2_LIBRARIES}
    ${SDL2_IMAGE_LIBRARIES}
    ${SDL2_TTF_LIBRARIES}
    ${SDL2_MIXER_LIBRARIES}
    7zdec
    -Wl,--whole-archive unarc -Wl,--no-whole-archive -Wl,--allow-multiple-definition
    unrar
    pthread rt
)

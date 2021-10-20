project(basisu LANGUAGES C CXX)
cmake_minimum_required(VERSION 3.10)

find_package(Threads REQUIRED)

set(BASIS_VERSION "v1_15_update2")
set(BASIS_URL "https://github.com/BinomialLLC/basis_universal")
if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.11)
    include(FetchContent)
    FetchContent_Declare(
        basis
        GIT_REPOSITORY "${BASIS_URL}"
        GIT_TAG "${BASIS_VERSION}"
    )
    FetchContent_GetProperties(basis)
else()
    set(BASIS_BASE_PATH "${EXTERNALS_DIR}")
    set(basis_POPULATED FALSE)
    if(NOT EXISTS "${BASIS_BASE_PATH}/basis/CMakeLists.txt")
        set(BASIS_ZIP "${DOWNLOADS_DIR}/basis.zip")
        file(DOWNLOAD "${BASIS_URL}/archive/refs/tags/${BASIS_VERSION}.zip" "${BASIS_ZIP}" TIMEOUT 60 TLS_VERIFY ON)

        file(MAKE_DIRECTORY "${BASIS_BASE_PATH}/basis")
        execute_process(COMMAND ${CMAKE_COMMAND} -E tar -xf "${BASIS_ZIP}" WORKING_DIRECTORY "${BASIS_BASE_PATH}")
    endif()

    set(basis_SOURCE_DIR "${BASIS_BASE_PATH}/basis")
    set(basis_BINARY_DIR "${PROJECT_BINARY_DIR}/externals/basis")
endif()

if(NOT basis_POPULATED)
    if(COMMAND FetchContent_Populate)
        FetchContent_Populate(basis)
    endif()
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(BUILD_X64 FALSE CACHE BOOL "")
    endif()
    set(STATIC TRUE CACHE BOOL "")

    # As of 1.15, basis is not setup to be used as a library.
    # So roll our own based on its CMakeLists.txt.
    # We want two things: 
    #   * an encoder library (texture reprocessing), to be used by consumer projects.
    #   * the transcoder (runtime usage).
    add_library(basisu-encoder STATIC
        "${basis_SOURCE_DIR}/encoder/basisu_backend.cpp"
	    "${basis_SOURCE_DIR}/encoder/basisu_basis_file.cpp"
	    "${basis_SOURCE_DIR}/encoder/basisu_comp.cpp"
	    "${basis_SOURCE_DIR}/encoder/basisu_enc.cpp"
	    "${basis_SOURCE_DIR}/encoder/basisu_etc.cpp"
	    "${basis_SOURCE_DIR}/encoder/basisu_frontend.cpp"
	    "${basis_SOURCE_DIR}/encoder/basisu_global_selector_palette_helpers.cpp"
	    "${basis_SOURCE_DIR}/encoder/basisu_gpu_texture.cpp"
        "${basis_SOURCE_DIR}/encoder/basisu_pvrtc1_4.cpp"
	    "${basis_SOURCE_DIR}/encoder/basisu_resampler.cpp"
	    "${basis_SOURCE_DIR}/encoder/basisu_resample_filters.cpp"
	    "${basis_SOURCE_DIR}/encoder/basisu_ssim.cpp"
	    "${basis_SOURCE_DIR}/encoder/basisu_astc_decomp.cpp"
	    "${basis_SOURCE_DIR}/encoder/basisu_uastc_enc.cpp"
        "${basis_SOURCE_DIR}/encoder/basisu_bc7enc.cpp"
	    "${basis_SOURCE_DIR}/encoder/lodepng.cpp"
	    "${basis_SOURCE_DIR}/encoder/apg_bmp.c"
	    "${basis_SOURCE_DIR}/encoder/jpgd.cpp"
	    "${basis_SOURCE_DIR}/encoder/basisu_kernels_sse.cpp"
	    "${basis_SOURCE_DIR}/transcoder/basisu_transcoder.cpp"
        "${basis_SOURCE_DIR}/zstd/zstd.c"
     )
    target_include_directories(basisu-encoder INTERFACE "${basis_SOURCE_DIR}")
    target_link_libraries(basisu-encoder PUBLIC Threads::Threads)
    target_compile_definitions(basisu-encoder
        PRIVATE
            BASISD_SUPPORT_KTX2_ZSTD=1
            BASISU_SUPPORT_SSE=0
            "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:_LARGEFILE64_SOURCE=1;_FILE_OFFSET_BITS=64>"
    )
    target_compile_options(basisu-encoder
        PRIVATE
            "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-fvisibility=hidden;-fPIC;-fno-strict-aliasing;-Wall;-Wextra;-Wno-unused-local-typedefs;-Wno-unused-value;-Wno-unused-parameter;-Wno-unused-variable>"
    )
    target_compile_features(basisu-encoder PUBLIC cxx_std_11)

    # Runtime transcoder.
    add_library(basisu-transcoder STATIC "${basis_SOURCE_DIR}/transcoder/basisu_transcoder.cpp" "${basis_SOURCE_DIR}/zstd/zstddeclib.c")
    # Trim transcoder.
    target_compile_definitions(basisu-transcoder
    PRIVATE
        BASISU_NO_ITERATOR_DEBUG_LEVEL
        "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:_LARGEFILE64_SOURCE=1;_FILE_OFFSET_BITS=64>"
        BASISU_SUPPORT_SSE=0
        BASISD_SUPPORT_KTX2_ZSTD=1
        BASISD_SUPPORT_UASTC=1
        # Desktop texture formats (s3tc, rgtc)
        BASISD_SUPPORT_DXT1=1 # (BC1)
        BASISD_SUPPORT_DXT5A=1 # (BC3/4/5)

        # bptc textures
        BASISD_SUPPORT_BC7=0
        BASISD_SUPPORT_BC7_MODE5=0

        # Common embedded devices formats.
        BASISD_SUPPORT_ETC2_EAC_RG11=1
        BASISD_SUPPORT_ETC2_EAC_A8=1    # 87% of android devices
        BASISD_SUPPORT_ASTC=1           # 77%
        # Seldom android support
        BASISD_SUPPORT_ATC=1            # 35% (Adreno, ASTC beats it)
                                        # Unused, only ON because needed by PVRTC2.
        BASISD_SUPPORT_PVRTC1=1         # 11% (PowerVR)
        BASISD_SUPPORT_PVRTC2=1
        BASISD_SUPPORT_ASTC_HIGHER_OPAQUE_QUALITY=1
        BASISD_SUPPORT_FXT1=0
    )

    # From its CMakeLists
    target_compile_options(basisu-transcoder
        PRIVATE
            "$<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-fvisibility=hidden;-fPIC;-fno-strict-aliasing;-Wall;-Wextra;-Wno-unused-local-typedefs;-Wno-unused-value;-Wno-unused-parameter;-Wno-unused-variable>"
    )
    target_compile_features(basisu-transcoder PUBLIC cxx_std_11)
    target_include_directories(basisu-transcoder INTERFACE "${basis_SOURCE_DIR}")
endif()
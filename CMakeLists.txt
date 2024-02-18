cmake_minimum_required(VERSION 3.20)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake)

project(depinc-miner)

find_package(OpenSSL REQUIRED)
find_package(unofficial-utf8proc CONFIG REQUIRED)
find_package(asio CONFIG REQUIRED)
find_package(CURL REQUIRED)
find_package(plog CONFIG REQUIRED)
find_package(cxxopts CONFIG REQUIRED)

find_package(PkgConfig REQUIRED)
pkg_check_modules(gmp REQUIRED IMPORTED_TARGET gmp)

file(GLOB MINER_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
file(GLOB UINT256_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/uint256/*.cpp)

file(GLOB UNIVALUE_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/univalue/lib/*.cpp)
add_library(univalue STATIC ${UNIVALUE_SRCS})
target_include_directories(univalue PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/univalue/include)

include(FetchContent)

FetchContent_Declare(tinyformat
    GIT_REPOSITORY https://github.com/c42f/tinyformat
    GIT_TAG v2.3.0
    PATCH_COMMAND git apply --reject ${CMAKE_CURRENT_SOURCE_DIR}/tinyformat-fix.patch || true
)

set(BUILD_BLS_PYTHON_BINDINGS OFF CACHE BOOL "no python binding")
set(BUILD_BLS_TESTS OFF CACHE BOOL "no tests")
set(BUILD_BLS_BENCHMARKS OFF CACHE BOOL "no benchmarks")

FetchContent_Declare(bls
    GIT_REPOSITORY https://github.com/Chia-Network/bls-signatures
    GIT_TAG 2.0.2
)

FetchContent_Declare(vdf
    GIT_REPOSITORY https://github.com/depinc/vdf
    GIT_TAG v1.0.3
)

FetchContent_Declare(chiapos
    GIT_REPOSITORY https://github.com/Chia-Network/chiapos
    GIT_TAG 2.0.4
    PATCH_COMMAND git apply --reject ${CMAKE_CURRENT_SOURCE_DIR}/chiapos-fix.patch || true
)
FetchContent_MakeAvailable(tinyformat bls blst vdf chiapos blake3)

add_executable(depinc-miner ${MINER_SRCS} ${UINT256_SRCS})
target_compile_features(depinc-miner PRIVATE cxx_std_17)
target_link_libraries(depinc-miner PRIVATE
    OpenSSL::SSL
    OpenSSL::Crypto
    utf8proc
    asio::asio
    CURL::libcurl
    plog::plog
    cxxopts::cxxopts
    univalue
    time_vdf
    chiapos
    bls
    PkgConfig::gmp
)

if (WIN32)
    target_link_libraries(depinc-miner PRIVATE ws2_32 -static)
endif()

target_include_directories(depinc-miner PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/uint256
    ${CMAKE_BINARY_DIR}/libs/green_reaper/include
    ${tinyformat_SOURCE_DIR}
    ${bls_SOURCE_DIR}/src
    ${blst_SOURCE_DIR}
    ${vdf_SOURCE_DIR}/src
    ${chiapos_SOURCE_DIR}
    ${blake3_SOURCE_DIR}/c
)

cmake_minimum_required(VERSION 3.20)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/vcpkg/scripts/buildsystems/vcpkg.cmake)

project(depinc-miner)

find_package(OpenSSL REQUIRED)
find_package(unofficial-utf8proc CONFIG REQUIRED)
find_package(asio CONFIG REQUIRED)
find_package(CURL REQUIRED)
find_package(plog CONFIG REQUIRED)
find_package(cxxopts CONFIG REQUIRED)

file(GLOB MINER_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
file(GLOB UINT256_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/uint256/*.cpp)

file(GLOB UNIVALUE_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/src/univalue/lib/*.cpp)
add_library(univalue STATIC ${UNIVALUE_SRCS})
target_include_directories(univalue PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src/univalue/include)

include(FetchContent)

FetchContent_Declare(tinyformat
    GIT_REPOSITORY https://github.com/c42f/tinyformat
    GIT_TAG v2.3.0
)

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
set(CP_BUILD_BLADEBIT_HARVESTER ON)
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
)

target_include_directories(depinc-miner PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/src/uint256
    ${tinyformat_SOURCE_DIR}
    ${bls_SOURCE_DIR}/src
    ${blst_SOURCE_DIR}
    ${vdf_SOURCE_DIR}/src
    ${chiapos_SOURCE_DIR}
    ${blake3_SOURCE_DIR}/c
)

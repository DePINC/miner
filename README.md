# Standalone miner for DePINC

## Clone the repo with submodules

Vcpkg is added as a submodule for convenient. You need to initialize the repo before preparing the build.

## Cross-compiling for Windows platform

To compile binary file for Windows platform, run the following command:

```bash
cmake . -B build -DVCPKG_TARGET_TRIPLET=x64-mingw-static -DVCPKG_CHAINLOAD_TOOLCHAIN_FILE=`pwd`/vcpkg/scripts/toolchains/mingw.cmake -DVCPKG_HOST_TRIPLET=x64-linux -DVCPKG_TARGET_ARCHITECTURE=x64
```


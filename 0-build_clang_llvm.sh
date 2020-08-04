#!/bin/bash -e

BUILD_DIR=${PWD}/build
#rm -rf $BUILD_DIR
build_clang_llvm() {
	if [ ! -d "$BUILD_DIR" ]; then
		mkdir build
	fi

	pushd build
	#cmake -DLLVM_ENABLE_PROJECTS="clang;compiler-rt" -DCMAKE_BUILD_TYPE=Release -DLLVM_ENABLE_DUMP=ON -G "Unix Makefiles" ../llvm-project/llvm
	make -j$(nproc)
	popd
    export PATH=${BUILD_DIR}/bin:${PATH}
}

build_clang_llvm

#!/bin/sh

ENABLE_INTERNAL_LOG=true
PROJECT_TOP=`pwd`
PROJECT_OUT_DIR="${PROJECT_TOP}/out"
EXECUTABLE_OUT="${PROJECT_OUT_DIR}/bin"
SHARED_LIB_OUT="${PROJECT_OUT_DIR}/lib"
STATIC_LIB_OUT="${PROJECT_OUT_DIR}/lib"
TEST_CASE_DIR="${PROJECT_TOP}/test"
LIBCO_SRC_DIR="${PROJECT_TOP}/libcoroutine"

PROJECT_NAME="demo"
PROJECT_VERSION="0.0.1"

COMPILE_CONFIGS="-DEXECUTABLE_OUT=${EXECUTABLE_OUT}     \
    -DENABLE_INTERNAL_LOG=${ENABLE_INTERNAL_LOG}        \
    -DSHARED_LIB_OUT=${SHARED_LIB_OUT}                  \
    -DSTATIC_LIB_OUT=${STATIC_LIB_OUT}                  \
    -DPROJECT_NAME=${PROJECT_NAME}                      \
    -DPROJECT_VERSION=${PROJECT_VERSION}                \
    -DPROJECT_TOP=${PROJECT_TOP}                        \
    -DTEST_CASE_DIR=${TEST_CASE_DIR}                    \
    -DLIBCO_SRC_DIR=${LIBCO_SRC_DIR}"


do_compile_jsoncpp(){
    unzip ${PROJECT_TOP}/download/jsoncpp.zip -d ${PROJECT_TOP}/third_party
    cd ${PROJECT_TOP}/third_party/jsoncpp-master/
    mkdir build
    cd build
    cmake .. -DJSONCPP_WITH_TESTS=OFF
    make
    cd ${PROJECT_TOP}
}

do_compile(){
# compilation for json
    #do_compile_jsoncpp
    echo "compiling ${PROJECT_NAME}"
    cd ${PROJECT_TOP}/build
    echo "generat makefile..."
    cmake .. ${COMPILE_CONFIGS}
    make -j8
}

do_prepare(){
    mkdir -p ${PROJECT_TOP}/third_party
    mkdir -p ${PROJECT_TOP}/out/lib
    mkdir -p ${PROJECT_TOP}/out/bin
    mkdir -p ${PROJECT_TOP}/build;
}

do_clean(){
    cd ${PROJECT_TOP}/build
    make clean
    cd ${PROJECT_TOP}
    rm -rf ${PROJECT_TOP}/build
    rm -rf ${PROJECT_TOP}/lib
    rm -rf ${PROJECT_TOP}/bin
    rm -rf ${PROJECT_TOP}/third_party
}

main(){
    do_clean
    do_prepare
    do_compile
}

main
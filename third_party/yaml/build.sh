dir_name=yaml-cpp-master/

clean(){
    rm -rf include/
    rm -rf libyaml-cpp.a
    rm -rf yaml-cpp-master/
}

build(){
    unzip yaml-cpp.zip
    cd ${dir_name}
    cmake .
    make -j8
    cd -
}

install(){
    install_path=$1
    rm -rf include/
    rm -rf ${install_path}/libyaml-cpp.a
    mkdir -p include/
    cp -rf ${dir_name}/include/* include/
    cp -rf ${dir_name}/libyaml-cpp.a ${install_path}/libyaml-cpp.a
}

if [ "$1" == "build" ]; then
    build
elif [ "$1" == "clean" ]; then
    clean
elif [ "$1" == "install" ]; then
    install $2
fi
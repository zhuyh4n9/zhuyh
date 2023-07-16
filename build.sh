clean(){
    python3 ./script/build.py clean config/build_config.json
}

build(){
    python3 ./script/build.py build config/build_config.json
}

force_build(){
    clean
    build
}

usage(){
    if [ $# -eq 1 ]; then
        echo "error :" $1
    fi
    echo "usage:"
    echo "sh build.sh clean|build|force_build"
}

if [ $# -lt 1 ]; then
    usage "invalid argument"
    exit 1
fi

if [ "$1" == "clean" ]; then
    clean
elif [ "$1" == "build" ]; then
    build
elif [ "$1" == "force_build" ]; then
    force_build
else
    usage "unsupport command: $1"
    exit 1
fi
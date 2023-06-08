set -e

ts-node ../../../run.ts uapi.json

cmake . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

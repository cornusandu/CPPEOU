mkdir tmp -Force > $null
mkdir ./dist/windows_x86_64 -Force > $null

g++ ./src/autoloaddll.cpp -c -o ./tmp/autoloaddll.o -std=c++20
gcc-ar rcs ./dist/windows_x86_64/autoloaddll.lib ./tmp/autoloaddll.o
cp ./tmp/autoloaddll.o ./dist/windows_x86_64/autoloaddll.o


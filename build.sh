mkdir ./tmp -p
mkdir ./dist/linux_x86_64 -p

g++ ./src/autoloaddll.cpp -c -o ./tmp/autoloaddll.o -Ofast -std=c++20
ar rcs ./dist/linux_x86_64/libautoloaddll.a ./tmp/autoloaddll.o
cp ./tmp/autoloaddll.o ./dist/linux_x86_64/autoloaddll.o


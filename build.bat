echo off
cls

del pallet-gen.exe
del terramosaic.exe
md build

echo Compiling "pallet-gen.cpp"...
g++ -Wall -o pallet-gen pallet-gen.cpp lib/colors.cpp lib/mosaic.cpp -I.

echo Compiling "main.cpp"...
g++ -Wall -o terramosaic main.cpp ./lib/colors.cpp ./lib/mosaic.cpp  -I.

move pallet-gen.exe ./build
move terramosaic.exe ./build

echo Done!
echo off
cls

del template-gen.exe
del terramosaic.exe
md build

echo Compiling "template-gen.cpp"...
g++ -Wall -o template-gen template-gen.cpp lib/colors.cpp lib/mosaic.cpp -I.

echo Compiling "main.cpp"...
g++ -Wall -o terramosaic main.cpp ./lib/colors.cpp ./lib/mosaic.cpp  -I.

move template-gen.exe ./build
move terramosaic.exe ./build

echo Done!
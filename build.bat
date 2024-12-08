echo off
cls

del template-gen.exe
del terramosaic.exe

echo Compiling "template-gen.cpp"...
g++ -Wall -o template-gen template-gen.cpp lib/colors.cpp lib/image.cpp -I.

echo Compiling "main.cpp"...
g++ -Wall -o terramosaic main.cpp ./lib/colors.cpp ./lib/image.cpp  -I.

echo Done!
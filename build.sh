mkdir build

echo Compiling "pallet-gen.cpp"...
g++ -Wall -o pallet-gen pallet-gen.cpp lib/pallet.cpp lib/colors.cpp lib/mosaic.cpp -I.

echo Compiling "main.cpp"...
g++ -Wall -o terramosaic main.cpp lib/pallet.cpp ./lib/colors.cpp ./lib/mosaic.cpp  -I.

mv pallet-gen ./build
mv terramosaic ./build

echo Done!

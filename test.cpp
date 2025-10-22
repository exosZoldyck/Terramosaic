#include <iostream>
#include <fstream>
#include <cstring>
#include <cstdint>
#include "lib/base64.cpp"

using namespace std;

int main(){
    std::ifstream infile("explosive_bunny_mosaic.png"); 

    infile.seekg(0, std::ios::end);
    size_t length = infile.tellg();
    infile.seekg(0, std::ios::beg);

    char *buffer = new char[length]; 

    infile.read(buffer, length);

    string binStr = "";

    for (int i = 0; i < length; i++) {
        binStr += buffer[i];
    }

    string output = base64_encode(binStr, false);
    // This is why I fucking hate coding sometimes
    // Hours of sleep lost over 1 FUCKING BYTE 
    // BEING SET TO "o" INSTEAD OF "0"
    output[6] = '0';

    ofstream tilesPallet_stream("test3.txt");
	tilesPallet_stream << output;
	tilesPallet_stream.close();

    return 0;
}
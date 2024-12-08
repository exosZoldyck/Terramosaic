#ifndef MOSAIC_H
#define MOSAIC_H

#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <map>
#include <fstream> 
#include "colors.h"

using namespace std;

class Mosaic{
    public:
        static string imageName;
        static unsigned int imageWidth;
        static unsigned int imageHeight;
        static string palletTilesDirPath;

        static vector<palletTile> fetchPalletTiles(string palletFilePath);

        static vector<RGBColor> fetchImagePixelRGBColors(string filePath_String, bool setImageResVars);

        static vector<CIELABColor> fetchImagePixelCIELABColors(int argc, char *argv[]);

        static void generateMosaicImageFile(vector<Tile> tiles, vector<palletTile> palletTiles, string palletTilesDirPath, bool debug);
};

#endif
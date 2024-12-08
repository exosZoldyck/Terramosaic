#ifndef IMAGE_H
#define IMAGE_H

#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <map>
#include "colors.h"

using namespace std;

class Image{
    public:
        static string imageName;
        static unsigned int imageWidth;
        static unsigned int imageHeight;

        static vector<RGBColor> fetchImagePixelRGBColors(string filePath_String, bool setImageResVars);

        static vector<CIELABColor> fetchImagePixelCIELABColors(int argc, char *argv[]);

        static void generateMosaicImageFile(vector<Tile> tiles, vector<palletTile> palletTiles, string palletTilesDirPath, bool debug);
};

#endif
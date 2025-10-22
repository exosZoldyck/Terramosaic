#ifndef MOSAIC_H
#define MOSAIC_H

#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <map>
#include <fstream> 
#include "tile.h"
#include "colors.h"
#include "pallet.h"

using namespace std;

class Mosaic{
    public:
        static string imageName;
        static unsigned int imageWidth;
        static unsigned int imageHeight;

        static vector<RGBColor> fetchImagePixelRGBColors(string filePath_String, bool setImageResVars, unsigned int *minResolution);

        static vector<CIELABColor> fetchImagePixelCIELABColors(string filePath_String);
        
        static vector<Tile> matchPixelsAndPalletTiles(vector<CIELABColor> pixels, const vector<palletTile> palletTiles, bool silentMode);

        static bool generateMosaicImageFile(vector<Tile> tiles, Pallet pallet, bool silentMode);

        static void generateMosaicJSONFile(vector<Tile> tiles, Pallet pallet, string palletFilePath, uint64_t calculationTime, uint64_t generationTime);
};

#endif
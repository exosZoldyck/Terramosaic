#ifndef PALLET_H
#define PALLET_H

#pragma once
#include <string>
#include <vector>
#include <fstream> 
#include "colors.h"

using namespace std;

struct palletTile{
    string name;
	string fileType;
    CIELABColor labColor;
};

class Pallet{
    public:
        unsigned int minResolution;
        string palletTilesDirPath;
        vector<palletTile> tiles;

        static void fetchPalletTiles(Pallet *self, string palletFilePath);

        Pallet();
};

#endif
#ifndef COLORS_H
#define COLORS_H

#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <cmath>
using namespace std;

class RGBColor{
	public:
		int r;
		int g;
		int b;

		void setValues(int r, int g, int b);

		string toString();

	RGBColor();

	RGBColor(int r, int g, int b);
};

class CIELABColor{
	public:
		double L;
    	double a;
    	double b;

		void setValues(double L, double a, double b);

		string toString();

	CIELABColor();

	CIELABColor(double L, double a, double b);
};

struct Tile{
    int pixelId;
    int palletId;
    double closestDeltaE = 9007199254740991;
};

struct palletTile{
    string name;
	string fileType;
    CIELABColor labColor;
};

class Colors{
	public:
		static RGBColor calcAvrgImgRGBColor(vector<RGBColor> colors, int width, int height);

		static CIELABColor rgbToCIELAB(RGBColor rgbColor);

		static double calcDeltaE(CIELABColor labColor1, CIELABColor labColor2);

		static vector<Tile> matchPixelsAndPalletTiles(vector<CIELABColor> pixels, const vector<palletTile> palletTiles, bool debug);
};

#endif
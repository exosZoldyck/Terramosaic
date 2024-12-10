#ifndef COLORS_H
#define COLORS_H

#pragma once
#include <string>
#include <vector>
#include <cmath>

using namespace std;

class RGBColor{
	public:
		int r;
		int g;
		int b;
		int a;

		void setValues(int r, int g, int b, int a);

		string toString();

	RGBColor();

	RGBColor(int r, int g, int b);

	RGBColor(int r, int g, int b, int a);
};

class CIELABColor{
	public:
		double L;
    	double a;
    	double b;
		bool transparent;

		void setValues(double L, double a, double b, bool transparent);

		string toString();

	CIELABColor();

	CIELABColor(double L, double a, double b);

	CIELABColor(double L, double a, double b, bool transparent);
};

class Colors{
	public:
		static RGBColor calcAvrgImgRGBColor(vector<RGBColor> colors, int width, int height);

		static CIELABColor rgbToCIELAB(RGBColor rgbColor);

		static double calcDeltaE(CIELABColor labColor1, CIELABColor labColor2);
};

#endif
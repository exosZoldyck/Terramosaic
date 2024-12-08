#include "colors.h"

// RGBColor
void RGBColor::setValues(int r, int g, int b){
    this->r = r;
    this->g = g;
    this->b = b;
}

string RGBColor::toString(){
    return "" + to_string(r) + "," +  to_string(g) + "," + to_string(b);
}

RGBColor::RGBColor(){

}

RGBColor::RGBColor(int r, int g, int b){
    this->r = r;
    this->g = g;
    this->b = b;
}



// CIELABColor
void CIELABColor::setValues(double L, double a, double b){
    this->L = L;
    this->a = a;
    this->b = b;
}

string CIELABColor::toString(){
    return "" + to_string(L) + "," +  to_string(a) + "," + to_string(b);
}

CIELABColor::CIELABColor(){

}

CIELABColor::CIELABColor(double L, double a, double b){
    this->L = L;
    this->a = a;
    this->b = b;
}



// Colors
RGBColor Colors::calcAvrgImgRGBColor(vector<RGBColor> colors, int width, int height){
    int r_avrg = 0;
    int g_avrg = 0;
    int b_avrg = 0;

    for (int i = 0; i < width * height; i++){
        r_avrg += colors[i].r;
    }
    r_avrg /= width * height;

    for (int i = 0; i < width * height; i++){
        g_avrg += colors[i].g;
    }
    g_avrg /= width * height;

    for (int i = 0; i < width * height; i++){
        b_avrg += colors[i].b;
    }
    b_avrg /= width * height;

    return RGBColor(r_avrg, g_avrg, b_avrg);
}

CIELABColor Colors::rgbToCIELAB(RGBColor rgbColor){
    double R = (float)rgbColor.r / (float)255.0;
    double G = (float)rgbColor.g / (float)255.0;
    double B = (float)rgbColor.b / (float)255.0;

    R = (R > 0.04045) ? pow((R + 0.055) / 1.055, 2.4) : R / 12.92;
    G = (G > 0.04045) ? pow((G + 0.055) / 1.055, 2.4) : G / 12.92;
    B = (B > 0.04045) ? pow((B + 0.055) / 1.055, 2.4) : B / 12.92;

    R *= 100.0;
    G *= 100.0;
    B *= 100.0;

    // Convert RGB to XYZ
    double X = R * 0.4124564 + G * 0.3575761 + B * 0.1804375;
    double Y = R * 0.2126729 + G * 0.7151522 + B * 0.0721750;
    double Z = R * 0.0193339 + G * 0.1191920 + B * 0.9503041;

    // Normalize to reference white
    X /= 95.047;
    Y /= 100.000;
    Z /= 108.883;

    X = (X > 0.008856) ? pow(X, 1.0 / 3.0) : (903.3 * X + 16.0) / 116.0;
    Y = (Y > 0.008856) ? pow(Y, 1.0 / 3.0) : (903.3 * Y + 16.0) / 116.0;
    Z = (Z > 0.008856) ? pow(Z, 1.0 / 3.0) : (903.3 * Z + 16.0) / 116.0;

    return CIELABColor((0.0, 116.0 * Y - 16.0), (X - Y) * 500.0, (Y - Z) * 200.0);
} 

double Colors::calcDeltaE(CIELABColor labColor1, CIELABColor labColor2){
	double deltaL = labColor2.L - labColor1.L;
	double deltaA = labColor2.a - labColor1.a;
	double deltaB = labColor2.b - labColor1.b;

	double deltaE = sqrt(deltaL * deltaL + deltaA * deltaA + deltaB * deltaB);
	return deltaE;
}

vector<Tile> Colors::matchPixelsAndPalletTiles(vector<CIELABColor> pixels, const vector<palletTile> palletTiles, bool debug = false){
    vector<Tile> tiles;
    tiles.resize(pixels.size());

    // For each pixel (j) create a new tile, and
    // search through every tile (i) in pallet to find
    // the closest match (smallest deltaE)
    for(int j = 0; j < pixels.size(); j++){
        Tile tile;
        tile.pixelId = j;

        for(int i = 0; i < palletTiles.size(); i++){
            palletTile palletTile = palletTiles[i];

            double deltaE = Colors::calcDeltaE(pixels[j], palletTile.labColor);

            if (deltaE < tile.closestDeltaE) {
                tile.closestDeltaE = deltaE;
                tile.palletId = i;
            };
        }

        if (debug) cout << "Progress: "
            << (int)((float)((j+1) * palletTiles.size()) / (float)(palletTiles.size() * pixels.size()) * 100) << "%"
            << "\t" << (j+1) * palletTiles.size() << "/" << (palletTiles.size() * pixels.size()) << endl;

        tiles[j] = tile;
    }

    return tiles;
}
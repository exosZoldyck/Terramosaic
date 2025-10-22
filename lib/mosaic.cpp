#include "mosaic.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#include "json.hpp"

using namespace std::filesystem;
using json = nlohmann::json;

string Mosaic::imageName = "output.png";
unsigned int Mosaic::imageWidth = 0;
unsigned int Mosaic::imageHeight = 0;

vector<RGBColor> Mosaic::fetchImagePixelRGBColors(string filePath_String, bool setImageResVars = false, unsigned int *minResolution_ptr = nullptr){
    int width, height;
    int channels; // 1 for grayscale image, 3 for rgb, 4 for rgba...
    
    // Parse the input string and turn it into a const char array because 
	// stb_image library needs that datatype as the input file path 
    string filePath_abs;
    const char *filePath;
    try{
        filePath_abs = canonical(path(filePath_String)).string();
        filePath = filePath_abs.c_str();
    } catch(exception){
        cout << "error: Couldn't find image file at path \"" << filePath_String << "\"\n";
        vector<RGBColor> empty;
        return empty;
    }

    // Read image file data 
    uint8_t *imageData;
    try{
        imageData = stbi_load(filePath, &width, &height, &channels, 0);
    } catch(exception){
        std::cout << "error: Couldn't fetch image data for \"" << filePath_abs << "\"\n";
        vector<RGBColor> empty;
        return empty;
    }

    if (imageData == NULL){
        std::cout << "error: Couldn't fetch image data for \"" << filePath_abs << "\"\n";
        stbi_image_free(imageData);
        vector<RGBColor> nullColor;
        return nullColor;
    }

    if (minResolution_ptr != nullptr){
        if (*minResolution_ptr > width) *minResolution_ptr = width;
        if (*minResolution_ptr > height) *minResolution_ptr = height;
    }

    // This is used for setting the input image's width and height global vars
    // and is only set when reading the CIELAB color values of the input image
    // and NOT when reading the pallet tile images
    if (setImageResVars) {
        imageWidth = width;
        imageHeight = height;

        for(int i = filePath_String.size() - 1; i >= 0; i--){
            if (filePath_String[i] == '.') {
                imageName = filePath_String.substr(0, i);
                for(int j = filePath_String.size() - 1; j >= 0; j--){
                    if (filePath_String[j] == '/' || filePath_String[j] == '\\'){ 
                        imageName = imageName.substr(j + 1);
                        break;
                    }
                }
                break;
            }
        }

    }

    // Create vector array to store the pixel RGB colors
    // and resize the vector to fit image resolution
    vector<RGBColor> pixels_RGB;
    pixels_RGB.resize(width * height);

    // Iterate over all pixels in image and create RGBColor objects array
    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            uint64_t pos = (y * width + x) * channels;

            uint8_t r = imageData[pos];
            uint8_t g = imageData[pos + 1];
            uint8_t b = imageData[pos + 2];
            
            uint8_t a;
            if (channels == 4){
                // If the pixel in main image is (50% >= transparent), 
                // make it fully transparent for the mosaic generation
                if (setImageResVars) a = ((uint8_t)imageData[pos + 3] >= 128) ? 255 : 0;
                else a = (uint8_t)imageData[pos + 3];
            } 

            pixels_RGB[(y * width + x)].setValues(r, g, b, (channels == 4) ? a : 255);
        }
    }

    // Free image data to prevent memory leak
    stbi_image_free(imageData);

    return pixels_RGB;
} 

vector<CIELABColor> Mosaic::fetchImagePixelCIELABColors(string filePath_String = "input.png"){
    // Fetch the RGB colors vector so it can be 
    // converted to and returned as a CIELAB colors vector

    vector<RGBColor> pixels_RGB = fetchImagePixelRGBColors(filePath_String, true);

    // Convert RGB pixel array to CIELAB pixel color array
    vector<CIELABColor> pixels_CIELAB;
    pixels_CIELAB.resize(pixels_RGB.size());
    for (int i = 0; i < pixels_RGB.size(); i++){
        pixels_CIELAB[i] = Colors::rgbToCIELAB(pixels_RGB[i]);
    }

    return pixels_CIELAB;
}

vector<Tile> Mosaic::matchPixelsAndPalletTiles(vector<CIELABColor> pixels, const vector<palletTile> palletTiles, bool silentMode = false){
    vector<Tile> tiles;
    tiles.resize(pixels.size());

    // For each pixel (j) create a new tile, and
    // search through every tile (i) in pallet to find
    // the closest match (smallest deltaE)
    for(int j = 0; j < pixels.size(); j++){
        Tile tile;
        tile.pixelId = j;

        // This is for if the main image pixel is (50% >= transparent)
        if (pixels[j].transparent) {
            tile.palletId = -1;
            tiles[j] = tile;
            continue;
        }

        for(int i = 0; i < palletTiles.size(); i++){
            palletTile palletTile = palletTiles[i];

            double deltaE = Colors::calcDeltaE(pixels[j], palletTile.labColor);

            if (deltaE < tile.closestDeltaE) {
                tile.closestDeltaE = deltaE;
                tile.palletId = i;
            };
        }

        if (!silentMode) cout << "Progress: "
            << (int)((float)((j+1) * palletTiles.size()) / (float)(palletTiles.size() * pixels.size()) * 100) << "%"
            << " (" << (j+1) * palletTiles.size() << " / " << (palletTiles.size() * pixels.size()) << ") pixel/tile matches calculated\n";

        tiles[j] = tile;
    }

    return tiles;
}

bool Mosaic::generateMosaicImageFile(vector<Tile> tiles, Pallet pallet, bool silentMode = false){
    const unsigned int channels = 4;
    const unsigned int palletTileWidth = pallet.minResolution;
    const unsigned int palletTileHeight = pallet.minResolution;
    const unsigned int width = imageWidth * palletTileWidth;
    const unsigned int height = imageHeight * palletTileHeight;
    
    // Every pixel has "channels" channels
    // Every tile has "palletTileWidth * palletTileHeight" pixels
    // The image is made up of "width * height" tiles
    uint8_t* imageData = new uint8_t[width * height * channels];

    // This is used so the loop doesn't have to load the same 
    // pallet tile image color values every time it want's to 
    // use the same tile
    map<int, vector<RGBColor>> loadedPalletTiles;

    // Load default transparent tile as index "-1"
    vector<RGBColor> transparentTile;
    transparentTile.resize(palletTileWidth * palletTileHeight);
    for (int i = 0; i < palletTileWidth * palletTileHeight; i++)
        transparentTile[i] = RGBColor(0, 0, 0, 0);
    loadedPalletTiles.insert({-1, transparentTile});
    transparentTile.clear();

    // For every i,j tile with index
    for (int j = 0; j < imageHeight; j++) {
        for (int i = 0; i < imageWidth; i++) {
            unsigned int tileIndex = (j * imageWidth + i);

            // Fetch the RGB color pixels of the current tile
            Tile tile = tiles[tileIndex];

            vector<RGBColor> tilePixels_RGB;

            // Check to see if pallet tile can be reused
            if (loadedPalletTiles.count(tile.palletId) > 0){
                tilePixels_RGB = loadedPalletTiles[tile.palletId];
            }
            // If not, load it in and add it to the list of loaded tiles
            else{
                string tileImgFilePath = pallet.palletTilesDirPath + pallet.tiles[tile.palletId].name + pallet.tiles[tile.palletId].fileType;
                tilePixels_RGB = fetchImagePixelRGBColors(tileImgFilePath);
                if (tilePixels_RGB.size() < 1) {
                    cout << "error: Unable to load pallet image file";
                    return 1;
                }

                loadedPalletTiles.insert({tile.palletId, tilePixels_RGB}); 
            }

            // For each x,y pixel of pallet image
            for (int y = 0; y < palletTileHeight; y++) {
                for (int x = 0; x < palletTileWidth; x++) {
                    // Xg = X + (i * W)
                    // Yg = Y + (j * H)
                    // Wt = w * W
                    // INDEXxy = Xg + (Yg * Wt)
                    uint64_t pixelIndex = ((x + (i * palletTileWidth)) + ((y + (j * palletTileHeight)) * width)) * channels; 

                    imageData[pixelIndex] = (uint8_t)tilePixels_RGB[x + (y * palletTileWidth)].r;
                    imageData[pixelIndex + 1] = (uint8_t)tilePixels_RGB[x + (y * palletTileWidth)].g;
                    imageData[pixelIndex + 2] = (uint8_t)tilePixels_RGB[x + (y * palletTileWidth)].b;
                    imageData[pixelIndex + 3] = (uint8_t)tilePixels_RGB[x + (y * palletTileWidth)].a;
                }
            }

            if (!silentMode) cout << "Progress: "
                << (int)(((float)(tileIndex + 1) / (float)(imageWidth * imageHeight)) * 100) << "% ("
                << tileIndex + 1 << " / " << imageWidth * imageHeight << ") tiles generated\n"; 
        }
    }

    // Free the memory because the pallet tiles aren't used after this point
    loadedPalletTiles.clear();

    cout << "\nWriting image data to file...\n";
    stbi_write_png((imageName + "_mosaic.png").c_str(), width, height, channels, imageData, width * channels);
    
    // Free the image data pointer to avoid memory leak
    delete [] imageData;
    imageData = nullptr;

    return 0;
} 

void Mosaic::generateMosaicJSONFile(vector<Tile> tiles, Pallet pallet, string palletFilePath, uint64_t calculationTime, uint64_t generationTime){
    string jsonText = "{\"width\": " + to_string(imageWidth) + ", "
        + "\"height\": " + to_string(imageHeight)+ ", "
        + "\"palletFilePath\": \"" + palletFilePath + "\", "
        + "\"calculationTime\": " + to_string(calculationTime) + ", "
        + "\"generationTime\": " + to_string(generationTime) + ", "
        + "\"tiles\": [";

    // For every i,j pixel of image
    for (int j = 0; j < imageHeight; j++) {
        for (int i = 0; i < imageWidth; i++) {
            unsigned int tileIndex = (j * imageWidth + i);

            jsonText += "{\"palletTileId\": " + to_string(tiles[tileIndex].palletId) + ", "
                + "\"palletTileName\": \""
                + ((tiles[tileIndex].palletId >= 0) ? pallet.tiles[tiles[tileIndex].palletId].name : "none") + "\"}";
            if (j + 1 < imageHeight || i + 1  < imageWidth) jsonText += ", ";
        }
    }

    jsonText += "]}";

	// Write to tiles pallet JSON file
	ofstream tilesPallet_stream(imageName + "_mosiac.json");
	tilesPallet_stream << jsonText;
	tilesPallet_stream.close();

    return;
}
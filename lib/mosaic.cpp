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
unsigned int Mosaic::minResolution = 0;
string Mosaic::palletTilesDirPath = "pallet.json";

vector<palletTile> Mosaic::fetchPalletTiles(string palletFilePath){
    string jsonText;

    try{
        ifstream json_stream(palletFilePath);
        while (getline(json_stream, jsonText)) {
            continue;
        }
        json_stream.close(); 
        json jsonData = json::parse(jsonText);

        Mosaic::minResolution = jsonData["minWidthHeight"]; 
        Mosaic::palletTilesDirPath = jsonData["dirPath"];

        json jsonData_tiles = jsonData["tiles"];
        int jsonData_count = 0; 
        while (jsonData_tiles[jsonData_count] != nullptr) jsonData_count++;

        vector<palletTile> tiles;
        tiles.resize(jsonData_count);
        for (int i = 0; i < jsonData_count; i++){
            tiles[i].name = jsonData_tiles[i]["name"];
            tiles[i].fileType = jsonData_tiles[i]["fileType"];

            tiles[i].labColor = CIELABColor(
                jsonData_tiles[i]["CIELABColor"]["L"], 
                jsonData_tiles[i]["CIELABColor"]["a"],
                jsonData_tiles[i]["CIELABColor"]["b"]
            );
        }

        return tiles;
    } catch(exception) {
        vector<palletTile> tiles;
        return tiles; 
    }
}

vector<RGBColor> Mosaic::fetchImagePixelRGBColors(string filePath_String, bool setImageResVars = false, unsigned int *minResolution_ptr = nullptr){
    int width, height;
    int channels; // 1 for grayscale image, 3 for rgb, 4 for rgba...
    
    // Parse the input string and turn it into a const char array because 
	// stb_image library needs that datatype as the input file path 
    string filePath_abs = canonical(path(filePath_String)).string();
    const char *filePath = filePath_abs.c_str();

    // Read image file data 
    uint8_t *imageData = stbi_load(filePath, &width, &height, &channels, 0);

    if (imageData == NULL){
        std::cout << "Error: Unable to load file \"" << filePath << "\"" << endl;
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

vector<CIELABColor> Mosaic::fetchImagePixelCIELABColors(int argc, char *argv[]){
    string filePath_String = "input.png";

    // Parse the input strings from argv if args are defined
    // and turn them into into a const char array because 
    // stb_image library needs that datatype as the input file path 
    if (argc > 1) filePath_String = argv[1];

    vector<RGBColor> pixels_RGB = fetchImagePixelRGBColors(filePath_String, true);

    // Convert RGB pixel array to CIELAB pixel color array
    vector<CIELABColor> pixels_CIELAB;
    pixels_CIELAB.resize(pixels_RGB.size());
    for (int i = 0; i < pixels_RGB.size(); i++){
        pixels_CIELAB[i] = Colors::rgbToCIELAB(pixels_RGB[i]);
    }

    return pixels_CIELAB;
}

void Mosaic::generateMosaicImageFile(vector<Tile> tiles, vector<palletTile> palletTiles, bool debug = false){
    const unsigned int channels = 4;
    const unsigned int palletTileWidth = minResolution;
    const unsigned int palletTileHeight = minResolution;
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
                string tileImgFilePath = palletTilesDirPath + palletTiles[tile.palletId].name + palletTiles[tile.palletId].fileType;
                tilePixels_RGB = fetchImagePixelRGBColors(tileImgFilePath);
                if (tilePixels_RGB.size() < 1) return;

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
        }
    }

    // Free the memory because the pallet tiles aren't used after this point
    loadedPalletTiles.clear();

    stbi_write_png((imageName + "_mosaic.png").c_str(), width, height, channels, imageData, width * channels);
    
    // Free the image data pointer to avoid memory leak
    delete [] imageData;
    imageData = nullptr;

    return;
} 

void Mosaic::generateMosaicJSONFile(vector<Tile> tiles, vector<palletTile> palletTiles, string palletFilePath, uint64_t calculationTime, uint64_t generationTime){
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
                + ((tiles[tileIndex].palletId >= 0) ? palletTiles[tiles[tileIndex].palletId].name : "none") + "\"}";
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
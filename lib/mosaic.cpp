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

vector<RGBColor> Mosaic::fetchImagePixelRGBColors(string filePath_String, bool setImageResVars = false){
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

            unsigned int r = imageData[pos];
            unsigned int g = imageData[pos + 1];
            unsigned int b = imageData[pos + 2];

            pixels_RGB[(y * width + x)].setValues(r, g, b);
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

void Mosaic::generateMosaicImageFile(vector<Tile> tiles, vector<palletTile> palletTiles, string palletTilesDirPath, bool debug = false){
    const unsigned int width = imageWidth;
    const unsigned int height = imageHeight;
    const unsigned int channels = 3;
    const unsigned int palletTileWidth = 16;
    const unsigned int palletTileHeight = 16;
    
    // Every pixel has "channels" channels
    // Every tile has "palletTileWidth * palletTileHeight" pixels
    // The image is made up of "width * height" tiles
    uint8_t* imageData = new uint8_t[(width * palletTileWidth) * (height * palletTileHeight) * channels];
    vector<vector<vector<uint8_t>>> imageData_matrix(width * palletTileWidth, vector<vector<uint8_t>>(height * palletTileHeight, vector<uint8_t>(3)));
    
    // This is used so the loop doesn't have to load the same 
    // pallet tile image color values every time it want's to 
    // use the same tile
    map<int, vector<RGBColor>> loadedPalletTiles;

    // For every i,j tile with index
    for (int j = 0; j < height; j++) {
        for (int i = 0; i < width; i++) {
            unsigned int tileIndex = (j * width + i);

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
                    unsigned globalX = x + i * palletTileWidth;
                    unsigned globalY = y + j * palletTileHeight;

                    imageData_matrix[globalX][globalY][0] = (uint8_t)tilePixels_RGB[y * palletTileWidth + x].r;
                    imageData_matrix[globalX][globalY][1] = (uint8_t)tilePixels_RGB[y * palletTileWidth + x].g;
                    imageData_matrix[globalX][globalY][2] = (uint8_t)tilePixels_RGB[y * palletTileWidth + x].b;
                }
            }
        }
    }

    // Free the memory because the pallet tiles aren't used after this point
    loadedPalletTiles.clear();

    // if (debug) for (int x = 0; x < imageData_matrix.size(); x++) {
    //     cout << endl;
    //     for (int y = 0; y < imageData_matrix[x].size(); y++) {
    //         cout << "rgb(" << (int)imageData_matrix[x][y][0] << ","
    //             << (int)imageData_matrix[x][y][1] << ","
    //             << (int)imageData_matrix[x][y][2] << ") ";
    //     }
    // }

    // Transform the 2D array of RGBs into a 1D RGB array
    for (int y = 0; y < height * palletTileHeight; y++) {
        for (int x = 0; x < width * palletTileWidth; x++) {
            uint64_t pixelIndex = (y * width * palletTileWidth + x) * channels;

            imageData[pixelIndex] = imageData_matrix[x][y][0];
            imageData[pixelIndex + 1] = imageData_matrix[x][y][1];
            imageData[pixelIndex + 2] = imageData_matrix[x][y][2];
        }
    }

    imageName += "_mosaic.png";
    stbi_write_png(imageName.c_str(), width * palletTileWidth, height * palletTileHeight, channels, imageData, width * palletTileWidth * channels);
    
    // Free the image data pointer to avoid memory leak
    delete [] imageData;
    imageData = nullptr;

    return;
} 
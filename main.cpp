#include <iostream>
#include <fstream> 
#include <chrono>
#include <filesystem>
#include "lib/json.hpp"
#include "lib/colors.h"
#include "lib/image.h"

using namespace std;
using namespace std::filesystem;
using json = nlohmann::json;

#define debug false

string palletTilesDirPath = "";

vector<palletTile> fetchPalletTiles(string palletFilePath = "pallet.json"){
    string jsonText;

    try{
        ifstream json_stream(palletFilePath);
        while (getline(json_stream, jsonText)) {
            continue;
        }
        json_stream.close(); 
        json jsonData = json::parse(jsonText);

        palletTilesDirPath = jsonData["dirPath"];

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

uint64_t timeSinceEpochMillisec() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

int main(int argc, char *argv[]) {
    string palletFilePath = "pallet.json";
    
    string binaryPath = (string)argv[0];
    for (int i = 0; i < binaryPath.size(); i++) 
        if (binaryPath[i] == '\\') 
            binaryPath[i] = '/';
    palletFilePath = binaryPath.substr(0, binaryPath.find_last_of("/") + 1) + palletFilePath;

    cout << "Loading tile pallet from \"" << palletFilePath << "\"..." << endl;
    const vector<palletTile> palletTiles = fetchPalletTiles(palletFilePath);
    
    if (palletTiles.size() < 1) {
        cout << "Error: Unable to read \""<< palletFilePath <<"\"" << endl;
        system("PAUSE");
        return 1;
    }
    
    if (debug){
        cout << "------------------- Loaded tiles -------------------" << endl;
        for(int i = 0; i < palletTiles.size(); i++){
            cout << palletTiles[i].name << palletTiles[i].fileType << "\t| lab("
                << palletTiles[i].labColor.L << ", "
                << palletTiles[i].labColor.a << ", "
                << palletTiles[i].labColor.b << ")" << endl;
        }
        cout << "----------------------------------------------------" << endl << endl;
    }
    cout << "Loaded tiles: " << palletTiles.size() << endl << endl; 

    cout << "Creating image CIELAB color array ..." << endl;
    vector<CIELABColor> pixels_CIELAB = Image::fetchImagePixelCIELABColors(argc, argv);
    if (pixels_CIELAB.size() < 1) return 1;
    cout << "Image resolution: " << Image::imageWidth << "x" << Image::imageHeight << " (" << pixels_CIELAB.size() << "px)" << endl << endl;

    cout << "Calculating closest pixel/tile color matches..." << endl;
    uint64_t matchStartTime = timeSinceEpochMillisec();
    vector<Tile> tiles = Colors::matchPixelsAndPalletTiles(pixels_CIELAB, palletTiles, debug);
    uint64_t matchEndTime = timeSinceEpochMillisec();

    if (debug) for(int i = 0; i < tiles.size(); i++){
        cout << tiles[i].pixelId << "\t" << tiles[i].palletId << "\t" << tiles[i].closestDeltaE << endl;
    }

    cout << "Generating mosaic image file..." << endl;
    uint64_t generationStartTime = timeSinceEpochMillisec();
    Image::generateMosaicImageFile(tiles, palletTiles, palletTilesDirPath, debug);
    uint64_t generationEndTime = timeSinceEpochMillisec();

    cout << endl << "Done!" << endl;

    cout << endl << "Pixels processed: " << pixels_CIELAB.size()
        << endl << "Match calculations: " << pixels_CIELAB.size() * palletTiles.size()
        << endl << "Match calculation time: " 
        << (double)(matchEndTime - matchStartTime) / (double)1000 << " s" << endl
        << endl << "Mosaic generation time: " 
        << (double)(generationEndTime - generationStartTime) / (double)1000 << " s" << endl 
        << endl << "Total time elapsed: " 
        << (double)((matchEndTime - matchStartTime) + (generationEndTime - generationStartTime)) / (double)1000 
        << " s"  << endl << endl;

    // Pause before exiting
    uint64_t execFinishTime = timeSinceEpochMillisec();
    while(timeSinceEpochMillisec() < execFinishTime + 1000);

	return 0;
} 
#include <iostream>
#include <chrono>
#include "lib/colors.h"
#include "lib/mosaic.h"

using namespace std;

#define debug false

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
    const vector<palletTile> palletTiles = Mosaic::fetchPalletTiles(palletFilePath);
    
    if (palletTiles.size() < 1) {
        cout << "Error: Unable to read \""<< palletFilePath <<"\"" << endl;
        system("PAUSE");
        return 1;
    }
    
    string loadedTiles_String = "\n------------------- Loaded tiles -------------------\n";
        for(int i = 0; i < palletTiles.size(); i++){
            loadedTiles_String += palletTiles[i].name + palletTiles[i].fileType + " | lab("
                + to_string(palletTiles[i].labColor.L) + ", "
                + to_string(palletTiles[i].labColor.a) + ", "
                + to_string(palletTiles[i].labColor.b) + ")\n";
        }
    loadedTiles_String += "----------------------------------------------------\n\n";
    cout << loadedTiles_String;
    cout << "Loaded tiles: " << palletTiles.size() << endl << endl; 

    cout << "Creating image CIELAB color array ..." << endl;
    vector<CIELABColor> pixels_CIELAB = Mosaic::fetchImagePixelCIELABColors(argc, argv);
    if (pixels_CIELAB.size() < 1) return 1;
    cout << "Image resolution: " << Mosaic::imageWidth << "x" << Mosaic::imageHeight << " (" << pixels_CIELAB.size() << "px)" << endl << endl;

    cout << "Calculating closest pixel/tile color matches..." << endl;
    uint64_t matchStartTime = timeSinceEpochMillisec();
    vector<Tile> tiles = Colors::matchPixelsAndPalletTiles(pixels_CIELAB, palletTiles, debug);
    uint64_t matchEndTime = timeSinceEpochMillisec();

    if (debug) for(int i = 0; i < tiles.size(); i++){
        cout << tiles[i].pixelId << "\t" << tiles[i].palletId << "\t" << tiles[i].closestDeltaE << endl;
    }

    cout << "Generating mosaic image file..." << endl;
    uint64_t generationStartTime = timeSinceEpochMillisec();
    Mosaic::generateMosaicImageFile(tiles, palletTiles, Mosaic::palletTilesDirPath, debug);
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
#include <iostream>
#include <chrono>
#include <filesystem>
#include <vector>
#include "lib/colors.h"
#include "lib/mosaic.h"
#include "lib/pallet.h"
#include "lib/tile.h"

using namespace std;
using namespace std::filesystem;

#define debug false

uint64_t timeSinceEpochMillisec() {
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

bool endsWith(const std::string& str, const std::string& suffix) {
    if (str.length() < suffix.length()) {
        return false;
    }
    return str.substr(str.length() - suffix.length()) == suffix;
}

int main(int argc, char *argv[]) {
    string inputImagePath = "";
    string palletFilePath = "pallet.json";
    bool silentMode = false;


    
    // Parse the input args
    for (int i = 1; i < argc; i++){
        string arg = (string)*(argv + i);
        
        string arg_next = (i + 1 >= argc) ? "" : (string)*(argv + (i + 1)); // Checks if next arg exists

        if (arg == "--pallet-path" || arg == "-p" ){
            if (arg_next == "") {
                cout << "error: Undefined pallet file path!\n";
                return 0;
            }

            if(endsWith(arg_next, ".json")) {
                try{
                    palletFilePath = absolute(relative(path(arg_next))).string();
                } catch(exception){
                    cout << "error: Missing pallet file or invalid path!\n";
                    return 0;
                }
            } else {
                cout << "error: File must be \".json\"!\n";
                return 0;
            }
            
            i++; // skip over next argument because it's a parameter
        }
        else if (arg == "--silent" || arg == "-s" ){ 
            silentMode = true;
        }
        else if (inputImagePath == "") { // input image path
            try{
                inputImagePath = absolute(relative(path(arg))).string();
            } catch(exception){
                cout << "error: Missing input image file or invalid path!\n";
                return 0;
            }
        }
    }



    cout << "Loading tile pallet from \"" << palletFilePath << "\"..." << "\n";
    Pallet pallet = Pallet();
    Pallet *pallet_ptr = &pallet;
    // This function sets the value of "pallet" internally using a pointer
    Pallet::fetchPalletTiles(pallet_ptr, palletFilePath);
    pallet_ptr = nullptr; 
    
    if (pallet.tiles.size() < 1) {
        cout << "Error: Unable to read \""<< palletFilePath <<"\"" << "\n";
        system("PAUSE");
        return 1;
    }
    
    string loadedTiles_String = "\n------------------- Loaded tiles -------------------\n";
        for(int i = 0; i < pallet.tiles.size(); i++){
            loadedTiles_String += pallet.tiles[i].name + pallet.tiles[i].fileType + " | lab("
                + to_string(pallet.tiles[i].labColor.L) + ", "
                + to_string(pallet.tiles[i].labColor.a) + ", "
                + to_string(pallet.tiles[i].labColor.b) + ")\n";
        }
    loadedTiles_String += "----------------------------------------------------\n\n";
    cout << loadedTiles_String;
    cout << "Loaded tiles: " << pallet.tiles.size() << "\n" << "\n"; 

    cout << "Creating image CIELAB color array ..." << "\n";
    vector<CIELABColor> pixels_CIELAB = Mosaic::fetchImagePixelCIELABColors(inputImagePath);
    if (pixels_CIELAB.size() < 1) return 1;
    cout << "Image resolution: " << Mosaic::imageWidth << "x" << Mosaic::imageHeight << " (" << pixels_CIELAB.size() << "px)" << "\n" << "\n";

    cout << "Calculating closest pixel/tile color matches..." << "\n";
    uint64_t matchStartTime = timeSinceEpochMillisec();
    vector<Tile> tiles = Mosaic::matchPixelsAndPalletTiles(pixels_CIELAB, pallet.tiles, silentMode);
    uint64_t matchEndTime = timeSinceEpochMillisec();

    if (debug) for(int i = 0; i < tiles.size(); i++){
        cout << tiles[i].pixelId << "\t" << tiles[i].palletId << "\t" << tiles[i].closestDeltaE << "\n";
    }

    cout << "Generating mosaic image file..." << "\n";
    uint64_t generationStartTime = timeSinceEpochMillisec();
    try{
        bool result = Mosaic::generateMosaicImageFile(tiles, pallet, silentMode);
        
        // If functions return "true" throw error
        if (result) {
            cout << "error: Unable to write image file" << "\n";
            return 1;
        }
    } catch (exception) {
        cout << "error: Unable to write image file" << "\n";
        return 1;
    }
    uint64_t generationEndTime = timeSinceEpochMillisec();

    try{
        Mosaic::generateMosaicJSONFile(tiles, pallet, palletFilePath, matchEndTime - matchStartTime, generationEndTime - generationStartTime);
    } catch (exception) {
        cout << "Warning: Unable to write JSON file" << "\n";
        system("PAUSE");
    }


    
    cout << "\n" << "Done!" << "\n";

    cout << "\n" << "Pixels processed: " << pixels_CIELAB.size()
        << "\n" << "Match calculations: " << pixels_CIELAB.size() * pallet.tiles.size()
        << "\n" << "Match calculation time: " 
        << (double)(matchEndTime - matchStartTime) / (double)1000 << " s" << "\n"
        << "\n" << "Mosaic generation time: " 
        << (double)(generationEndTime - generationStartTime) / (double)1000 << " s" << "\n" 
        << "\n" << "Total time elapsed: " 
        << (double)((matchEndTime - matchStartTime) + (generationEndTime - generationStartTime)) / (double)1000 
        << " s"  << "\n" << "\n";

    // Pause before exiting
    uint64_t execFinishTime = timeSinceEpochMillisec();
    while(timeSinceEpochMillisec() < execFinishTime + 1500);

	return 0;
} 
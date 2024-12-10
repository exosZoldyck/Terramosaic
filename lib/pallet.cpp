#include "pallet.h"
#include "json.hpp"

using json = nlohmann::json;

Pallet::Pallet(){

}

void Pallet::fetchPalletTiles(Pallet *self, string palletFilePath){
    string jsonText;

    try{
        ifstream json_stream(palletFilePath);
        while (getline(json_stream, jsonText)) {
            continue;
        }
        json_stream.close(); 
        json jsonData = json::parse(jsonText);

        self->minResolution = jsonData["minWidthHeight"]; 
        self->palletTilesDirPath = jsonData["dirPath"];

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

        self->tiles = tiles;
        return;
    } catch(exception) {
        vector<palletTile> tiles;
        self->tiles = tiles;
        return; 
    }
}
#include <iostream>
#include <string>
#include <vector>
#include <filesystem> 
#include <fstream> 
#include "lib/colors.h"
#include "lib/mosaic.h"

using namespace std;
using namespace std::filesystem;

bool endsWith(const std::string& str, const std::string& suffix) {
    if (str.length() < suffix.length()) {
        return false;
    }
    return str.substr(str.length() - suffix.length()) == suffix;
}

int main(int argc, char *argv[]) {
	vector<string> filePath_List;
	string jsonText = "{\"dirPath\": ";
	if (argc <= 1){
		std::cout << "Error: No arg provided" << endl;
		system("PAUSE");
		return 1;
	}

	path p = argv[1];
	if (argc == 2 && is_directory(p)){ 
		string path_string = p.string();

		for(int i = 0; i < path_string.size(); i++){
			if (path_string[i] == '\\') path_string[i] = '/';
		}
		if (!endsWith(path_string, "/")) path_string += "/";
		
		jsonText += "\"" + path_string + "\", ";

		for (auto const & dir_entry : directory_iterator{p}){
			string filePath = dir_entry.path().string();
			if (!endsWith(filePath, ".png") &&
				!endsWith(filePath, ".jpg") &&
				!endsWith(filePath, ".bmp") &&
				!endsWith(filePath, ".tga")
			) continue;

			filePath_List.push_back(filePath);
		}
	} 
	else{
		std::cout << "Error: Provided arg must be directory path" << endl;
		system("PAUSE");
		return 1;
	}

	int global_i = 0;
	unsigned int minResolution = 2'147'483'647;
	unsigned int *minResolution_ptr = &minResolution;
	string tilesJSONString = "\"tiles\": [";
	do{
		string filePath_String = filePath_List[global_i];

		vector<RGBColor> pixelsRGB = Mosaic::fetchImagePixelRGBColors(filePath_String, true, minResolution_ptr);

		RGBColor avrgRGBColor = Colors::calcAvrgImgRGBColor(pixelsRGB, Mosaic::imageWidth, Mosaic::imageWidth);
		CIELABColor avrgCIELABColor = Colors::rgbToCIELAB(avrgRGBColor);

		// This just parses the image name string from the filename
		// by creating a substring from the last instance of
		// "/" or "\" to the last instance of "."
		string name = filePath_String;
		string fileType = "";
		for(int i = filePath_String.size() - 1; i >= 0; i--){
			if (filePath_String[i] == '.') {
				name = filePath_String.substr(0, i);
				fileType = filePath_String.substr(i);
				for(int j = filePath_String.size() - 1; j >= 0; j--){
					if (filePath_String[j] == '/' || filePath_String[j] == '\\'){ 
						name = name.substr(j + 1);
						break;
					}
				}
				break;
			}
		}

		tilesJSONString += "{\"name\": \"" + name + "\", "
			+ "\"fileType\": \"" + fileType +
			+ "\", \"CIELABColor\": {"
			+ "\"L\": " + to_string(avrgCIELABColor.L) + ", " 
			+ "\"a\": " + to_string(avrgCIELABColor.a) + ", " 
			+ "\"b\": " + to_string(avrgCIELABColor.b) + "" 
			+ "}}";

		if (global_i + 1 < filePath_List.size()) tilesJSONString += ", ";

		std::cout << "Added \"" << name << "\" with " << "lab(" 
			<< avrgCIELABColor.toString() << ")" << endl;

		global_i++;
	} while (global_i < filePath_List.size());

	jsonText += "\"minWidthHeight\": " + to_string(minResolution) + ", " + tilesJSONString + "]}";

	// Write to tiles pallet JSON file
	ofstream tilesPallet_stream("pallet.json");
	tilesPallet_stream << jsonText;
	tilesPallet_stream.close();

	return 0;
} 
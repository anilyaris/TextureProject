//An example main file for the application.

#include <cstdlib>
#include <ctime>
#include <iostream>
#include "CLComponents.hpp"
#include "Recolor.hpp"

#define WEIGHT 2.5
//"20 216 21 105 224 152 76 101 74 13"

void mn(string texturePath, string outPath, string mapInfo, char numRegions) {

	try {

		vector<ColorRGB> colors;
		srand(time(NULL));

		CLComponents cl;
		Recolor recolor(cl, numRegions, WEIGHT);

		while (numRegions-- > 0) colors.push_back(ColorRGB(RAND, RAND, RAND));

		recolor.setColors(colors);
		recolor.recolor(texturePath, outPath, mapInfo);

	}
	catch (string s) {

		throw;

	}

}

int main(int argc, char** argv) {

	if (argc < 4) {

		std::cerr << "Wrong argument format." << std::endl;
		return -1;

	}

	try { 
		
		unsigned int numRegions;
		string mapInfo(argv[3]);

		if (mapInfo.length() < 5 || mapInfo.compare(mapInfo.length() - 4, 4, ".png") != 0)
			numRegions = mapInfo.find_first_of(' ') == string::npos ? stoi(mapInfo) : find_spaces(mapInfo) / 2 + 1;
		else numRegions = stoi(mapInfo.substr(mapInfo.find_last_of('_') + 1, mapInfo.length() - mapInfo.find_last_of('_') - 5));

		mn(argv[1], argv[2], argv[3], numRegions); 	
	
	}
	catch (string s) { 
		
		std::cerr << s << std::endl;

	}

	return 0;

}

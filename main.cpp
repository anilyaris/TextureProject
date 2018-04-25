//An example main file for the application.

#include <cstdlib>
#include <ctime>
#include <iostream>
#include "CLComponents.hpp"
#include "Recolor.hpp"

int main(int argc, char** argv) {

	if (argc != 5) {

		std::cerr << "Wrong argument format: There should be 3 file paths for texture, map and output images and 1 number to specify number of regions in the map." << std::endl;
		return -1;

	}

	try {

		CLComponents cl;
		cl.init();
		Recolor recolor(cl);

		vector<ColorRGB> colors;
		srand(time(NULL));

		int numRegions = atoi(argv[4]);
		while (numRegions-- > 0) colors.push_back(ColorRGB(RAND, RAND, RAND));

		recolor.setColors(colors);
		recolor.recolor(argv[1], argv[2], argv[3]);

	}
	catch (string s) {

		std::cerr << s << std::endl;

	}

	return 0;

}
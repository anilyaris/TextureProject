#pragma once

#include <iostream>
#include <cstdint>
#include <vector>
#include <string>
#include <CL/cl.hpp>
#include "PNG.hpp"
#include "CLComponents.hpp"
#include "util.hpp"

using std::string;
using std::vector;

struct ColorHSV;

struct ColorRGB {

	int16_t R, G, B;
	bool isEmpty;

	//Default constructor creates an empty color that signifies no recoloring.
	ColorRGB() { isEmpty = true; }
	ColorRGB(int16_t r, int16_t g, int16_t b);

	ColorHSV convert();

	cl_short3 operator+(ColorRGB& rhs);
	cl_short3 operator-(ColorRGB& rhs);

};

struct ColorHSV {

	double H, S, V;

	ColorHSV() {}
	ColorHSV(double h, double s, double v) : H(h), S(s), V(v) {}

	cl_double3 operator+(ColorHSV& rhs);
	cl_double3 operator-(ColorHSV& rhs);

};

typedef cl_uint2 Coord;
typedef cl_uint3 Border;

class Recolor {

	private:

		PNG_RW texture, map;
		vector<ColorRGB> colors;
		bool antialias;

		CLComponents& cl;

		void mapParse(vector<Coord>& samples, vector<Border>& borders, cl::Buffer& buffer_map);

	public:

		Recolor(CLComponents& clc, bool typ = true) : cl(clc), antialias(typ) {}
		void setColors(vector<ColorRGB>& clrs);

		void recolor(string texturePath, string mapPath, string outPath);
		
};
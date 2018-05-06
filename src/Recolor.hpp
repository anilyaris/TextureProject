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
		unsigned int reg;
		bool antialias;
		cl_double weight;

		CLComponents& cl;

		void mapParse(vector<Coord>& samples, vector<Border>& borders, cl::Buffer& buffer_map);
		void kmeans(cl::Buffer& buffer_texture, cl::Buffer& buffer_map, vector<Coord>& samples, cl_uchar& mode);

	public:

		Recolor(CLComponents& clc, unsigned int r, cl_double w = -1.0, bool typ = true) : cl(clc), reg(r), weight(w), antialias(typ) {}
		void setColors(vector<ColorRGB>& clrs);

		void recolor(string texturePath, string outPath, string mapInfo);
		
};
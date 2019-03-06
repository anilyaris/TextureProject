#pragma once

#include <iostream>
#include <cstdint>
#include <vector>
#include <string>
#include <CL/cl.hpp>
#include "PNG.hpp"
#include "CLComponents.hpp"
#include "util.hpp"
#include <DirectXTex.h>

using std::string;
using std::vector;
using std::pair;

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

	uint8_t findMaxDifference();

};

struct ColorHSV {

	double H, S, V;

	ColorHSV() {}
	ColorHSV(double h, double s, double v) : H(h), S(s), V(v) {}

	cl_double3 operator+(ColorHSV& rhs);
	cl_double3 operator-(ColorHSV& rhs);

};

template <typename vec3_t>
struct diffItem {

	vec3_t diff;
	bool calculated = false;

};

typedef diffItem<cl_short3> rgbDiff;
typedef diffItem<cl_double3> hsvDiff;

typedef cl_uint2 Coord;	//height, width (y,x)
typedef cl_uint3 Border;

class Recolor {

	private:

		PNG_RW texture, map;
		string bin;
		cl::Buffer buffer_map;
		vector<Coord> samples;
		vector<Border> borders;

		vector<diffItem<cl_short3>> rgb_diffs;
		vector<diffItem<cl_double3>> hsv_diffs;
		bool diffs_set;

		vector<ColorRGB> colors;
		unsigned char reg;
		bool antialias;
		cl_double weight;
		cl_uchar eyeRegion = 26;

		CLComponents& cl;
		ID3D11Device* device = nullptr;
		ID3D11DeviceContext* context = nullptr;

		void mapParse();
		void kmeans(cl::Buffer& buffer_texture, cl_uchar& mode);

	public:

		Recolor(CLComponents& clc, unsigned char r = 25, cl_double w = -1.0, bool typ = true) : cl(clc), reg(r), weight(w), antialias(typ), rgb_diffs(r, { 0, 0, 0 }), hsv_diffs(r, { 0.0, 0.0, 0.0 }), diffs_set(false) {}
		void setColors(const vector<ColorRGB>& clrs);		
		void setBin(const string& path);
		
		pair<vector<rgbDiff>&, vector<hsvDiff>&> getDiffs();
		void setDiffs(const vector<rgbDiff>& rgbdiffs, const vector<hsvDiff>& hsvdiffs);
		void resetDiffs();

		void recolor(const string& texturePath, const string& outPath, const string& mapInfo, const texDataInfo& textureBinData, const string& pmPath = "", uint8_t eyeIrisIdentifier = false);
				
};
#include <cstdint>
#include <cmath>
#include <vector>
#include <algorithm>
#include <CL/cl.hpp>
#include "PNG.hpp"
#include "Recolor.hpp"
#include "util.hpp"
#include "CLCode.h"

using std::vector;
using std::min;
using std::max;
using std::stoi;

//Constructs RGB color with given values, if RAND is provided as the value the corresponding channel will be randomized.
ColorRGB::ColorRGB(int16_t r, int16_t g, int16_t b) {
	
	isEmpty = false;

	R = r == RAND ? rand() % 256 : r;
	G = g == RAND ? rand() % 256 : g;
	B = b == RAND ? rand() % 256 : b;
	
}

//Converts RGB color to HSV.
ColorHSV ColorRGB::convert() {

	double var_R = R;
	double var_G = G;
	double var_B = B;

	double var_Min = min(min(var_R, var_G), var_B);
	double var_Max = max(max(var_R, var_G), var_B);
	double del_Max = var_Max - var_Min;

	double v = var_Max / 255;
	double h, s;

	//This is a gray, no chroma...
	if (del_Max == 0.0) {

		h = 0.0;
		s = 0.0;

	}
	//Chromatic data...
	else {

		s = del_Max / var_Max;

		double del_R = (((var_Max - var_R) / 6) + (del_Max / 2)) / del_Max;
		double del_G = (((var_Max - var_G) / 6) + (del_Max / 2)) / del_Max;
		double del_B = (((var_Max - var_B) / 6) + (del_Max / 2)) / del_Max;

		if (var_R == var_Max) h = del_B - del_G;
		else if (var_G == var_Max) h = (1.0 / 3) + del_R - del_B;
		else if (var_B == var_Max) h = (2.0 / 3) + del_G - del_R;
			
	}

	if (h < 0.0) h = h + 1;
	if (h > 1.0) h = h - 1;

	return ColorHSV(h, s, v);

}

cl_short3 ColorRGB::operator+(ColorRGB& rhs) {

	return { R + rhs.R, G + rhs.G, B + rhs.B };

}

cl_short3 ColorRGB::operator-(ColorRGB& rhs) {

	return { R - rhs.R, G - rhs.G, B - rhs.B };

}

cl_double3 ColorHSV::operator+(ColorHSV& rhs) {

	return { H + rhs.H, S + rhs.S, V + rhs.V };

}

cl_double3 ColorHSV::operator-(ColorHSV& rhs) {

	return { H - rhs.H, S - rhs.S, V - rhs.V };

}

void Recolor::setColors(vector<ColorRGB>& clrs) {

	colors = move(clrs);

}

//Performs the recoloring of the texture.
void Recolor::recolor(string texturePath, string outPath, string mapInfo) {

	try {
		
		texture.read(texturePath);
		cl_uint height = texture.getHeight(), width = texture.getWidth();

		unsigned int bytes = height * width * 3;

		cl::Buffer buffer_texture(cl.context, CL_MEM_READ_ONLY, sizeof(uint8_t) * bytes);
		cl::Buffer buffer_map(cl.context, CL_MEM_READ_WRITE, sizeof(uint8_t) * bytes / 3);

		cl.queue.enqueueWriteBuffer(buffer_texture, CL_TRUE, 0, sizeof(uint8_t) * bytes, texture.getImage());

		vector<Coord> samples;
		vector<Border> borders;

		if (mapInfo.length() < 5 || mapInfo.compare(mapInfo.length() - 4, 4, ".png") != 0) {

			if (weight < 0) throw string("Recoloring error: Negative color weight is not accepted.");
			cl_uchar mode;

			if (mapInfo.find_first_of(' ') == string::npos) {

				mode = 0;
				for (int i = 0; i < reg; i++) samples.push_back({ rand() % height, rand() % width });

			}
				
			else {

				mode = 1;

				Coord sample = { 0, 0 };
				int beginIndex = 0;

				for (int i = 0; i <= mapInfo.size(); i++) {

					if (mapInfo[i] == ' ' || i == mapInfo.size()) {

						if (sample.x == 0) sample.x = stoi(mapInfo.substr(beginIndex, i - beginIndex));
						else if (sample.y == 0) {

							sample.y = stoi(mapInfo.substr(beginIndex, i - beginIndex));
							if (sample.x >= height || sample.y >= width) throw string("Recoloring error: Given sample out of bounds.");
							samples.push_back(sample);
							sample.x = sample.y = 0;

						}

						beginIndex = i + 1;
							
					}

				}				

			}

			if (reg > 25) throw string("Recoloring error: Maximum number of regions allowed is 25.");
			if (reg != colors.size()) throw string("Recoloring error: Number of regions does not match number of provided colors.");

			kmeans(buffer_texture, buffer_map, samples, mode);			

		}

		else {

			weight = -1.0;

			map.read(mapInfo);
			if (height != map.getHeight() || width != map.getWidth() || map.getColor() != 1) throw string("Recoloring error: Map image not suitable for operation.");
			if (texture.getColor() < 3) throw string("Recoloring error: Texture image is grayscale.");

			cl.queue.enqueueWriteBuffer(buffer_map, CL_TRUE, 0, sizeof(uint8_t) * bytes / 3, map.getImage());
			samples.resize(reg);

		}

		mapParse(samples, borders, buffer_map);

		vector<cl_short3> rgb_diff(reg, { 0, 0, 0 });
		vector<cl_double3> hsv_diff(reg, { 0.0, 0.0, 0.0 });

		for (int i = 0; i < samples.size(); i++) {

			Coord sample = samples[i];

			if (!colors[i].isEmpty && sample.x != 0 && sample.y != 0) {

				uint8_t* pixel = texture.getImage() + 3 * (width * sample.x + sample.y);
				ColorRGB sampleColor(*pixel, *(pixel+1), *(pixel+2));
				rgb_diff[i] =  colors[i] - sampleColor;
				hsv_diff[i] =  colors[i].convert() - sampleColor.convert();

			}

		}
				
		cl::Buffer buffer_rgb_diff(cl.context, CL_MEM_READ_ONLY, sizeof(cl_short3) * reg);
		cl::Buffer buffer_hsv_diff(cl.context, CL_MEM_READ_ONLY, sizeof(cl_double3) * reg);
		cl::Buffer buffer_rgb(cl.context, CL_MEM_READ_WRITE, sizeof(uint8_t) * bytes);
		cl::Buffer buffer_hsv(cl.context, CL_MEM_READ_WRITE, sizeof(uint8_t) * bytes);

		cl.queue.enqueueWriteBuffer(buffer_rgb_diff, CL_TRUE, 0, sizeof(cl_short3) * reg, &rgb_diff[0]);
		cl.queue.enqueueWriteBuffer(buffer_hsv_diff, CL_TRUE, 0, sizeof(cl_double3) * reg, &hsv_diff[0]);

		cl.build(RECOLOR_CODE, RECOLOR_LEN);

		cl::Kernel kernel_recolor(cl.program, "recolor");
		kernel_recolor.setArg(0, buffer_texture);
		kernel_recolor.setArg(1, buffer_map);
		kernel_recolor.setArg(2, buffer_rgb_diff);
		kernel_recolor.setArg(3, buffer_hsv_diff);
		kernel_recolor.setArg(4, buffer_rgb);
		kernel_recolor.setArg(5, buffer_hsv);
		cl.queue.enqueueNDRangeKernel(kernel_recolor, cl::NullRange, cl::NDRange(height, width, 2), cl::NullRange);
		cl.queue.finish();

		cl.build(ADD_CODE, ADD_LEN);

		cl::Kernel kernel_add(cl.program, "add");
		kernel_add.setArg(0, buffer_rgb);
		kernel_add.setArg(1, buffer_hsv);
		cl.queue.enqueueNDRangeKernel(kernel_add, cl::NullRange, cl::NDRange(height, width, 3), cl::NullRange);
		cl.queue.finish();

		cl.queue.enqueueReadBuffer(buffer_rgb, CL_TRUE, 0, sizeof(uint8_t) * bytes, texture.getImage());
		
		if (antialias) {

			cl.build(ANTIALIAS_CODE, ANTIALIAS_LEN);

			cl::Buffer buffer_borders(cl.context, CL_MEM_READ_ONLY, sizeof(cl_uint3) * borders.size());
			cl.queue.enqueueWriteBuffer(buffer_borders, CL_TRUE, 0, sizeof(cl_uint3) * borders.size(), &borders[0]);

			cl::Buffer buffer_width(cl.context, CL_MEM_READ_ONLY, sizeof(cl_uint));
			cl.queue.enqueueWriteBuffer(buffer_width, CL_TRUE, 0, sizeof(cl_uint), &width);

			cl::Kernel kernel_antialias(cl.program, "antialias");
			kernel_antialias.setArg(0, buffer_rgb);
			kernel_antialias.setArg(1, buffer_borders);
			kernel_antialias.setArg(2, buffer_width);
			cl.queue.enqueueNDRangeKernel(kernel_antialias, cl::NullRange, cl::NDRange(borders.size(), 3), cl::NullRange);
			cl.queue.finish();

		}

		cl.queue.enqueueReadBuffer(buffer_rgb, CL_TRUE, 0, sizeof(uint8_t) * bytes, texture.getImage());
		texture.write(outPath);		

	}
	catch (string s) {
		
		throw;

	}

}

//Used for obtaining the information from the map file which will be necessary for the recoloring.
void Recolor::mapParse(vector<Coord>& samples, vector<Border>& borders, cl::Buffer& buffer_map) {

	try {

		cl_uint height = texture.getHeight(), width = texture.getWidth();

		cl.build(PARSE_CODE, PARSE_LEN);

		cl::Buffer buffer_samples(cl.context, CL_MEM_WRITE_ONLY, sizeof(cl_uint2) * reg);
		cl::Buffer buffer_preborders(cl.context, CL_MEM_WRITE_ONLY, sizeof(cl_uint3) * height * width);
		cl::Buffer buffer_numborders(cl.context, CL_MEM_READ_WRITE, sizeof(cl_uint));
		cl::Buffer buffer_antialias(cl.context, CL_MEM_READ_ONLY, sizeof(cl_uchar));

		cl_uint numborders = 0;
		cl.queue.enqueueWriteBuffer(buffer_numborders, CL_TRUE, 0, sizeof(cl_uint), &numborders);
		cl.queue.enqueueWriteBuffer(buffer_antialias, CL_TRUE, 0, sizeof(cl_uchar), &antialias);

		cl::Kernel kernel_parse(cl.program, "parse");
		kernel_parse.setArg(0, buffer_map);
		kernel_parse.setArg(1, buffer_samples);
		kernel_parse.setArg(2, buffer_preborders);
		kernel_parse.setArg(3, buffer_numborders);
		kernel_parse.setArg(4, buffer_antialias);
		cl.queue.enqueueNDRangeKernel(kernel_parse, cl::NullRange, cl::NDRange(height, width), cl::NullRange);
		cl.queue.finish();

		if (weight < 0) cl.queue.enqueueReadBuffer(buffer_samples, CL_TRUE, 0, sizeof(cl_uint2) * reg, &samples[0]);
		cl.queue.enqueueReadBuffer(buffer_numborders, CL_TRUE, 0, sizeof(cl_uint), &numborders);
		borders.resize(numborders);
		if (numborders > 0) cl.queue.enqueueReadBuffer(buffer_preborders, CL_TRUE, 0, sizeof(cl_uint3) * numborders, &borders[0]);

	}
	catch (string s) {

		throw;

	}

}

//Used for obtaining regions if no map file is given.
void Recolor::kmeans(cl::Buffer& buffer_texture, cl::Buffer& buffer_map, vector<Coord>& samples, cl_uchar& mode) {

	try {

		cl_uint height = texture.getHeight(), width = texture.getWidth();
		
		vector<cl_double8> dsamples;
		for (Coord coord : samples) {

			unsigned int pixel = width * coord.x + coord.y;
			cl_double R = texture.getImage()[3 * pixel], G = texture.getImage()[3 * pixel + 1], B = texture.getImage()[3 * pixel + 2];
			dsamples.push_back({ weight * R, weight * G, weight * B, (cl_double)coord.x, (cl_double)coord.y, (cl_double)reg, 0., 0. });

		}

		cl::Buffer buffer_samples(cl.context, CL_MEM_READ_WRITE, sizeof(cl_double8) * reg);
		cl::Buffer buffer_weight(cl.context, CL_MEM_READ_ONLY, sizeof(cl_double));
		cl.queue.enqueueWriteBuffer(buffer_weight, CL_TRUE, 0, sizeof(cl_double), &weight);

		bool breakClause;
		do {
			
			cl.queue.enqueueWriteBuffer(buffer_samples, CL_TRUE, 0, sizeof(cl_double8) * reg, &dsamples[0]);
			cl.build(KMEANS_CODE, KMEANS_LEN);

			cl::Kernel kernel_kmeans(cl.program, "kmeans");
			kernel_kmeans.setArg(0, buffer_texture);
			kernel_kmeans.setArg(1, buffer_map);
			kernel_kmeans.setArg(2, buffer_samples);
			kernel_kmeans.setArg(3, buffer_weight);
			cl.queue.enqueueNDRangeKernel(kernel_kmeans, cl::NullRange, cl::NDRange(height, width), cl::NullRange);
			cl.queue.finish();

			if (mode == 1) break;

			vector<cl_double8> prevSamples(reg, cl_double8());
			cl.queue.enqueueReadBuffer(buffer_samples, CL_TRUE, 0, sizeof(cl_double8) * reg, &prevSamples[0]);

			for (int i = 0; i < reg; i++) dsamples[i] = cl_double8();
			cl.queue.enqueueWriteBuffer(buffer_samples, CL_TRUE, 0, sizeof(cl_double8) * reg, &dsamples[0]);

			cl.build(NEWMEANS_CODE, NEWMEANS_LEN);

			cl::Kernel kernel_newmeans(cl.program, "newmeans");
			kernel_newmeans.setArg(0, buffer_texture);
			kernel_newmeans.setArg(1, buffer_map);
			kernel_newmeans.setArg(2, buffer_samples);
			kernel_newmeans.setArg(3, buffer_weight);
			cl.queue.enqueueNDRangeKernel(kernel_newmeans, cl::NullRange, cl::NDRange(height, width), cl::NullRange);
			cl.queue.finish();

			cl.queue.enqueueReadBuffer(buffer_samples, CL_TRUE, 0, sizeof(cl_double8) * reg, &dsamples[0]);

			breakClause = true;
			for (int i = 0; i < reg; i++) {

				dsamples[i].s0 /= dsamples[i].s7;
				dsamples[i].s1 /= dsamples[i].s7;
				dsamples[i].s2 /= dsamples[i].s7;
				dsamples[i].s3 /= dsamples[i].s7;
				dsamples[i].s4 /= dsamples[i].s7;
				dsamples[i].s5 = reg;
				dsamples[i].s7 = 0.;

				breakClause = breakClause && (fabs(dsamples[i].s0 - prevSamples[i].s0) <= weight) 
					&& (fabs(dsamples[i].s1 - prevSamples[i].s1) <= weight)	&& (fabs(dsamples[i].s2 - prevSamples[i].s2) <= weight)
					&& (fabs(dsamples[i].s3 - prevSamples[i].s3) <= 1.0) && (fabs(dsamples[i].s4 - prevSamples[i].s4) <= 1.0);

			}

		} while (!breakClause);


		for (int i = 0; i < reg; i++) {

			samples[i] = { (cl_uint)dsamples[i].s3, (cl_uint)dsamples[i].s4 };
			if (samples[i].x == 0) samples[i].x++;
			if (samples[i].y == 0) samples[i].y++;

		}

	}
	catch (string s) {

		throw;

	}
}
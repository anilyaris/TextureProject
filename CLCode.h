//Contains code strings for the OpenCL programs.

#pragma once

#define ADD_CODE R"(

void kernel add(global unsigned char* rgb, global const unsigned char* hsv) {

	unsigned int pixel = 3 * (get_global_id(0) * get_global_size(1) + get_global_id(1)) + get_global_id(2);
	rgb[pixel] = ((short)rgb[pixel] + (short)hsv[pixel]) / 2;

}

)"

#define ADD_LEN 250

#define ANTIALIAS_CODE R"(

void kernel antialias(global unsigned char* rgb, global const uint3* borders, global const unsigned int* width) {

	uint3 border = borders[get_global_id(0)];
	unsigned int dirs = border.z;
	unsigned char u = (dirs >> 3) % 2, d = (dirs >> 2) % 2, l = (dirs >> 1) % 2, r = dirs % 2;
	
	double num_dirs = (double)(u + d + l + r);
	unsigned int index = 3 * (width[0] * border.x + border.y) + get_global_id(1);
	double val =  (num_dirs + 1) * rgb[index];

	if (u != 0) val = val + (double)rgb[index - 3 * width[0]];
	if (d != 0) val = val + (double)rgb[index + 3 * width[0]];
	if (l != 0) val = val + (double)rgb[index - 3];
	if (r != 0) val = val + (double)rgb[index + 3];

	barrier(CLK_LOCAL_MEM_FENCE | CLK_GLOBAL_MEM_FENCE);
	rgb[index] = round(val / (2 * num_dirs + 1));

}

)"

#define ANTIALIAS_LEN 778

#define PARSE_CODE R"(

void kernel parse(global unsigned char* map, global uint2* samples, global uint3* borders, global unsigned int* numborders, global const unsigned char* antialias) {

	unsigned int pixel = get_global_id(0) * get_global_size(1) + get_global_id(1);
	if (map[pixel] == 0) return;
	if (map[pixel] == 255) {

		unsigned char region = map[pixel + 1];
		map[pixel] = region;
		samples[region / 10 - 1] = (uint2)(get_global_id(0), get_global_id(1));
		return;

	}

	if (antialias[0] != 0) {

		unsigned int border = 0;
		if (get_global_id(0) != 0 && map[pixel] != map[pixel - get_global_size(1)] && map[pixel - get_global_size(1)] != 0) border = border + 8;
		if (get_global_id(0) != (get_global_size(0) - 1) && map[pixel] != map[pixel + get_global_size(1)] && map[pixel + get_global_size(1)] != 0) border = border + 4;
		if (get_global_id(1) != 0 && map[pixel] != map[pixel - 1] && map[pixel - 1] != 0) border = border + 2;
		if (get_global_id(1) != (get_global_size(1) - 1) && map[pixel] != map[pixel + 1] && map[pixel + 1] != 0) border = border + 1;

		if (border != 0) {

			unsigned int borderno = atomic_inc(numborders);
			borders[borderno] = (uint3)(get_global_id(0), get_global_id(1), border);

		}		

	}

}

)"

#define PARSE_LEN 1212

#define RECOLOR_CODE R"(

double3 rgb2hsv(short3 rgb) {

	double var_R = (double)rgb.x;
	double var_G = (double)rgb.y;
	double var_B = (double)rgb.z;

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
		else if (var_G == var_Max) h = ((double)1.0 / 3) + del_R - del_B;
		else if (var_B == var_Max) h = ((double)2.0 / 3) + del_G - del_R;

	}

	if (h < 0.0) h = h + 1;
	if (h > 1.0) h = h - 1;

	return (double3)(h, s, v);

}

short3 hsv2rgb(double3 hsv) {

	double H = hsv.x, S = hsv.y, V = hsv.z;
	short r, g, b;
	if (S == 0.0) r = g = b = round(V * 255);
	else {

		double var_h = H * 6;
		if (var_h == 6.0) var_h = 0.0; //H must be < 1

		int var_i = (int)var_h;
		double var_1 = V * (1 - S);
		double var_2 = V * (1 - S * (var_h - var_i));
		double var_3 = V * (1 - S * (1 - (var_h - var_i)));

		double var_r, var_g, var_b;
		switch (var_i) {

		case 0:
			var_r = V;
			var_g = var_3;
			var_b = var_1;
			break;

		case 1:
			var_r = var_2;
			var_g = V;
			var_b = var_1;
			break;

		case 2:
			var_r = var_1;
			var_g = V;
			var_b = var_3;
			break;

		case 3:
			var_r = var_1;
			var_g = var_2;
			var_b = V;
			break;

		case 4:
			var_r = var_3;
			var_g = var_1;
			var_b = V;
			break;

		default:
			var_r = V;
			var_g = var_1;
			var_b = var_2;

		}

		r = round(var_r * 255);
		g = round(var_g * 255);
		b = round(var_b * 255);

	}

	return (short3)(r, g, b);

}

short sclamp(short val, short mn, short mx) {

	if (val < mn) return mn;
	else if (val > mx) return mx;
	return val;

}

double dclamp(double val, double mn, double mx) {

	if (val < mn) return mn;
	else if (val > mx) return mx;
	return val;

}

double rclamp(double val, double mn, double mx) {

	if (val < mn) return val + (mx - mn);
	else if (val >= mx)	return val - (mx - mn);
	return val;

} 

void kernel recolor(global const unsigned char* texture, global const unsigned char* map, global const short3* rgb_diff, global const double3* hsv_diff, global unsigned char* rgb_out, global unsigned char* hsv_out) {

	unsigned int pixel = get_global_id(0) * get_global_size(1) + get_global_id(1);
	unsigned char region = map[pixel] / 10;
	
	if (region > 0) {
		
		short3 rgb = (short3)(texture[3 * pixel], texture[3 * pixel + 1], texture[3 * pixel + 2]);
		if (get_global_id(2) == 0) {

			rgb = rgb + rgb_diff[region - 1];
			rgb_out[3 * pixel] = sclamp(rgb.x, (short)0, (short)255);
			rgb_out[3 * pixel + 1] = sclamp(rgb.y, (short)0, (short)255);
			rgb_out[3 * pixel + 2] = sclamp(rgb.z, (short)0, (short)255);

		}

		else {

			double3 hsv = rgb2hsv(rgb) + hsv_diff[region - 1];
			hsv.x = rclamp(hsv.x, (double)0.0, (double)1.0);
			hsv.y = dclamp(hsv.y, (double)0.0, (double)1.0);
			hsv.z = dclamp(hsv.z, (double)0.0, (double)1.0);
			
			rgb = hsv2rgb(hsv);
			hsv_out[3 * pixel] = rgb.x;
			hsv_out[3 * pixel + 1] = rgb.y;
			hsv_out[3 * pixel + 2] = rgb.z;

		}

	}

}

)"

#define RECOLOR_LEN 3363
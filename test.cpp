#include <iostream>
#include <cstdint>
#include <cstdlib>
#include <ctime>
#include <utility>
#include <array>
#include <vector>
#include <algorithm>
#include <fstream>
#include <string>
#include <CL/cl.hpp>
#include "CRC.hpp"
#include "PNG.hpp"
#include "CLComponents.hpp"
#include "Recolor.hpp"

using namespace std;

void f(int* a) {
	
	/*
	cout << a << " " << *a << endl;
	a++;
	cout << a << " " << *a << endl;
	
	if (a < 0) {
		throw string("error");
	}
	*/
	
	for (int i = 0; i < 5; i++) (*(a++))++;
	
}

int main() {

	/*CRC crc;
	uint8_t arr[29] = {0x74, 0x45, 0x58, 0x74, 0x53, 0x6F, 0x66, 0x74, 0x77, 0x61, 0x72, 0x65, 0x00, 0x70, 0x61, 0x69, 0x6E, 0x74, 0x2E, 0x6E, 0x65, 0x74, 0x20, 0x34, 0x2E, 0x30, 0x2E, 0x32, 0x31};
	uint32_t c = crc.calculate(arr, 29);
	cout << hex << c << endl;

	int a[4] = {0,1,2,3};
	cout << a << " " << *a << endl;
	cout << "------------" << endl;
	f(a);
	cout << "------------" << endl;
	cout << a << " " << *a << endl;

	uint8_t a[6] = {0xA7, 0x03, 0x4D, 0x85, 0x61, 0xC1};
	uint32_t k = bytesTo32(a);
	cout << k << endl;
	uint8_t b[5];
	_32ToBytes(k, b);
	cout << hex << (unsigned int)b[0] << " " << (unsigned int)b[1] << " " << (unsigned int)b[2] << " " << (unsigned int)b[3] << endl;

	int m = 0;
	int n = move(m);
	cout << n << " " << m << endl;
	m = 1;
	cout << n << " " << m << endl;

	int x[2] = {0,1};
	int y[2];
	y[0] = move(x[0]);
	y[1] = move(x[1]);
	cout << y[0] << " " << y[1] << " " << x[0] << " " << x[1] << endl;
	x[0] = 2;
	x[1] = 3;
	cout << y[0] << " " << y[1] << " " << x[0] << " " << x[1] << endl;

	int a[3] = {5,1,2};
	int b[5];
	copy(a, a+3, b+2);
	cout << b[0] << " " << b[1] << " " << b[2] << " " << b[3] << " " << b[4] << endl;
	a[0] = 3;
	cout << b[0] << " " << b[1] << " " << b[2] << " " << b[3] << " " << b[4] << endl;

	try {
		f(-2);
	}

	catch (string s) {
		cout << s << endl;
	}

	char a[4] = {'I', 'H', 'D', 'R'};
	array<char, 4> b;
	copy(a, a+4, b.begin());
	cout << b[0] << endl;
	a[0] = 'K';
	cout << b[0] << endl;

	int* a = new int[0];
	cout << (a == NULL) << endl;
	delete [] a;

	vector<int> a;
	int* b = new int[3];
	b[0] = 1;
	b[1] = 2;
	b[2] = 3;
	a.push_back(0);
	int size = a.size();
	//a.resize(size + 3);
	a.insert(a.end(), b, b+3);
	cout << a[0] << a[1] << a[2] << endl;
	b[0] = 0;
	cout << a[0] << a[1] << a[2] << endl;
	delete [] b;
	cout << a.size() << endl;
	for (int n : a) cout << n << endl;

	int a = 5;
	int b[4 * a] = {1,2,3,4,5};
	cout << b[0] << endl;


	vector<int> a = {0,1,2,3,4};
	int b[3];
	copy(a.begin() + 2, a.begin() + 5, b);
	cout << b[0] << b[1] << b[2] << endl;

	int* a = new int[5];
	for (int i = 0; i < 5; i++) a[i] = i;
	f(a);
	cout << a[0] << a[1] << a[2] << a[3] << a[4] << endl;
	delete [] a;

	int a = 5;
	int b = a / 2 - 1;
	cout << b;

	vector<int> a;
	for (int i = 0; i < 5; i++) a.push_back(i);
	f(&(a[0]));
	for (int i = 0; i < 5; i++) cout << a[i];

	unsigned int a = 4294967295;
	cout << a << endl;
	a++;
	cout << a << endl;

	vector<int> a = {0,1,2};
	vector<int> b = {3,4,5};
	b.resize(6);
	copy(a.begin(), a.begin()+3, b.end() - 4);
	for (int i : b) cout << i;

	int* a = new int[1];
	a[0] = 0;
	cout << a[0] << endl;

	unsigned long a = 4026597135;
	uint8_t x = a >> 24;
	uint8_t y = a >> 16;
	uint8_t z = a >> 8;
	uint8_t t = a;

	cout << x << " " << y << " " << z << " " << t << endl;*

	cout << sizeof(int) << endl;

	int a[3][2] = { {1,2}, {3,4}, {4,5} };
	for (int i = 0; i < 3; i++) {

		for (int j = 0; j < 2; j++) cout << &a[i][j] << endl;

	}

	for (int i = 0; i < 6; i++) cout << (*(a + i / 2) + i % 2) << endl;*


	string path = "C:\\Users\\Anýl\\Desktop\\original.png";
	PNG_RW p;
	p.read(path);
	p.write("C:\\Users\\Anýl\\Desktop\\riginal.png");

	Recolor::f();

	ColorRGB rgb(45, 89, 168);
	ColorHSV hsv = rgb.convert();
	cout << 360 * hsv.H << " " << 100 * hsv.S << " " << 100 * hsv.V << endl;

	rgb = hsv.convert();
	cout << rgb.R << " " << rgb.G << " " << rgb.B << endl;

	CLComponents cl;
	cl.init();
	
	PNG_RW png;
	png.read("C:\\Users\\Anýl\\Desktop\\map.png");
		
	cl_uint height = png.getHeight(), width = png.getWidth();
	uint8_t* map = png.getImage();

	vector<Coord> samples;
	vector<Border> borders;

	uint8_t reg = 5;
	samples.resize(reg);

	unsigned int bytes = height * width * 3;
	cl::Buffer buffer_map(cl.context, CL_MEM_READ_WRITE, sizeof(uint8_t) * bytes / 3);
	cl.queue.enqueueWriteBuffer(buffer_map, CL_TRUE, 0, sizeof(uint8_t) * bytes / 3, map);

	cl::Buffer buffer_samples(cl.context, CL_MEM_WRITE_ONLY, sizeof(cl_uint2) * reg);
	cl::Buffer buffer_preborders(cl.context, CL_MEM_WRITE_ONLY, sizeof(cl_uint3) * height * width);
	cl::Buffer buffer_numborders(cl.context, CL_MEM_READ_WRITE, sizeof(cl_uint));
	cl::Buffer buffer_antialias(cl.context, CL_MEM_READ_WRITE, sizeof(cl_uchar));

	cl_uint numborders = 0;
	cl_uchar antialias = 1;
	cl.queue.enqueueWriteBuffer(buffer_numborders, CL_TRUE, 0, sizeof(cl_uint), &numborders);
	cl.queue.enqueueWriteBuffer(buffer_antialias, CL_TRUE, 0, sizeof(cl_uchar), &antialias);

	pair<const char*, size_t> p[] = { { PARSE_CODE, PARSE_LEN } };
	cl.build(p, 1);

	cl::Kernel kernel_parse(cl.program, "parse");
	kernel_parse.setArg(0, buffer_map);
	kernel_parse.setArg(1, buffer_samples);
	kernel_parse.setArg(2, buffer_preborders);
	kernel_parse.setArg(3, buffer_numborders);
	kernel_parse.setArg(4, buffer_antialias);
	cl.queue.enqueueNDRangeKernel(kernel_parse, cl::NullRange, cl::NDRange(height, width), cl::NullRange);
	cl.queue.finish();

	cl.queue.enqueueReadBuffer(buffer_samples, CL_TRUE, 0, sizeof(cl_uint2) * reg, &samples[0]);
	cl.queue.enqueueReadBuffer(buffer_numborders, CL_TRUE, 0, sizeof(cl_uint), &numborders);
	borders.resize(numborders);
	if (numborders != 0) cl.queue.enqueueReadBuffer(buffer_preborders, CL_TRUE, 0, sizeof(cl_uint3) * numborders, &borders[0]);

	pair<const char*, size_t> c[] = { { EXAMPLE_CODE, EXAMPLE_LEN } };
	cl.build(c, 1);

	cl::Kernel kernel_example(cl.program, "example");
	kernel_example.setArg(0, buffer_map);
	kernel_example.setArg(1, buffer_samples);
	cl.queue.enqueueNDRangeKernel(kernel_example, cl::NullRange, cl::NDRange(height, width), cl::NullRange);
	cl.queue.finish();

	cl.queue.enqueueReadBuffer(buffer_map, CL_TRUE, 0, sizeof(uint8_t) * height * width, map);
	cl.queue.enqueueReadBuffer(buffer_samples, CL_TRUE, 0, sizeof(cl_uint2) * reg, &samples[0]);*
	*/

	CLComponents cl;
	cl.init();
	Recolor r(cl);

	vector<ColorRGB> colors;
	srand(time(NULL));
	colors.push_back(ColorRGB(RAND, RAND, RAND));
	colors.push_back(ColorRGB(RAND, RAND, RAND));
	colors.push_back(ColorRGB(RAND, RAND, RAND));
	colors.push_back(ColorRGB(RAND, RAND, RAND));
	colors.push_back(ColorRGB(RAND, RAND, RAND));

	r.setColors(colors);
	r.recolor("C:\\Users\\Anýl\\Desktop\\original.png", "C:\\Users\\Anýl\\Desktop\\map.png", "C:\\Users\\Anýl\\Desktop\\final.png");

	return 0;
}
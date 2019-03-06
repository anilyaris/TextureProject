#include <cstdint>
#include <string>
#include <vector>
#include <array>
#include <fstream>
#include <algorithm>
#include <utility>
#include "CRC.hpp"
#include "PNG.hpp"
#include "Deflate.hpp"
#include "util.hpp"
#include "lodepng.h"

using std::move;
using std::copy;
using std::equal;
using std::ifstream;
using std::ofstream;

//Constructs the chunk from a pointer within a char buffer.
Chunk::Chunk(char* start) : type((start + 4), 4) {
	
	length = bytesTo32((uint8_t*) start);
	
	if (length == 0) data = NULL;
	else data = new uint8_t[length];	
	for (int i = 0; i < length; i++) data[i] = start[8 + i];
	
	crc = bytesTo32((uint8_t*) (start + 8 + length));

	fromFile = true;
	
}

//Constructs the chunk from given values.
Chunk::Chunk(string typ, uint32_t len, uint8_t* dat, uint32_t CRC) {

	type = typ;
	length = len;
	data = dat;
	crc = CRC;
	fromFile = false;

}

Chunk::Chunk(Chunk&& chunk) {
	
	length = move(chunk.length);
		
	type = move(chunk.type);
	
	//if (data) delete [] data;
	data = chunk.data;
	chunk.data = NULL;
	
	crc = move(chunk.crc);
	fromFile = move(chunk.fromFile);
	
}

Chunk::~Chunk() {
	
	if (data && fromFile) delete [] data;
	
}

string Chunk::getType() const {
	
	return type;
	
}

uint32_t Chunk::getLength() const {
	
	return length;
	
}

uint8_t* Chunk::getData() const {

	return data;

}

uint32_t Chunk::getCRC() const {

	return crc;

}

//Inserts the contents of the chunk to a string buffer.
void Chunk::toBytes(string& buf) const {
	
	char chars[4];
	
	_32ToBytes(length, (uint8_t*)chars);
	buf.insert(buf.length(), chars, 4);

	buf += type;
	buf.insert(buf.length(), (char*) data, length);
	
	_32ToBytes(crc, (uint8_t*)chars);
	buf.insert(buf.length(), chars, 4);
	
}

PNG_RW::PNG_RW() : image(NULL), chunks(), deflate() {};

//Reads a PNG file from the given path.
void PNG_RW::read(string fullPath) {	

	try {

		fileName = fullPath;
		char* buffer = acquire(fullPath);
		int fileSize = width;

		vector<uint8_t> zlib;
		parse(buffer, fileSize, zlib);
		
		delete[] buffer;
		zlibParse(zlib);

		vector<uint8_t> decompressed;
		deflate.decompress(zlib, decompressed);
		reconstruct(decompressed);		

	}
	catch (string s) {

		throw;

	}

}

//Writes a PNG file to the given path.
void PNG_RW::write(string fullPath) {

	/*try {

		vector<uint8_t> filtered;
		filter(filtered);

		vector<uint8_t> zlib;
		deflate.compress(zlib, filtered);

		output(zlib, fullPath);		

	}
	catch (string s) {

		throw;

	}*/

	LodePNGColorType colorType = LCT_RGBA;
	if (color == 3) colorType = LCT_RGB;
	lodepng::encode(fullPath, image, width, height, colorType);

}

//Gets the content of the input PNG file as a buffer.
char* PNG_RW::acquire(string fullPath) {

	ifstream file(fullPath, ifstream::binary);
	if (file) {

		file.seekg(0, file.end);
		int len = file.tellg();
		file.seekg(0, file.beg);

		char* buffer = new char[len];
		file.read(buffer, len);
		file.close();

		width = len;
		return buffer;

	}

	else throw fullPath + "File not found.";

}

//Writes the output PNG buffer to the file.
void PNG_RW::output(vector<uint8_t>& zlib, string& fullPath) const {

	ofstream file(fullPath, ofstream::binary);

	string content;
	char signature[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
	content.insert(0, signature, 8);

	uint8_t chars[4];

	for (int i = 0; i < chunks.size(); i++) {

		string type = chunks[i].getType();
		if (type == "IEND") Chunk("IDAT", zlib.size(), &zlib[0], CRC().calculate("IDAT", zlib)).toBytes(content);
		chunks[i].toBytes(content);

	}

	file << content;
	file.close();

}

//Clears the image buffer.
void PNG_RW::clearImage() {

	if (image) delete[] image;

}

//Constructs an empty buffer for the image to be read.
void PNG_RW::constructImage() {

	image = new uint8_t[height * width * color];	

}

PNG_RW::~PNG_RW() {
	
	clearImage();
	
}

string PNG_RW::getName() const {

	return fileName;

}

uint8_t* PNG_RW::getImage() const {
	
	return image;
	
}

uint32_t PNG_RW::getWidth() const {
	
	return width;
	
}

uint32_t PNG_RW::getHeight() const {
	
	return height;
	
}

uint8_t PNG_RW::getColor() const {
	
	return color;
	
}

//Gets PNG information from the input buffer.
void PNG_RW::parse(char*& buffer, int fileSize, vector<uint8_t>& zlib) {
	
	bool IHDR_flag = false, IEND_flag = false;
	uint8_t IDAT_state = 0;

	chunks.clear();
	
	char signature[8] = {137, 80, 78, 71, 13, 10, 26, 10};
	if (!equal(signature, signature + 8, buffer)) throw fileName + "-> Invalid PNG file: Signature not found.";
		
	int pos = 8;
	
	do {
		
		Chunk chunk(buffer + pos);
		string chunkType = chunk.getType();
		uint8_t* chunkData = chunk.getData();
		uint32_t chunkLength = chunk.getLength();
		
		if (!IHDR_flag) IHDR_flag = (chunkType == "IHDR");
		if (!IHDR_flag) throw fileName + "-> Invalid PNG file: Data found before header.";
		
		if (chunkType == "IHDR") {
			
			if (chunkLength != 13) throw fileName + "-> Invalid PNG file: Header has wrong format.";
			
			width = bytesTo32(chunkData);
			height = bytesTo32(chunkData + 4);
			depth = chunkData[8];
			color = chunkData[9];
			comp = chunkData[10];
			filt = chunkData[11];
			ilace = chunkData[12];
			
			if (color != 0 && color != 2 /*&& color != 3 && color != 4*/ && color != 6) throw fileName + "-> Invalid PNG file: Color type not allowed or supported.";
			if (depth != 8) throw fileName + "-> Invalid PNG file: Bit depth not allowed or supported.";
			/*else if (color == 3) {				
				if (depth != 1 && depth != 2 && depth != 4 && depth != 8) throw fileName + "-> Invalid PNG file: Bit depth not allowed for color type.";								
			}			
			else if (color == 4) {				
				if (depth != 8 && depth != 16) throw fileName + "-> Invalid PNG file: Bit depth not allowed for color type.";			
			}		
			else if (color == 6) {				
				if (depth != 8 && depth != 16) throw fileName + "-> Invalid PNG file: Bit depth not allowed for color type.";								
			}*/			
			if (comp != 0 || filt != 0 || ilace != 0) throw fileName + "-> Invalid PNG file: Invalid compression/filtering/interlace method.";

			color = (10 * color + 8 - color * color) / 8;
			
		}
		
		else if (chunkType == "IDAT") {
			
			if (IDAT_state == 0) IDAT_state++;
			else if (IDAT_state == 2) throw fileName + "-> Invalid PNG file: Data is not consecutive";
			
			zlib.insert(zlib.end(), chunkData, chunkData + chunkLength);
			
		}
			
		else if (chunkType == "IEND") {
			
			IEND_flag = true;
			if (chunkLength != 0) throw fileName + "-> Invalid PNG file: Footer has wrong format.";
			
		}
		
		else if (IDAT_state == 1) IDAT_state++;
		
		if (chunkType != "IDAT" && chunkType[0] < 'a') chunks.push_back(move(chunk));
		pos += 12 + chunkLength;
		
	} while (pos < fileSize && !IEND_flag);
	
	if (!IHDR_flag) throw fileName + "-> Invalid PNG file: No header found.";
	if (!IEND_flag) throw fileName + "-> Invalid PNG file: No footer found.";
	if (IDAT_state == 0) throw fileName + "-> Invalid PNG file: No data found.";
	if (pos != fileSize) throw fileName + "-> Invalid PNG file: Data found after footer.";

}

//Performs filtering on the output PNG buffer.
void PNG_RW::filter(vector<uint8_t>& data) const {

	if (image == NULL) throw fileName + "-> No image to write.";

	for (int i = 0; i < height; i++) {

		data.push_back(filt);

		for (int j = 0; j < width; j++) {

			for (int k = 0; k < color; k++) {

				unsigned int index = color * (width * i + j) + k;

				uint8_t x = image[index];
				uint8_t a = j == 0 ? 0 : image[index - color];
				uint8_t b = i == 0 ? 0 : image[index - width * color];
				uint8_t c = (i == 0 || j == 0) ? 0 : image[index - (width + 1) * color];

				switch (filt) {

					case 0: 
						data.push_back(x);
						break;

					case 1:
						data.push_back(x - a);
						break;

					case 2:
						data.push_back(x - b);
						break;

					case 3:
						data.push_back(x - ((uint16_t)a + (uint16_t)b) / 2);
						break;

					case 4:
						data.push_back((int)x - paeth(a, b, c));
						break;
					
					default:
						throw fileName + "Invalid PNG file: Invalid filter type.";

				}				

			}

		}

	}

}

//Performs defiltering on the input PNG buffer.
void PNG_RW::reconstruct(vector<uint8_t>& data) {

	clearImage();
	constructImage();

	unsigned int pos = 0;
	for (unsigned int i = 0; i < height; i++) {

		uint8_t filterType = data[pos++];
		for (unsigned int j = 0; j < width; j++) {

			for (unsigned int k = 0; k < color; k++) {

				unsigned int index = color * (width * i + j) + k;

				uint8_t x = data[pos++];
				uint8_t a = j == 0 ? 0 : image[index - color];
				uint8_t b = i == 0 ? 0 : image[index - width * color];
				uint8_t c = (i == 0 || j == 0) ? 0 : image[index - (width + 1) * color];

				switch (filterType) {

				case 0:
					image[index] = x;
					break;

				case 1:
					image[index] = x + a;
					break;

				case 2:
					image[index] = x + b;
					break;

				case 3:
					image[index] = x + ((uint16_t)a + (uint16_t)b) / 2;
					break;

				case 4:
					image[index] = (int)x + paeth(a, b, c);
					break;

				default:
					throw fileName + "Invalid PNG file: Invalid filter type.";

				}

			}

		}

	}		

	data.clear();

}
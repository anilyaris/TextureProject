#include "DDS.hpp"
#include "util.hpp"
#include <cstdint>
#include <cstring>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
#include <fstream>

using std::ifstream;
using std::ofstream;
using std::map;
using std::move;
using std::binary_search;

/*const DDS_RW::BNTXHeader& DDS_RW::getBNTXHeader() const {

	return bntxHeader;

}

const DDS_RW::TexContainer& DDS_RW::getTexContainer() const {

	return texContainer;

}

const DDS_RW::BlockHeader& DDS_RW::getBlockHeader() const {

	return blockHeader;

}

const DDS_RW::TextureInfo& DDS_RW::getTextureInfo() const {

	return textureInfo;

}*/

uint64_t DDS_RW::readN(unsigned char*& buf, int n, unsigned char bom) {

	uint64_t result = 0;
	if (n <= 8) {

		bool little = bom == '<';
		int _n = n;
		unsigned int mult = 1;
		while (n > 0) {

			result += *(buf + (little ? 0 : (n - 1))) * mult;
			if (little) buf++;
			mult = mult << 8;
			n--;

		}
		if (!little && _n > 0) buf += _n;

	}
	return result;

}

uint8_t DDS_RW::blkWidth(uint8_t dim) {

	if (dim < 0x2e) return 4;
	else if (dim < 0x30) return 5;
	else if (dim < 0x32) return 6;
	else if (dim < 0x35) return 8;
	else if (dim < 0x39) return 10;
	return 12;

}
uint8_t DDS_RW::blkHeight(uint8_t dim) {

	if (dim < 0x2f) return 4;
	else if (dim < 0x31 || dim == 0x32 || dim == 0x35) return 5;
	else if (dim < 0x34 || dim == 0x36) return 6;
	else if (dim < 0x38) return 8;
	else if (dim < 0x3a) return 10;
	return 12;

}
int DDS_RW::getAddrBlockLinear(int x, int y, int image_width, int bytes_per_pixel, int base_address, int blockHeight) {

	int image_width_in_gobs = DIV_ROUND_UP(image_width * bytes_per_pixel, 64);

	int GOB_address = (base_address
		+ (y / (8 * blockHeight)) * 512 * blockHeight * image_width_in_gobs
		+ (x * bytes_per_pixel / 64) * 512 * blockHeight
		+ (y % (8 * blockHeight) / 8) * 512);

	x *= bytes_per_pixel;

	int Address = (GOB_address + ((x % 64) / 32) * 256 + ((y % 8) / 2) * 64
		+ ((x % 32) / 16) * 32 + (y % 2) * 16 + (x % 16));

	return Address;
}

void DDS_RW::swizzle(unsigned int width, unsigned int height, unsigned int blkWidth, unsigned int blkHeight,
	bool roundPitch, uint8_t bpp, int tileMode, uint8_t blockHeightLog2, unsigned char* data, bool toSwizzle, vector<unsigned char>& result, int offset) {

	if (blockHeightLog2 > 5) throw "Invalid blockHeightLog2";
	int blockHeight = 1 << blockHeightLog2;

	width = DIV_ROUND_UP(width, blkWidth);
	height = DIV_ROUND_UP(height, blkHeight);

	unsigned int pitch, surfSize;
	if (tileMode == 1) {
		pitch = width * bpp;

		if (roundPitch) pitch = round_up(pitch, 32);

		surfSize = pitch * height;

	}
	else {
		pitch = round_up(width * bpp, 64);
		surfSize = pitch * round_up(height, blockHeight * 8);
	}

	result.resize(offset + surfSize);

	for (unsigned int y = 0; y < height; y++) {

		for (unsigned int x = 0; x < width; x++) {

			unsigned int pos = tileMode == 1 ? y * pitch + x * bpp : getAddrBlockLinear(x, y, width, bpp, 0, blockHeight);
			int pos_ = (y * width + x) * bpp;

			if (pos + bpp <= surfSize) {

				for (uint8_t z = 0; z < bpp; z++) {

					if (toSwizzle) result[pos + z + offset] = data[pos_ + z];
					else result[pos_ + z + offset] = data[pos + z];

				}

			}

		}

	}

}

size_t DDS_RW::generateHeader(unsigned int num_mipmaps, int w, int h, string format_, vector<uint8_t>& compSel, size_t size, bool compressed, unsigned char*& hdr) {

	size_t len = compSel.size();
	size_t ret = 128;

	bool luminance = false;
	bool RGB = false;

	map<uint8_t, uint32_t> compSels;
	uint8_t fmtbpp = 0;
	unsigned char fourcc[4];

	bool has_alpha = true;

	if (format_ == "rgba8") {  // ABGR8
		RGB = true;
		compSels = { {2, 0x000000ff}, {3, 0x0000ff00}, {4, 0x00ff0000}, {5, 0xff000000}, {1, 0} };
		fmtbpp = 4;
	}
	else if (format_ == "bgra8") { // ARGB8
		RGB = true;
		compSels = { {2, 0x00ff0000}, {3, 0x0000ff00}, {4, 0x000000ff}, {5, 0xff000000}, {1, 0} };
		fmtbpp = 4;
	}
	else if (format_ == "bgr10a2") { // A2RGB10
		RGB = true;
		compSels = { {2, 0x3ff00000}, {3, 0x000ffc00}, {4, 0x000003ff}, {5, 0xc0000000}, {1, 0} };
		fmtbpp = 4;
	}
	else if (format_ == "rgb565") { // BGR565
		RGB = true;
		compSels = { {2, 0x0000001f}, {3, 0x000007e0}, {4, 0x0000f800}, {5, 0}, {1, 0} };
		fmtbpp = 2;
		has_alpha = false;
	}
	else if (format_ == "bgr565") { // RGB565
		RGB = true;
		compSels = { {2, 0x0000f800}, {3, 0x000007e0}, {4, 0x0000001f}, {5, 0}, {1, 0} };
		fmtbpp = 2;
		has_alpha = false;
	}
	else if (format_ == "rgb5a1") { // A1BGR5
		RGB = true;
		compSels = { {2, 0x0000001f}, {3, 0x000003e0}, {4, 0x00007c00}, {5, 0x00008000}, {1, 0} };
		fmtbpp = 2;
	}
	else if (format_ == "bgr5a1") { // A1RGB5
		RGB = true;
		compSels = { {2, 0x00007c00}, {3, 0x000003e0}, {4, 0x0000001f}, {5, 0x00008000}, {1, 0} };
		fmtbpp = 2;
	}
	else if (format_ == "a1bgr5") { // RGB5A1
		RGB = true;
		compSels = { {2, 0x00008000}, {3, 0x00007c00}, {4, 0x000003e0}, {5, 0x0000001f}, {1, 0} };
		fmtbpp = 2;
	}
	else if (format_ == "rgba4") { // ABGR4
		RGB = true;
		compSels = { {2, 0x0000000f}, {3, 0x000000f0}, {4, 0x00000f00}, {5, 0x0000f000}, {1, 0} };
		fmtbpp = 2;
	}
	else if (format_ == "abgr4") { // RGBA4
		RGB = true;
		compSels = { {2, 0x0000f000}, {3, 0x00000f00}, {4, 0x000000f0}, {5, 0x0000000f}, {1, 0} };
		fmtbpp = 2;
	}
	else if (format_ == "l8") { // L8
		luminance = true;
		compSels = { {2, 0x000000ff}, {3, 0}, {4, 0}, {5, 0}, {1, 0} };
		fmtbpp = 1;

		if (compSel[len - 4] != 2) has_alpha = false;
	}
	else if (format_ == "la8") { // A8L8
		luminance = true;
		compSels = { {2, 0x000000ff}, {3, 0x0000ff00}, {4, 0}, {5, 0}, {1, 0} };
		fmtbpp = 2;
	}
	else if (format_ == "la4") { // A4L4
		luminance = true;
		compSels = { {2, 0x0000000f}, {3, 0x000000f0}, {4, 0}, {5, 0}, {1, 0} };
		fmtbpp = 1;
	}

	int32_t flags = 0x00000001 | 0x00001000 | 0x00000004 | 0x00000002;
	int32_t pflags;

	int caps = 0x00001000;

	if (num_mipmaps == 0) num_mipmaps = 1;
	else if (num_mipmaps != 1) {
		flags |= 0x00020000;
		caps |= 0x00000008 | 0x00400000;
	}

	if (!compressed) {

		flags |= 0x00000008;

		bool a = false;

		if (compSel[len - 1] != 2 && compSel[len - 2] != 2 && compSel[len - 3] != 2 && compSel[len - 4] == 2) { // ALPHA
			a = true;
			pflags = 0x00000002;
		}
		else if (luminance)  pflags = 0x00020000;
		else if (RGB) flags = 0x00000040;
		else return 0;

		if (has_alpha && !a) pflags |= 0x00000001;
		size = w * fmtbpp;

	}

	else {

		flags |= 0x00080000;
		pflags = 0x00000004;

		vector<string> dx10_formats = { "BC4U", "BC4S", "BC5U", "BC5S", "BC6H_UF16", "BC6H_SF16", "BC7", "BC7S", "BC7U" };
		bool format_in = false;
		for (string& fmt : dx10_formats) {
			if (format_ == fmt) {
				format_in = true;
				break;
			}
		}

		string fourcc_;
		if (format_ == "BC1") fourcc_ = "DXT1";
		else if (format_ == "BC2") fourcc_ = "DXT3";
		else if (format_ == "BC3") fourcc_ = "DXT5";
		else if (format_in)	fourcc_ = "DX10";
		else throw "Invalid format!";
		for (int i = 0; i < 4; i++) fourcc[i] = fourcc_[i];

	}

	hdr[0] = hdr[1] = 'D'; hdr[2] = 'S'; hdr[3] = ' '; 
	hdr[4] = 124; hdr[5] = hdr[6] = hdr[7] = 0;
	L32ToBytes(flags, (uint8_t*)hdr + 8);
	L32ToBytes(h, (uint8_t*)hdr + 12);
	L32ToBytes(w, (uint8_t*)hdr + 16);
	L32ToBytes(size, (uint8_t*)hdr + 20);
	L32ToBytes(num_mipmaps, (uint8_t*)hdr + 28);
	hdr[76] = 32; hdr[77] = hdr[78] = hdr[79] = 0;
	L32ToBytes(pflags, (uint8_t*)hdr + 80);
	if (compressed) {
		hdr[84] = fourcc[0]; hdr[85] = fourcc[1]; hdr[86] = fourcc[2]; hdr[87] = fourcc[3];
	}
	else {
		L32ToBytes(fmtbpp << 3, (uint8_t*)hdr + 88);
		try {
			L32ToBytes(compSels.at(compSel[len - 1]), (uint8_t*)hdr + 92);
		}
		catch (...) {
			L32ToBytes(compSels[2], (uint8_t*)hdr + 92);
		}
		try {
			L32ToBytes(compSels.at(compSel[len - 2]), (uint8_t*)hdr + 96);
		}
		catch (...) {
			L32ToBytes(compSels[3], (uint8_t*)hdr + 96);
		}
		try {
			L32ToBytes(compSels.at(compSel[len - 3]), (uint8_t*)hdr + 100);
		}
		catch (...) {
			L32ToBytes(compSels[4], (uint8_t*)hdr + 100);
		}
		try {
			L32ToBytes(compSels.at(compSel[len - 4]), (uint8_t*)hdr + 104);
		}
		catch (...) {
			L32ToBytes(compSels[5], (uint8_t*)hdr + 104);
		}

	}

	L32ToBytes(caps, (uint8_t*)hdr + 108);			
			
	unsigned char* add = NULL;
	if (format_ == "BC4U") {
		add = new unsigned char[20]();
		add[0] = 0x50;
		add[4] = 0x03;
		add[12] = 0x01;
	}
	else if (format_ == "BC4S") {
		add = new unsigned char[20]();
		add[0] = 0x51;
		add[4] = 0x03;
		add[12] = 0x01;
	}
	else if (format_ == "BC5U") {
		add = new unsigned char[20]();
		add[0] = 0x53;
		add[4] = 0x03;
		add[12] = 0x01;
	}
	else if (format_ == "BC5S") {
		add = new unsigned char[20]();
		add[0] = 0x54;
		add[4] = 0x03;
		add[12] = 0x01;
	}
	else if (format_ == "BC6H_UF16") {
		add = new unsigned char[20]();
		add[0] = 0x5F;
		add[4] = 0x03;
		add[12] = 0x01;
	}
	else if (format_ == "BC6H_SF16") {
		add = new unsigned char[20]();
		add[0] = 0x60;
		add[4] = 0x03;
		add[12] = 0x01;
	}
	else if (format_ == "BC7" || format_ == "BC7U") {
		add = new unsigned char[20]();
		add[0] = 0x62;
		add[4] = 0x03;
		add[12] = 0x01;
	}
	else if (format_ == "BC7S") {
		add = new unsigned char[20]();
		add[0] = 0x63;
		add[4] = 0x03;
		add[12] = 0x01;
	}
	if (add) {

		unsigned char* new_hdr = new unsigned char[148]();
		memcpy(new_hdr, hdr, 128);
		memcpy(new_hdr + 128, add, 20);
		delete[] add;
		delete[] hdr;
		hdr = new_hdr;
		ret = 148;

	}		
	
	return ret;

}

void DDS_RW::BNTXHeader::init(unsigned char* data, unsigned int pos, unsigned char bom_) {

	data += pos;
	for (int i = 0; i < 8; i++) magic[i] = *(data++);
	version = readN(data, 4, bom_);
	bom = readN(data, 2, bom_);
	alignmentShift = *(data++);
	targetAddrSize = *(data++);
	fileNameAddr = readN(data, 4, bom_);
	flag = readN(data, 2, bom_);
	firstBlkAddr = readN(data, 2, bom_);
	relocAddr = readN(data, 4, bom_);
	fileSize = readN(data, 4, bom_);

}

void DDS_RW::TexContainer::init(unsigned char* data, unsigned int pos, unsigned char bom) {

	data += pos;
	for (int i = 0; i < 4; i++) target[i] = *(data++);
	count = readN(data, 4, bom);
	infoPtrsAddr = readN(data, 8, bom);
	dataBlkAddr = readN(data, 8, bom);
	dictAddr = readN(data, 8, bom);
	memPoolAddr = readN(data, 8, bom);
	memPoolPtr = readN(data, 8, bom);
	baseMemPoolAddr = readN(data, 4, bom);

}

void DDS_RW::BlockHeader::init(unsigned char* data, unsigned int pos, unsigned char bom) {

	data += pos;
	for (int i = 0; i < 4; i++) magic[i] = *(data++);
	nextBlkAddr = readN(data, 4, bom);
	blockSize = readN(data, 4, bom);

}

void DDS_RW::TextureInfo::init(unsigned char* data, unsigned int pos, unsigned char bom) {

	data += pos;
	flags = *(data++);
	dim = *(data++);
	tileMode = readN(data, 2, bom);
	swizzle = readN(data, 2, bom);
	numMips = readN(data, 2, bom);
	numSamples = readN(data, 2, bom);
	data += 2;
	format_ = readN(data, 4, bom);
	accessFlags = readN(data, 4, bom);
	width = readN(data, 4, bom);
	height = readN(data, 4, bom);
	depth = readN(data, 4, bom);
	arrayLength = readN(data, 4, bom);
	textureLayout = readN(data, 4, bom);
	textureLayout2 = readN(data, 4, bom);
	data += 20;
	imageSize = readN(data, 4, bom);
	alignment = readN(data, 4, bom);
	compSel = readN(data, 4, bom);
	type_ = *(data++);
	data += 3;
	nameAddr = readN(data, 8, bom);
	parentAddr = readN(data, 8, bom);
	ptrsAddr = readN(data, 8, bom);
	userDataAddr = readN(data, 8, bom);
	texPtr = readN(data, 8, bom);
	texViewPtr = readN(data, 8, bom);
	descSlotDataAddr = readN(data, 8, bom);
	userDictAddr = readN(data, 8, bom);

}

DDS_RW::~DDS_RW() {

	if (fileData) delete[] fileData;

}

unsigned char* DDS_RW::getFileData() {

	return fileData;

}

void DDS_RW::setFileData(unsigned char* fileData) {

	this->fileData = fileData;

}

vector<uint32_t>& DDS_RW::getTexSizes() {

	return texSizes;

}

void DDS_RW::setTexSizes(const vector<uint32_t>& texSizes) {

	this->texSizes = texSizes;

}

DDS_RW::BNTXParseResult& DDS_RW::parseBNTX(string fullPath, bool getOnlyNameMode) {

	ifstream file(fullPath, ifstream::binary);
	if (file) {

		bntxPath = fullPath;

		file.seekg(0, file.end);
		bntxLen = file.tellg();
		file.seekg(0, file.beg);

		unsigned char* buffer = new unsigned char[bntxLen];
		file.read((char*)buffer, bntxLen);
		file.close();

		int64_t pos = 0;

		unsigned char bom;
		if (buffer[0xc] == (unsigned char)0xff && buffer[0xd] == (unsigned char)0xfe) bom = '<';
		else if (buffer[0xc] == (unsigned char)0xfe && buffer[0xd] == (unsigned char)0xff) bom = '>';
		else throw "Invalid BOM!";

		BNTXHeader bntxHeader;
		bntxHeader.init(buffer, pos, bom);
		pos += bntxHeader.size;

		if (memcmp(bntxHeader.magic, "BNTX\0\0\0\0", 8) != 0) throw "Invalid file header!";

		unsigned char* buf = buffer + (bntxHeader.fileNameAddr - 2);
		uint16_t fnameLen = readN(buf, 2, bom);
		string fname((char*)buffer + bntxHeader.fileNameAddr, fnameLen);

		if (getOnlyNameMode) {

			bntxParse.fname = fname;
			return bntxParse;

		}

		TexContainer texContainer;
		texContainer.init(buffer, pos, bom);
		pos += texContainer.size;

		bool target;
		if (memcmp(texContainer.target, "Gen ", 4) == 0) target = false;
		else if (memcmp(texContainer.target, "NX  ", 4) == 0) target = true;
		else throw "Unsupported target platform!";

		vector<TexInfo>& textures = bntxParse.textures;
		vector<string>& texNames = bntxParse.texNames;
		vector<uint32_t> texSizes;

		uint32_t count = texContainer.count;
		for (uint32_t i = 0; i < count; i++) {

			buf = buffer + (texContainer.infoPtrsAddr + 8 * i);
			pos = readN(buf, 8, bom);

			BlockHeader blockHeader;
			blockHeader.init(buffer, pos, bom);
			pos += blockHeader.size;

			TextureInfo textureInfo;
			textureInfo.init(buffer, pos, bom);

			if (memcmp(blockHeader.magic, "BRTI", 4) != 0) continue;

			buf = buffer + textureInfo.nameAddr;
			uint16_t nameLen = readN(buf, 2, bom);
			string name((char*)buffer + textureInfo.nameAddr + 2, nameLen);

			vector<uint8_t> compSel, compSel2;

			for (uint8_t j = 0; j < 4; j++) {

				uint8_t value = (textureInfo.compSel >> (8 * (3 - j))) & 0xff;
				compSel2.push_back(value);
				if (value == 0) value = 5 - compSel.size();
				compSel.push_back(value);

			}

			buf = buffer + textureInfo.ptrsAddr;
			int64_t dataAddr = readN(buf, 8, bom);
			vector<int64_t> mipOffsets;
			mipOffsets.push_back(0);

			for (uint16_t j = 1; j < textureInfo.numMips; j++) {

				buf = buffer + textureInfo.ptrsAddr + 8 * j;
				int64_t mipOffset = readN(buf, 8, bom);
				mipOffsets.push_back(mipOffset - dataAddr);

			}

			textures.emplace_back();
			TexInfo& tex = textures.back();

			tex.infoAddr = pos;
			tex.info = textureInfo;
			tex.bom = bom;
			tex.target = target;

			tex.name = name;

			tex.readTexLayout = textureInfo.flags & 1;
			tex.sparseBinding = textureInfo.flags >> 1;
			tex.sparseResidency = textureInfo.flags >> 2;
			tex.dim = textureInfo.dim;
			tex.tileMode = textureInfo.tileMode;
			tex.numMips = textureInfo.numMips;
			tex.width = textureInfo.width;
			tex.height = textureInfo.height;
			tex.format = textureInfo.format_;
			tex.arrayLength = textureInfo.arrayLength;
			tex.blockHeightLog2 = textureInfo.textureLayout & 7;
			tex.imageSize = textureInfo.imageSize;

			tex.compSel = compSel;
			tex.compSel2 = compSel2;

			tex.alignment = textureInfo.alignment;
			tex.type = textureInfo.type_;

			tex.mipOffsets = mipOffsets;
			tex.dataAddr = dataAddr;

			buf = buffer + dataAddr;
			tex.data = vector<unsigned char>(buf, buf + textureInfo.imageSize);
			texNames.push_back(name);
			texSizes.push_back(textureInfo.imageSize);

		}

		if (fileData) delete[] fileData;
		fileData = buffer;
		this->texSizes = move(texSizes);

		bntxParse.fname = fname;
		for (uint8_t j = 0; j < 4; j++) bntxParse.target[j] = texContainer.target[j];

		return bntxParse;

	}


	else throw fullPath + "File not found.";

}

void DDS_RW::decode(TexInfo& tex) {

	uint8_t format = tex.format >> 8;
	bool dimCnd = binary_search(blk_dims.begin(), blk_dims.end(), format);

	bntxDecode.blkWidth = dimCnd ? DDS_RW::blkWidth(format) : 1;
	bntxDecode.blkHeight = dimCnd ? DDS_RW::blkHeight(format) : 1;
	
	uint8_t bpp = bntxDecode.bpp = bpps.at(format);

	uint16_t linesPerBlockHeight = (1 << tex.blockHeightLog2) * 8;
	unsigned int blockHeightShift = 0;

	size_t numOffsets = tex.mipOffsets.size();
	for (int64_t mipLevel = 0; mipLevel < numOffsets; mipLevel++) {

		uint32_t width = tex.width >> mipLevel;
		uint32_t height = tex.height >> mipLevel;
		if (width < 1) width = 1;
		if (height < 1) height = 1;

		int size = DIV_ROUND_UP(width, bntxDecode.blkWidth) * DIV_ROUND_UP(height, bntxDecode.blkHeight) * bpp;
		
		if (pow2_round_up(DIV_ROUND_UP(height, bntxDecode.blkHeight)) < linesPerBlockHeight) blockHeightShift += 1;

		int64_t mipOffset = tex.mipOffsets[mipLevel];

		vector<unsigned char> result_;
		int log = tex.blockHeightLog2;
		log -= blockHeightShift;
		if (log < 0) log = 0;
		swizzle(width, height, bntxDecode.blkWidth, bntxDecode.blkHeight, tex.target, bpp, tex.tileMode,
			log, tex.data.data() + mipOffset, false, result_);
		bntxDecode.result.push_back(vector<unsigned char>(result_.begin(), result_.begin() + size));
	}

}


void DDS_RW::obtainDDS(TexInfo& tex, vector<unsigned char>& data) {

	bool formatCnd = binary_search(formats.begin(), formats.end(), tex.format);
	if (formatCnd && tex.dim == 2 && tex.arrayLength < 2 && tex.tileMode < 2) {

		string format_;
		uint32_t texformat = tex.format;
		uint32_t texformat8 = texformat >> 8;

		if (texformat == 0x101)
			format_ = "la4";

		else if (texformat == 0x201)
			format_ = "l8";
		else if (texformat == 0x301)
			format_ = "rgba4";

		else if (texformat == 0x401)
			format_ = "abgr4";

		else if (texformat == 0x501)
			format_ = "rgb5a1";

		else if (texformat == 0x601)
			format_ = "a1bgr5";

		else if (texformat == 0x701)
			format_ = "rgb565";

		else if (texformat == 0x801)
			format_ = "bgr565";

		else if (texformat == 0x901)
			format_ = "la8";

		else if ((texformat8) == 0xb)
			format_ = "rgba8";

		else if ((texformat8) == 0xc)
			format_ = "bgra8";

		else if (texformat == 0xe01)
			format_ = "bgr10a2";

		else if ((texformat8) == 0x1a)
			format_ = "BC1";

		else if ((texformat8) == 0x1b)
			format_ = "BC2";

		else if ((texformat8) == 0x1c)
			format_ = "BC3";

		else if (texformat == 0x1d01)
			format_ = "BC4U";

		else if (texformat == 0x1d02)
			format_ = "BC4S";

		else if (texformat == 0x1e01)
			format_ = "BC5U";

		else if (texformat == 0x1e02)
			format_ = "BC5S";

		else if (texformat == 0x1f05)
			format_ = "BC6H_SF16";

		else if (texformat == 0x1f0a)
			format_ = "BC6H_UF16";
		else if (texformat8 == 0x20) {
			format_ = "BC7";
		}
		else if (texformat == 0x2001)
			format_ = "BC7U";

		else if (texformat == 0x2006)
			format_ = "BC7S";

		else if (texformat == 0x3b01)
			format_ = "bgr5a1";

		decode(tex);

		unsigned char* hdr = new unsigned char[128]();
		hdr_size = generateHeader(tex.numMips, tex.width, tex.height, format_, tex.compSel,
			bntxDecode.result[0].size(), (texformat8 >= 0x1a && texformat8 <= 0x20), hdr);
		data = vector<unsigned char>(hdr, hdr + hdr_size);
		for (auto& v : bntxDecode.result) data.insert(data.end(), v.begin(), v.end());
		delete[] hdr;

	}
	

}

void DDS_RW::injectDDS(unsigned char* buffer) {

	TexInfo& tex = bntxParse.textures[0];
	if (tex.numMips == 0) tex.numMips = 1;

	uint16_t alignment;
	unsigned int blockHeight, linesPerBlockHeight;
	if (tex.tileMode) blockHeight = linesPerBlockHeight = alignment = 1;
	else {

		alignment = 512;
		blockHeight = getBlockHeight(DIV_ROUND_UP(tex.height, bntxDecode.blkHeight));
		linesPerBlockHeight = blockHeight * 8;

	}

	vector<unsigned char> result_;
	int surfSize = 0, blockHeightShift = 0;
	size_t numOffsets = tex.mipOffsets.size();
	for (int64_t mipLevel = 0; mipLevel < numOffsets; mipLevel++) {

		int64_t mipOffset = getCurrentMipOffset(tex.width, tex.height, bntxDecode.blkWidth, bntxDecode.blkHeight, bntxDecode.bpp, mipLevel);
		//size_t mipSize = bntxDecode.result[mipLevel].size();

		uint32_t width_ = tex.width >> mipLevel;
		uint32_t height_ = tex.height >> mipLevel;
		if (width_ < 1) width_ = 1;
		if (height_ < 1) height_ = 1;

		int width__ = DIV_ROUND_UP(width_, bntxDecode.blkWidth);
		int height__ = DIV_ROUND_UP(height_, bntxDecode.blkHeight);

		unsigned int numAlignBytes = round_up(surfSize, alignment) - surfSize;
		surfSize += numAlignBytes;

		int pitch;
		if (tex.tileMode) {

			pitch = width__ * bntxDecode.bpp;
			if (tex.target) pitch = round_up(pitch, 32);
			surfSize += pitch * height__;

		}
		else {

			if (pow2_round_up(height__) < linesPerBlockHeight) blockHeightShift += 1;
			pitch = round_up(width__ * bntxDecode.bpp, 64);
				
			int blk = blockHeight >> blockHeightShift;
			if (blk < 1) blk = 1;
			surfSize += pitch * round_up(height__, blk * 8);

		}

		result_.resize(result_.size() + numAlignBytes);

		int log = tex.blockHeightLog2;
		log -= blockHeightShift;
		if (log < 0) log = 0;
		swizzle(width_, height_, bntxDecode.blkWidth, bntxDecode.blkHeight, tex.target, bntxDecode.bpp, tex.tileMode,
			log, buffer + hdr_size + mipOffset, true, result_, result_.size());
			
	}

	/*unsigned char* ptrs = this->fileData + tex.info.ptrsAddr;
	for (int64_t mipLevel = 0; mipLevel < numOffsets; mipLevel++) {

		int64_t mipOffset = tex.mipOffsets[mipLevel];
		uint64_t writeVal = tex.dataAddr + mipOffset;
		L32ToBytes(writeVal & (uint32_t)0xFFFFFFFF, ptrs + mipLevel * 8);
		L32ToBytes(writeVal >> 32, ptrs + mipLevel * 8 + 4);

	}*/

	memcpy(fileData + tex.dataAddr, result_.data(), texSizes[0]);
		
	ofstream outfile(bntxPath, ofstream::binary);
	outfile.write((char*)fileData, bntxLen);
	
}

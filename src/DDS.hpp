#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <map>

using std::vector;
using std::string;
using std::map;

class DDS_RW {

	public:

		typedef struct BNTXHeader {

			unsigned char magic[8];
			uint32_t version;
			uint16_t bom;
			uint8_t alignmentShift;
			uint8_t targetAddrSize;
			uint32_t fileNameAddr;
			uint16_t flag;
			uint16_t firstBlkAddr;
			uint32_t relocAddr;
			uint32_t fileSize;
			size_t size = 32;

			void init(unsigned char* data, unsigned int pos, unsigned char bom_);


		} BNTXHeader;

		typedef struct TexContainer {

			unsigned char target[4];
			uint32_t count;
			int64_t infoPtrsAddr;
			int64_t dataBlkAddr;
			int64_t dictAddr;
			int64_t memPoolAddr;
			int64_t memPoolPtr;
			uint32_t baseMemPoolAddr;
			size_t size = 56;

			void init(unsigned char* data, unsigned int pos, unsigned char bom);

		} TexContainer;

		typedef struct BlockHeader {

			unsigned char magic[4];
			uint32_t nextBlkAddr;
			uint32_t blockSize;
			size_t size = 16;

			void init(unsigned char* data, unsigned int pos, unsigned char bom);

		} BlockHeader;

		typedef struct TextureInfo {

			uint8_t flags;
			uint8_t dim;
			uint16_t tileMode;
			uint16_t swizzle;
			uint16_t numMips;
			uint16_t numSamples;
			uint32_t format_;
			uint32_t accessFlags;
			int32_t width;
			int32_t height;
			int32_t depth;
			uint32_t arrayLength;
			uint32_t textureLayout;
			uint32_t textureLayout2;
			uint32_t imageSize;
			uint32_t alignment;
			uint32_t compSel;
			uint8_t type_;
			int64_t nameAddr;
			int64_t parentAddr;
			int64_t ptrsAddr;
			int64_t userDataAddr;
			int64_t texPtr;
			int64_t texViewPtr;
			int64_t descSlotDataAddr;
			int64_t userDictAddr;
			size_t size = 144;

			void init(unsigned char* data, unsigned int pos, unsigned char bom);

		} TextureInfo;

		typedef struct TexInfo {

			int64_t infoAddr;
			TextureInfo info;
			unsigned char bom;
			bool target;
			
			string name;

			uint8_t readTexLayout;
			uint8_t sparseBinding;
			uint8_t sparseResidency;
			uint8_t dim;
			uint16_t tileMode;
			uint16_t numMips;
			int32_t width;
			int32_t height;
			uint32_t format;
			uint32_t arrayLength;
			uint8_t blockHeightLog2;
			uint32_t imageSize;

			vector<uint8_t> compSel;
			vector<uint8_t> compSel2;

			uint32_t alignment;
			uint8_t type;

			vector<int64_t> mipOffsets;
			int64_t dataAddr;

			vector<unsigned char> data;


		} TexInfo;

		typedef struct BNTXParseResult {

			string fname;
			unsigned char target[4];
			vector<TexInfo> textures;
			vector<string> texNames;

		} BNTXParseResult;

		typedef struct DecodeResult {

			vector<vector<unsigned char>> result;
			uint8_t blkWidth;
			uint8_t blkHeight;
			uint8_t bpp;

		} DecodeResult;

	private:
	
		//BNTXHeader bntxHeader;R5_G5_B5_A1_UNORM
		//TexContainer texContainer;
		//BlockHeader blockHeader;
		//TextureInfo textureInfo;
		//TexInfo tex;

		unsigned char* fileData = NULL;
		vector<uint32_t> texSizes;
		string bntxPath;
		size_t bntxLen;
		size_t hdr_size;
		BNTXParseResult bntxParse;
		DecodeResult bntxDecode;

		static uint64_t readN(unsigned char*& buf, int n, unsigned char bom);
		static int DIV_ROUND_UP(int n, int d) {
			return (n + d - 1) / d;
		}
		static int round_up(int x, int y) {
			return ((x - 1) | (y - 1)) + 1;
		}
		static int pow2_round_up(int x) {
			x -= 1;
			x |= x >> 1;
			x |= x >> 2;
			x |= x >> 4;
			x |= x >> 8;
			x |= x >> 16;

			return x + 1;
		}
		static int getBlockHeight(int height) {
			int blockHeight = pow2_round_up(height / 8);
			if (blockHeight > 16) blockHeight = 16;
			return blockHeight;
		}
		static int getCurrentMipOffset(int width, int height, int blkWidth, int blkHeight, int bpp, int currLevel) {
			int offset = 0, width_, height_;

			for (int mipLevel = 0; mipLevel < currLevel; mipLevel++) {
				width_ = width >> mipLevel;
				height_ = height >> mipLevel;

				if (width_ < 1) width_ = 1;
				if (height_ < 1) height_ = 1;

				width_ = DIV_ROUND_UP(width_, blkWidth);
				height_ = DIV_ROUND_UP(height_, blkHeight);

				offset += width_ * height_ * bpp;
			}
			return offset;
		}

		static int getAddrBlockLinear(int x, int y, int image_width, int bytes_per_pixel, int base_address, int blockHeight);
		static void swizzle(unsigned int width, unsigned int height, unsigned int blkWidth, unsigned int blkHeight, 
			bool roundPitch, uint8_t bpp, int tileMode, uint8_t blockHeightLog2, unsigned char* data, bool toSwizzle, vector<unsigned char>& result, int offset = 0);
		static size_t generateHeader(unsigned int num_mipmaps, int w, int h, string format_, vector<uint8_t>& compSel, size_t size, bool compressed, unsigned char*& hdr);
		void decode(TexInfo& tex);

		const vector<uint32_t> formats = {
			0x0101, // 'R4_G4_UNORM'
			0x0201, // 'R8_UNORM'
			0x0301, // 'R4_G4_B4_A4_UNORM'
			0x0401, // 'A4_B4_G4_R4_UNORM'
			0x0501, // 'R5_G5_B5_A1_UNORM'
			0x0601, // 'A1_B5_G5_R5_UNORM'
			0x0701, // 'R5_G6_B5_UNORM'
			0x0801, // 'B5_G6_R5_UNORM'
			0x0901, // 'R8_G8_UNORM'
			0x0b01, // 'R8_G8_B8_A8_UNORM'
			0x0b06, // 'R8_G8_B8_A8_SRGB'
			0x0c01, // 'B8_G8_R8_A8_UNORM'
			0x0c06, // 'B8_G8_R8_A8_SRGB'
			0x0e01, // 'R10_G10_B10_A2_UNORM'
			0x1a01, // 'BC1_UNORM'
			0x1a06, // 'BC1_SRGB'
			0x1b01, // 'BC2_UNORM'
			0x1b06, // 'BC2_SRGB'
			0x1c01, // 'BC3_UNORM'
			0x1c06, // 'BC3_SRGB'
			0x1d01, // 'BC4_UNORM'
			0x1d02, // 'BC4_SNORM'
			0x1e01, // 'BC5_UNORM'
			0x1e02, // 'BC5_SNORM'
			0x1f05, // 'BC6_FLOAT'
			0x1f0a, // 'BC6_UFLOAT'
			0x2001, // 'BC7_UNORM'
			0x2006, // 'BC7_SRGB'
			0x2d01, // 'ASTC_4x4_UNORM'
			0x2d06, // 'ASTC_4x4_SRGB'
			0x2e01, // 'ASTC_5x4_UNORM'
			0x2e06, // 'ASTC_5x4_SRGB'
			0x2f01, // 'ASTC_5x5_UNORM'
			0x2f06, // 'ASTC_5x5_SRGB'
			0x3001, // 'ASTC_6x5_UNORM'
			0x3006, // 'ASTC_6x5_SRGB'
			0x3101, // 'ASTC_6x6_UNORM'
			0x3106, // 'ASTC_6x6_SRGB'
			0x3201, // 'ASTC_8x5_UNORM'
			0x3206, // 'ASTC_8x5_SRGB'
			0x3301, // 'ASTC_8x6_UNORM'
			0x3306, // 'ASTC_8x6_SRGB'
			0x3401, // 'ASTC_8x8_UNORM'
			0x3406, // 'ASTC_8x8_SRGB'
			0x3501, // 'ASTC_10x5_UNORM'
			0x3506, // 'ASTC_10x5_SRGB'
			0x3601, // 'ASTC_10x6_UNORM'
			0x3606, // 'ASTC_10x6_SRGB'
			0x3701, // 'ASTC_10x8_UNORM'
			0x3706, // 'ASTC_10x8_SRGB'
			0x3801, // 'ASTC_10x10_UNORM'
			0x3806, // 'ASTC_10x10_SRGB'
			0x3901, // 'ASTC_12x10_UNORM'
			0x3906, // 'ASTC_12x10_SRGB'
			0x3a01, // 'ASTC_12x12_UNORM'
			0x3a06, // 'ASTC_12x12_SRGB'
			0x3b01  // 'B5_G5_R5_A1_UNORM'
		};

		const vector<uint8_t> blk_dims = { //# format -> (blkWidth, blkHeight)
			0x1a, 0x1c, // (4, 4)
			0x1b, // (4, 4)
			0x1c, // (4, 4)
			0x1d, // (4, 4)
			0x1e, // (4, 4)
			0x1f, // (4, 4)
			0x20, // (4, 4)
			0x2d, // (4, 4)
			0x2e, // (5, 4)
			0x2f, // (5, 5)
			0x30, // (6, 5)
			0x31, // (6, 6)
			0x32, // (8, 5)
			0x33, // (8, 6)
			0x34, // (8, 8)
			0x35, // (10, 5)
			0x36, // (10, 6)
			0x37, // (10, 8)
			0x38, // (10, 10)
			0x39, // (12, 10)
			0x3a  // (12, 12)
		};
		static uint8_t blkWidth(uint8_t dim);
		static uint8_t blkHeight(uint8_t dim);

		const map<uint8_t, uint8_t> bpps = { //# format->bytes_per_pixel
			{0x01, 0x01}, {0x02, 0x01}, {0x03, 0x02}, {0x05, 0x02}, {0x07, 0x02},
			{0x09, 0x02}, {0x0b, 0x04}, {0x0e, 0x04}, {0x1a, 0x08}, {0x1b, 0x10},
			{0x1c, 0x10}, {0x1d, 0x08}, {0x1e, 0x10}, {0x1f, 0x10}, {0x20, 0x10},
			{0x2d, 0x10}, {0x2e, 0x10}, {0x2f, 0x10}, {0x30, 0x10}, {0x31, 0x10},
			{0x32, 0x10}, {0x33, 0x10}, {0x34, 0x10}, {0x35, 0x10}, {0x36, 0x10},
			{0x37, 0x10}, {0x38, 0x10}, {0x39, 0x10}, {0x3a, 0x10}
		};

	public:

		//const BNTXHeader& getBNTXHeader() const;
		//const TexContainer& getTexContainer() const;
		//const BlockHeader& getBlockHeader() const;
		//const TextureInfo& getTextureInfo() const;

		~DDS_RW();
		
		unsigned char* getFileData();
		void setFileData(unsigned char* fileData);

		vector<uint32_t>& getTexSizes();
		void setTexSizes(const vector<uint32_t>& texSizes);

		BNTXParseResult& parseBNTX(string fullPath, bool getOnlyNameMode = false);
		void obtainDDS(TexInfo& tex, vector<unsigned char>& data);
		void injectDDS(unsigned char* buffer);
};

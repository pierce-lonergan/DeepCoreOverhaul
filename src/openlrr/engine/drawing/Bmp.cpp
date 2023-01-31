// Bmp.cpp : 
//

#include "../core/Errors.h"
#include "../core/Memory.h"

#include "Bmp.h"


/**********************************************************************************
 ******** Functions
 **********************************************************************************/

#pragma region Functions

// <LegoRR.exe @00489ef0>
void __cdecl Gods98::BMP_Parse(const void* data, uint32 fileSize, OUT BMP_Image* image)
{
	log_firstcall();

	const BMP_Header* header = static_cast<const BMP_Header*>(data);

	BMP_ParseInfoHeader(&header->info, image);

	// Use the real image offset defined in the file header.
	image->buffer1 = (static_cast<uint8*>(const_cast<void*>(data)) + header->file.img_offset);

	#if false
	if (header->info.bits_per_pixel == 8) {
		const uint32 colourCount = (header->info.colours == 0 ? 256 : header->info.colours);

		// Always create a table 256 long.
		BMP_PaletteEntry* palette = static_cast<BMP_PaletteEntry*>(Mem_Alloc(sizeof(BMP_PaletteEntry) * 256));
		std::memset(palette, 0, sizeof(BMP_PaletteEntry) * 256);
		std::memcpy(palette, (static_cast<const uint8*>(data) + sizeof(BMP_Header)), colourCount * sizeof(BMP_PaletteEntry));

		image->rgb = false;
		image->red_mask   = 0xfc; // mask value unique to paletted image
		image->green_mask = 0xfc;
		image->blue_mask  = 0xfc;
		image->alpha_mask = 0xfc;
		image->palette_size = (sint32)colourCount;
		for (uint32 i = 0; i < colourCount; i++) {
			const uint8 swap = palette[i].red; // Convert palette from BGR to RGB.
			palette[i].red = palette[i].blue;
			palette[i].blue = swap;
		}
		image->palette = palette; // place in image structure
	}
	else {
		image->rgb = true;
		image->palette_size = 0;
		image->palette = nullptr;
	}

	image->width  = (sint32)header->info.width;
	image->height = (sint32)header->info.height;
	image->depth  = header->info.bits_per_pixel;

	const uint32 byteDepth = (image->depth + 7) / 8;
	image->bytes_per_line = ((image->width * byteDepth) + 3) & ~3;

	image->buffer1 = (static_cast<uint8*>(data) + header->file.img_offset);
	image->buffer2 = nullptr;

	image->aspectx = image->aspecty = 1;
	#endif
}

/// CUSTOM: Parsing for just the BITMAPINFOHEADER struct without the file header->
void Gods98::BMP_ParseInfoHeader(const void* data, OUT BMP_Image* image)
{
	const BMP_InfoHeader* info = static_cast<const BMP_InfoHeader*>(data);

	uint32 paletteOffset = 0;
	if (info->bits_per_pixel == 8) {
		// BMP treats 0 as 256 palette colours.
		uint32 colourCount = (info->colours == 0 ? 256 : info->colours);
		paletteOffset = colourCount * sizeof(BMP_PaletteEntry);

		// It's impossible to have more than 256 colours with 8-bits per pixel,
		//  so cap at 256 to stop the compiler from complaining.
		colourCount = std::min(BMP_MAXPALETTEENTRIES, colourCount);
		
		// Always create a table 256 long.
		BMP_PaletteEntry* palette = static_cast<BMP_PaletteEntry*>(Mem_Alloc(sizeof(BMP_PaletteEntry) * BMP_MAXPALETTEENTRIES));
		std::memset(palette, 0, sizeof(BMP_PaletteEntry) * BMP_MAXPALETTEENTRIES);
		//std::memcpy(palette, (static_cast<uint8*>(data) + sizeof(BMP_InfoHeader)), colourCount * sizeof(BMP_PaletteEntry));
		std::memcpy(palette, (static_cast<const uint8*>(data) + info->img_header_size), colourCount * sizeof(BMP_PaletteEntry));

		image->rgb = false;
		image->palette_size = colourCount;
		for (uint32 i = 0; i < colourCount; i++) {
			std::swap(palette[i].red, palette[i].blue); // Convert palette from BGR to RGB.
		}
		image->palette = palette; // Place in image structure.
	}
	else {
		image->rgb = true;
		image->palette_size = 0;
		image->palette = nullptr;
	}

	image->width  = info->width;
	image->height = info->height;
	image->depth  = info->bits_per_pixel;

	// Pad to nearest 4 bytes.
	const uint32 byteDepth = (image->depth + 7) / 8;
	image->bytes_per_line = ((image->width * byteDepth) + 3) & ~3;

	image->buffer1 = (static_cast<uint8*>(const_cast<void*>(data)) + info->img_header_size + paletteOffset);
	image->buffer2 = nullptr;

	image->aspectx = image->aspecty = 1;

	BMP_SetupChannelMasks(image, true); // BMP treats 16-bit as 15-bit.
}

/// CUSTOM: Returns a pointer to the allocated buffer, which must be freed by the caller.
void* Gods98::BMP_Create(OUT BMP_Image* image, uint32 width, uint32 height, uint32 bpp, OPTIONAL OUT uint32* bufferSize, bool use15Bit)
{
	image->rgb = true;
	image->palette_size = 0;
	image->palette = nullptr;

	image->width  = width;
	image->height = height;
	image->depth  = bpp;

	// Pad to nearest 4 bytes.
	const uint32 byteDepth = (image->depth + 7) / 8;
	image->bytes_per_line = ((image->width * byteDepth) + 3) & ~3;

	const uint32 allocSize = (image->bytes_per_line * image->height);
	image->buffer1 = Mem_Alloc(allocSize);
	image->buffer2 = nullptr;

	std::memset(image->buffer1, 0, allocSize);
	if (bufferSize) *bufferSize = allocSize;

	image->aspectx = image->aspecty = 1;

	BMP_SetupChannelMasks(image, use15Bit);

	return image->buffer1;
}

/// CUSTOM: Setup channel masks for use in DirectDraw_SourceCopy.
void Gods98::BMP_SetupChannelMasks(IN OUT BMP_Image* image, bool use15Bit)
{
	if (!image->rgb) {
		image->red_mask   = 0xfc; // Mask value unique to paletted image.
		image->green_mask = 0xfc;
		image->blue_mask  = 0xfc;
		image->alpha_mask = 0xfc;
		return;
	}

	uint32 bitDepth = image->depth;
	if (image->depth == 16 && use15Bit) {
		//image->depth = 15;
		bitDepth = 15;
	}

	switch (bitDepth) {
	case 15: // 15-bpp (treated as 16-bpp)
		image->red_mask   = 0x7c00; // 0b0111110000000000
		image->green_mask = 0x03e0; // 0b0000001111100000
		image->blue_mask  = 0x001f; // 0b0000000000011111
		image->alpha_mask = 0;
		break;
	case 16: // 16-bpp (proper)
		image->red_mask   = 0xf800; // 0b1111100000000000
		image->green_mask = 0x07e0; // 0b0000011111100000
		image->blue_mask  = 0x001f; // 0b0000000000011111
		image->alpha_mask = 0;
		break;
	case 24:
		image->red_mask   = 0xff0000;
		image->green_mask = 0x00ff00;
		image->blue_mask  = 0x0000ff;
		image->alpha_mask = 0;
		break;
	case 32:
		image->red_mask   = 0x00ff0000;
		image->green_mask = 0x0000ff00;
		image->blue_mask  = 0x000000ff;
		image->alpha_mask = 0xff000000;
		break;
	}
}

/// CUSTOM: Assigns all fields of a bitmap header for writing to file.
void Gods98::BMP_SetupHeader(OUT BMP_Header* header, uint32 width, uint32 height, uint32 bpp, uint32 bufferSize)
{
	header->file.bmp_str[0] = 'B';
	header->file.bmp_str[1] = 'M';
	header->file.reserved1 = 0;
	header->file.reserved2 = 0;
	header->file.file_size = sizeof(BMP_Header) + bufferSize;
	header->file.img_offset = sizeof(BMP_Header);
	header->info.img_header_size = sizeof(BMP_InfoHeader);
	header->info.width  = width;
	header->info.height = height;
	header->info.planes = 1;
	header->info.bits_per_pixel = bpp;
	header->info.compression = 0; //BI_RGB;

	header->info.comp_img_size = bufferSize;
	header->info.horz_res = 1;
	header->info.vert_res = 1;
	header->info.colours = 0;
	header->info.important_colours = 0;
}

// <LegoRR.exe @0048a030>
void __cdecl Gods98::BMP_Cleanup(BMP_Image* image)
{
	log_firstcall();

	if (image->palette) {
		Mem_Free(image->palette);
		image->palette = nullptr;
	}
}

#pragma endregion

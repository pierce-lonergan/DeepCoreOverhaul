#pragma once

#include "../common.h"

#include "geometry.h"


#pragma region Colour Structs

/// TODO: Add `r`, `g`, `b`, `a` field aliases to all colours. They're much cleaner.

struct ColourRGBI {
	union {
		struct {
			/*0,4*/ uint32 r;
			/*4,4*/ uint32 g;
			/*8,4*/ uint32 b;
			/*c*/
		};
		/*0,c*/ std::array<uint32, 3> channels;
		/*c*/
	};
}; assert_sizeof(ColourRGBI, 0xc);


struct ColourRGBF {
	union {
		struct {
			/*0,4*/ real32 r;
			/*4,4*/ real32 g;
			/*8,4*/ real32 b;
			/*c*/
		};
		/*0,c*/		Vector3F xyz;
		/*0,c*/		std::array<real32, 3> channels;
		/*c*/
	};
}; assert_sizeof(ColourRGBF, 0xc);


struct ColourRGBAI {
	union {
		struct {
			/*00,4*/ uint32 r;
			/*04,4*/ uint32 g;
			/*08,4*/ uint32 b;
			/*0c,4*/ uint32 a;
			/*10*/
		};
		/*00,c*/	ColourRGBI rgb;
		/*00,10*/	std::array<uint32, 4> channels;
		/*10*/
	};
}; assert_sizeof(ColourRGBAI, 0x10);


struct ColourRGBAF {
	union {
		struct {
			/*00,4*/ real32 r;
			/*04,4*/ real32 g;
			/*08,4*/ real32 b;
			/*0c,4*/ real32 a;
			/*10*/
		};
		/*00,c*/	ColourRGBF rgb;
		/*00,c*/	Vector3F xyz;
		/*00,10*/	Vector4F xyzw;
		/*00,10*/	std::array<real32, 4> channels;
		/*10*/
	};
}; assert_sizeof(ColourRGBAF, 0x10);


#pragma pack(push, 1)

// 24-bit packed RGB-ordered colour channels
struct ColourRGBPacked {
	union {
		struct {
			/*0,1*/ uint8 r;
			/*1,1*/ uint8 g;
			/*2,1*/ uint8 b;
			/*3*/
		};
		/*0,3*/ std::array<uint8, 3> channels;
		/*3*/
	};
}; assert_sizeof(ColourRGBPacked, 0x3);

// 32-bit packed RGBA-ordered colour channels
struct ColourRGBAPacked {
	union {
		struct {
			/*0,1*/ uint8 r;
			/*1,1*/ uint8 g;
			/*2,1*/ uint8 b;
			/*3,1*/ uint8 a;
			/*4*/
		};
		/*0,4*/ std::array<uint8, 4> channels;
		/*0,4*/ uint32 rgbaColour;
		/*4*/
	};
	
}; assert_sizeof(ColourRGBAPacked, 0x4);


// 24-bit packed BGR-ordered colour channels
struct ColourBGRPacked {
	union {
		struct {
			/*0,1*/ uint8 b;
			/*1,1*/ uint8 g;
			/*2,1*/ uint8 r;
			/*3*/
		};
		/*0,3*/ std::array<uint8, 3> channels;
		/*3*/
	};
}; assert_sizeof(ColourBGRPacked, 0x3);

// 32-bit packed BGRA-ordered colour channels
struct ColourBGRAPacked {
	union {
		struct {
			/*0,1*/ uint8 b;
			/*1,1*/ uint8 g;
			/*2,1*/ uint8 r;
			/*3,1*/ uint8 a;
			/*4*/
		};
		/*0,4*/ std::array<uint8, 4> channels;
		/*0,4*/ uint32 bgraColour;
		/*4*/
	};
}; assert_sizeof(ColourRGBAPacked, 0x4);

#pragma pack(pop)

#pragma endregion

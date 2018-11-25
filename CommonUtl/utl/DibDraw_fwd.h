#ifndef DibDraw_fwd_h
#define DibDraw_fwd_h
#pragma once


enum RopCodes
{
	ROP_PSDPxax = 0xb8074a			// ( ( destination XOR pattern ) AND source ) XOR pattern
};


namespace gdi
{
	enum DisabledStyle { DisabledGrayScale, DisabledGrayOut, DisabledEffect, DisabledBlendColor, DisabledStd };		// for image list conversion
}


#endif // DibDraw_fwd_h

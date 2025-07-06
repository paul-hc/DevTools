#ifndef DibDraw_fwd_h
#define DibDraw_fwd_h
#pragma once


enum RopCodes
{
	ROP_PSDPxax = 0xb8074a			// ( ( destination XOR pattern ) AND source ) XOR pattern
};


class CEnumTags;


namespace gdi
{
	enum AlphaFading { AlphaFadeStd = 128, AlphaFadeMore = 112, AlphaFadeMost = 96 };


	enum DisabledStyle { Dis_FadeGray, Dis_GrayScale, Dis_GrayOut, Dis_DisabledEffect, Dis_BlendColor, Dis_MfcStd };		// for image list conversion

	const CEnumTags& GetTags_DisabledStyle( void );
}


#endif // DibDraw_fwd_h

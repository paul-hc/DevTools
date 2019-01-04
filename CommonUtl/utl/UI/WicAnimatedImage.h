#ifndef WicAnimatedImage_h
#define WicAnimatedImage_h
#pragma once

#include "WicImage.h"
#include <d2d1.h>


// container of animated image (typically GIF)
//
class CWicAnimatedImage : public CWicImage
{
public:
	CWicAnimatedImage( const wic::CBitmapDecoder& decoder );
	virtual ~CWicAnimatedImage();

	// base overrides
	virtual bool IsAnimated( void ) const;
	virtual const CSize& GetBmpSize( void ) const;

	const wic::CBitmapDecoder& GetDecoder( void ) const { return m_decoder; }

	// global metadata info
	const CSize& GetGifSize( void ) const { return m_gifSize; }
	const CSize& GetGifPixelSize( void ) const { return m_gifPixelSize; }
	const D2D1_COLOR_F& GetBkgndColor( void ) const { return m_bkgndColor; }
private:
	bool StoreGlobalMetadata( void );
	bool StoreBkgndColor( IWICMetadataQueryReader* pDecoderMetadata );
public:
	enum DisposalMethods
	{
		DM_Undefined	= 0,
		DM_None			= 1,
		DM_Background	= 2,
		DM_Previous		= 3
	};

	struct CFrameMetadata
	{
		CFrameMetadata( void ) { Reset(); }

		void Reset( void );
		void Store( IWICMetadataQueryReader* pMetadataReader );
	public:
		enum { MinDelay = 5, NoDelay = 0 };

		CRect m_rect;
		UINT m_frameDelay;						// miliseconds
		DisposalMethods m_frameDisposal;
	};
private:
	wic::CBitmapDecoder m_decoder;				// keep the decoder alive for animation
	bool m_isGif;
	CSize m_gifSize;
	CSize m_gifPixelSize;						// calculated using pixel aspect ratio
	D2D1_COLOR_F m_bkgndColor;					// image background from global metadata

	static const D2D1_COLOR_F s_transparentColor;
};


#endif // WicAnimatedImage_h

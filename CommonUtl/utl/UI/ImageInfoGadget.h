#ifndef ImageInfoGadget_h
#define ImageInfoGadget_h
#pragma once

#include "Direct2D.h"
#include "DirectWrite.h"
#include "IImageZoomView.h"
#include "WicImage.h"


namespace d2d
{
	class CImageInfoGadget : public CGadgetBase
	{
	public:
		CImageInfoGadget( ui::IImageZoomView* pImageView, IDWriteTextFormat* pInfoFont );
		~CImageInfoGadget();

		bool BuildInfo( void );

		// IDeviceComponent interface
		virtual void DiscardDeviceResources( void );
		virtual bool CreateDeviceResources( void );

		// IGadgetComponent interface
		virtual bool IsValid( void ) const;
		virtual void Draw( const CViewCoords& coords );
	private:
		bool MakeTextLayout( void );
	private:
		ui::IImageZoomView* m_pImageView;
		ui::CImageFileDetails m_info;
		UINT m_zoomPct;

		// device-independent
		CComPtr< IDWriteTextFormat > m_pInfoFont;

		// text rendering resources
		CComPtr< ID2D1Brush > m_pBkBrush;
		CComPtr< ID2D1Brush > m_pTextBrush;
		CComPtr< ID2D1Brush > m_pDimensionsBrush;
		CComPtr< ID2D1Brush > m_pNavigBrush;
		CComPtr< ID2D1Brush > m_pFrameTextBrush;

		CComPtr< IDWriteTextLayout > m_pInfoTextLayout;
		CComPtr< IDWriteTextLayout > m_pFrameTextLayout;		// displayed on top of the info, only for multi-frame images (not animated)
	};
}


#endif // ImageInfoGadget_h

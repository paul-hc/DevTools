#ifndef IImageZoomView_h
#define IImageZoomView_h
#pragma once

#include "Image_fwd.h"


class CScrollView;


namespace ui
{
	enum ViewStatusFlags
	{
		FullScreen			= BIT_FLAG( 0 ),
		ZoomMouseTracking	= BIT_FLAG( 1 )
	};

	typedef int TViewStatusFlag;


	interface IZoomView
	{
		virtual CScrollView* GetScrollView( void ) = 0;

		virtual CSize GetSourceSize( void ) const = 0;
		virtual ui::TDisplayColor GetBkColor( void ) const = 0;

		virtual ui::ImageScalingMode GetScalingMode( void ) const = 0;
		virtual UINT GetZoomPct( void ) const = 0;

		virtual bool IsAccented( void ) const = 0;			// for displaying the focused frame
		virtual bool HasViewStatusFlag( ui::TViewStatusFlag flag ) const = 0;
	};
}


class CWicImage;
namespace ui { struct CImageFileDetails; }


namespace ui
{
	interface IImageZoomView
	{
		virtual ui::IZoomView* GetZoomView( void ) = 0;

		virtual CWicImage* GetImage( void ) const = 0;
		virtual CWicImage* QueryImageFileDetails( ui::CImageFileDetails& rFileDetails ) const = 0;
	};
}


#endif // IImageZoomView_h

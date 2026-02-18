#ifndef IImageProxy_h
#define IImageProxy_h
#pragma once


enum SpecialImageIndex
{
	AutoImage = -2,		// lookup the image
	NoImage = -1,		// no image specified

	NoOverlayMask = 0	// no overlay image specified
};


namespace ui
{
	interface IImageProxy : public utl::IMemoryManaged
	{
		virtual bool IsEmpty( void ) const = 0;
		virtual const CSize& GetSize( void ) const = 0;
		virtual void SizeToText( CDC* /*pDC*/ ) {}

		virtual void Draw( CDC* pDC, const CPoint& pos, COLORREF transpColor = CLR_NONE ) const = 0;
		virtual void DrawDisabled( CDC* pDC, const CPoint& pos, COLORREF transpColor = CLR_NONE ) const = 0;

		// implemented methods
		virtual bool HasTransparency( void ) const { return false; }
		virtual void FillBackground( CDC* pDC, const CPoint& pos, COLORREF bkColor ) const;
	};
}


#endif // IImageProxy_h

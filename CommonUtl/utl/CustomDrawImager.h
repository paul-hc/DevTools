#ifndef CustomDrawImager_h
#define CustomDrawImager_h
#pragma once

#include "CustomDrawImager_fwd.h"


class CGlyphThumbnailer;


// shared thumbnail store for file/folder items that works with with shell icons and image thumbnails automatically
//
class CFileItemsThumbnailStore
{
	CFileItemsThumbnailStore( void );
	~CFileItemsThumbnailStore();
public:
	static CFileItemsThumbnailStore& Instance( void );
	static int GetDefaultGlyphDimension( ui::GlyphGauge glyphGauge );

	int GetGlyphDimension( ui::GlyphGauge glyphGauge ) const;
	bool SetGlyphDimension( ui::GlyphGauge glyphGauge, int glyphDimension );

	CSize GetGlyphSize( ui::GlyphGauge glyphGauge ) const;

	CGlyphThumbnailer* GetThumbnailer( ui::GlyphGauge glyphGauge );

	void RegisterControl( ICustomDrawControl* pCustomDrawCtrl );
	void UnregisterControl( ICustomDrawControl* pCustomDrawCtrl );
	void UpdateControls( void );
private:
	const CGlyphThumbnailer* GetThumbnailer( ui::GlyphGauge glyphGauge ) const { return const_cast< CFileItemsThumbnailStore* >( this )->GetThumbnailer( glyphGauge ); }
private:
	std::auto_ptr< CGlyphThumbnailer > m_pThumbnailer[ ui::_GlyphGaugeCount ];		// self-encapsulated
	std::vector< ICustomDrawControl* > m_customDrawCtrls;							// to notify controls of changed glyph metrics
};


// Custom draw imager for list or tree controls that contains small and large imagelists, with one transparent image at m_transpImageIndex (=0).
// Actual thumbnails are custom drawn on top of the transparent image.
//
class CBaseCustomDrawImager
{
protected:
	CBaseCustomDrawImager( void ) {}

	void InitImageLists( const CSize& smallImageSize, const CSize& largeImageSize );
public:
	virtual ~CBaseCustomDrawImager();

	CImageList* GetImageList( ui::GlyphGauge glyphGauge ) { return &m_imageLists[ glyphGauge ]; }
	int GetTranspImageIndex( void ) const { return m_transpImageIndex; }

	bool DrawItemGlyph( const NMCUSTOMDRAW* pDraw, const CRect& itemImageRect );		// for CListCtrl, CTreeCtrl

	// glyph custom drawing
	virtual ui::ICustomImageDraw* GetRenderer( void ) const = 0;
	virtual bool SetCurrGlyphGauge( ui::GlyphGauge currGlyphGauge ) = 0;
private:
	CImageList m_imageLists[ ui::_GlyphGaugeCount ];
	int m_transpImageIndex;								// index of the transparent entry in the image list
};


// uses the shared CFileItemsThumbnailStore with dual thumbnailers for small and large thumbnails
//
class CFileGlyphCustomDrawImager : public CBaseCustomDrawImager
{
public:
	CFileGlyphCustomDrawImager( ui::GlyphGauge currGlyphGauge );

	// base overrides
	virtual ui::ICustomImageDraw* GetRenderer( void ) const;
	virtual bool SetCurrGlyphGauge( ui::GlyphGauge currGlyphGauge );
private:
	ui::GlyphGauge m_currGlyphGauge;			// currently used by the control
};


// uses a single thumbnailer (renderer) switching bounds size to current glyph gauge
//
struct CSingleCustomDrawImager : public CBaseCustomDrawImager
{
	CSingleCustomDrawImager( ui::ICustomImageDraw* pRenderer, const CSize& smallImageSize, const CSize& largeImageSize );

	virtual ui::ICustomImageDraw* GetRenderer( void ) const;
	virtual bool SetCurrGlyphGauge( ui::GlyphGauge currGlyphGauge );
private:
	ui::ICustomImageDraw* m_pRenderer;					// single thumbnailer
	const bool m_ctrlDrivesBoundsSize;					// true if thumbnailer bounds size is driven by the list control; otherwise the thumbnailer drives image sizes
	CSize m_imageSizes[ ui::_GlyphGaugeCount ];
};


#endif // CustomDrawImager_h

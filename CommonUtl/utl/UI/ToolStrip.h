#ifndef ToolStrip_h
#define ToolStrip_h
#pragma once

#include "Image_fwd.h"


class CImageStore;


struct CToolStrip
{
	CToolStrip( void ) : m_imageSize( 0, 0 ) {}
	~CToolStrip();

	bool IsValid( void ) const { return !m_buttonIds.empty(); }
	void Clear( void );

	int GetImageCount( void ) const;		// buttons - separators
	bool HasImages( void ) const { return m_pImageList.get() != NULL; }
	CImageList* EnsureImageList( void );

	bool LoadStrip( UINT toolStripId, COLORREF transpColor = color::Auto );

	enum { UseSharedImage = -2, UseButtonId, NullIconId };

	CToolStrip& AddButton( UINT buttonId, CIconId iconId = CIconId( (UINT)UseButtonId ) );
	CToolStrip& AddButton( UINT buttonId, HICON hIcon );
	CToolStrip& AddSeparator( void ) { return AddButton( 0 ); }

	void AddButtons( const UINT buttonIds[], size_t buttonCount, IconStdSize iconStdSize = SmallIcon );		// same buttonId and iconId

	void RegisterButtons( CImageStore* pImageStore );
	static void RegisterStripButtons( UINT toolStripId, COLORREF transpColor = color::Auto, CImageStore* pImageStore = NULL );
public:
	CSize m_imageSize;
	std::vector< UINT > m_buttonIds;
	std::auto_ptr< CImageList > m_pImageList;
};


#endif // ToolStrip_h

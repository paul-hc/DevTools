/////////////////////////////////////////////////////////
//
// Copyright (C) 2022, Oracle. All rights reserved.
//
#ifndef ToolImageList_h
#define ToolImageList_h
#pragma once

#include "Image_fwd.h"
#include "StdColors.h"


namespace ui
{
	interface IImageStore;

	ui::IImageStore* GetImageStoresSvc( void );
}


// encapsulates an image list and toolbar buttons + separators: equivalent to a loaded toolbar resource

class CToolImageList
{
public:
	CToolImageList( IconStdSize iconStdSize = SmallIcon );
	CToolImageList( const UINT buttonIds[], size_t count, IconStdSize iconStdSize = SmallIcon );
	~CToolImageList();

	void Clear( void );

	bool IsValid( void ) const { return HasButtons() && HasImages(); }

	bool HasButtons( void ) const { return !m_buttonIds.empty(); }
	bool HasImages( void ) const { return m_pImageList.get() != nullptr; }
	bool HasAlpha( void ) const { return m_hasAlpha; }

	IconStdSize GetIconStdSize( void ) const { return m_imageSize.GetStdSize(); }

	const CSize& GetImageSize( void ) const { return m_imageSize.GetSize(); }
	void SetImageSize( const CIconSize& glyphSize ) { m_imageSize = glyphSize; }

	TImageListFlags GetImageListFlags( void ) const { return m_imageListFlags; }
	TBitsPerPixel GetBitsPerPixel( void ) const { return ui::CImageListInfo::GetBitsPerPixel( m_imageListFlags ); }	// exclude ILC_MASK if not monochrome
	ui::CImageListInfo GetImageListInfo( void ) const;

	CImageList* GetImageList( void ) const { return m_pImageList.get(); }
	CImageList* CreateImageList( int countOrGrowBy = -5 );
	CImageList* EnsureImageList( void );					// ensure button images are loaded (by default from the store)
	void ResetImageList( CImageList* pImageList );			// store the new imagelist along with the new image size

	const std::vector<UINT>& GetButtonIds( void ) const { return m_buttonIds; }
	std::vector<UINT>& RefButtonIds( void ) { return m_buttonIds; }
	void StoreButtonIds( const UINT buttonIds[], size_t count ) { m_buttonIds.assign( buttonIds, buttonIds + count ); }

	int GetImageCount( void ) const;										// buttons minus separators
	static int EvalButtonCount( const UINT buttonIds[], size_t count );		// buttons minus separators

	bool ContainsButton( UINT buttonId ) const { return FindButtonPos( buttonId ) != utl::npos; }
	size_t FindButtonPos( UINT buttonId ) const;

	bool LoadToolbar( UINT toolBarId, COLORREF transpColor = CLR_NONE );					// loads imagelist and button IDs from toolbar resource
	bool _LoadIconStrip( UINT iconStripId, const UINT buttonIds[], size_t count );			// creates imagelist from icon strip (custom size multi-images) and stores button IDs - not used!
	bool LoadButtonImages( ui::IImageStore* pSrcImageStore = ui::GetImageStoresSvc() );		// creates imagelist from button IDs
private:
	CIconSize m_imageSize;
	TImageListFlags m_imageListFlags;
	bool m_hasAlpha;
protected:
	std::vector<UINT> m_buttonIds;
	std::auto_ptr<CImageList> m_pImageList;
};


#endif // ToolImageList_h

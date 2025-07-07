#ifndef ImageStore_fwd_h
#define ImageStore_fwd_h
#pragma once

#include "Image_fwd.h"


namespace ui
{
	struct CIconEntry
	{
		CIconEntry( void ) : m_bitsPerPixel( 0 ), m_dimension( 0 ), m_stdSize( DefaultSize ) {}

		CIconEntry( TBitsPerPixel bitsPerPixel, IconStdSize stdSize )
			: m_bitsPerPixel( bitsPerPixel )
			, m_dimension( ui::GetIconDimension( stdSize ) )
			, m_stdSize( stdSize )
		{
		}

		CIconEntry( TBitsPerPixel bitsPerPixel, int dimension )
			: m_bitsPerPixel( bitsPerPixel )
			, m_dimension( dimension )
			, m_stdSize( ui::LookupIconStdSize( m_dimension ) )
		{
		}

		CIconEntry( TBitsPerPixel bitsPerPixel, const CSize& imageSize )
			: m_bitsPerPixel( bitsPerPixel )
			, m_dimension( ui::GetIconDimension( imageSize ) )
			, m_stdSize( ui::LookupIconStdSize( m_dimension ) )
		{
		}

		bool operator==( const CIconEntry& right ) const;

		CSize GetSize( void ) const { return CSize( m_dimension, m_dimension ); }
	public:
		TBitsPerPixel m_bitsPerPixel;		// ILC_COLOR32/ILC_COLOR24/ILC_COLOR16/ILC_COLOR8/ILC_COLOR4/ILC_MASK
		int m_dimension;					// 16(x16), 24(x24), 32(x32), etc
		IconStdSize m_stdSize;				// SmallIcon/MediumIcon/LargeIcon/...
	};


	struct CIconKey : public CIconEntry
	{
		CIconKey( void ) : CIconEntry(), m_iconResId( 0 ) {}
		CIconKey( UINT iconResId, const CIconEntry& iconEntry ) : CIconEntry( iconEntry ), m_iconResId( iconResId ) {}
	public:
		UINT m_iconResId;
	};
}


namespace ui
{
	struct CStripBtnInfo
	{
		CStripBtnInfo( UINT cmdId = 0, int imagePos = -1 ) : m_cmdId( cmdId ), m_imagePos( imagePos ) {}
	public:
		UINT m_cmdId;
		int m_imagePos;
	};


	class CToolbarDescr : private utl::noncopyable
	{
	public:
		CToolbarDescr( UINT toolbarId, const UINT buttonIds[] = nullptr, size_t buttonCount = 0 );

		UINT GetToolbarId( void ) const { return m_toolbarId; }
		const std::tstring& GetToolbarTitle( void ) const { return m_toolbarTitle; }
		const std::vector<ui::CStripBtnInfo>& GetBtnInfos( void ) const { return m_btnInfos; }

		const ui::CStripBtnInfo* FindBtnInfo( UINT cmdId ) const;
		bool ContainsBtn( UINT cmdId ) const { return FindBtnInfo( cmdId ) != nullptr; }

		void StoreBtnInfos( const UINT buttonIds[], size_t buttonCount );
		void Clear( void ) { m_toolbarId = 0; m_toolbarTitle.clear(); m_btnInfos.clear(); }
	private:
		UINT m_toolbarId;
		std::tstring m_toolbarTitle;
		std::vector<ui::CStripBtnInfo> m_btnInfos;		// excluding separators
	};
}


namespace utl
{
	struct CStripBtnInfoHasher		// use with std::unordered_map<> for pairs
	{
		size_t operator()( const ui::CStripBtnInfo& stripBtnInfo ) const;
	};
}


class CIcon;
class CIconGroup;


namespace ui
{
	interface IImageStore
	{
		virtual CIconGroup* FindIconGroup( UINT cmdId ) const = 0;
		virtual const CIcon* RetrieveIcon( const CIconId& cmdId ) = 0;
		virtual CBitmap* RetrieveBitmap( const CIconId& cmdId, COLORREF transpColor ) = 0;

		typedef std::pair<CBitmap*, CBitmap*> TBitmapPair;		// <bmp_unchecked, bmp_checked>

		virtual TBitmapPair RetrieveMenuBitmaps( const CIconId& cmdId ) = 0;
		virtual TBitmapPair RetrieveMenuBitmaps( const CIconId& cmdId, bool useCheckedBitmaps ) = 0;

		virtual void QueryToolbarDescriptors( std::vector<ui::CToolbarDescr*>& rToolbarDescrs ) const = 0;
		virtual void QueryToolbarsWithButton( std::vector<ui::CToolbarDescr*>& rToolbarDescrs, UINT cmdId ) const = 0;
		virtual void QueryIconGroups( std::vector<CIconGroup*>& rIconGroups ) const = 0;
		virtual void QueryIconKeys( std::vector<ui::CIconKey>& rIconKeys, IconStdSize iconStdSize = AnyIconSize ) const = 0;

		// utils
		CIcon* FindIcon( UINT cmdId, IconStdSize iconStdSize = SmallIcon, TBitsPerPixel bitsPerPixel = ILC_COLOR ) const;		// ILC_COLOR means best match, i.e. highest color
		const CIcon* RetrieveLargestIcon( UINT cmdId, IconStdSize maxIconStdSize = HugeIcon_48 );
		CBitmap* RetrieveMenuBitmap( const CIconId& cmdId ) { return RetrieveBitmap( cmdId, ::GetSysColor( COLOR_MENU ) ); }
		int BuildImageList( CImageList* pDestImageList, const UINT buttonIds[], size_t buttonCount, const CSize& imageSize );
	};


	ui::IImageStore* GetImageStoresSvc( void );
}


namespace pred
{
	struct HasCmdId : public std::unary_function<ui::CStripBtnInfo, bool>
	{
		HasCmdId( UINT cmdId ) : m_cmdId( cmdId ) {}

		bool operator()( const ui::CStripBtnInfo& btnInfo ) const
		{
			return m_cmdId == btnInfo.m_cmdId;
		}

		bool operator()( const ui::CToolbarDescr* pToolbarDescr ) const
		{
			ASSERT_PTR( pToolbarDescr );
			return pToolbarDescr->ContainsBtn( m_cmdId );
		}
	private:
		UINT m_cmdId;
	};
}


#endif // ImageStore_fwd_h

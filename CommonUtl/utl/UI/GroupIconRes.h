#ifndef GroupIconRes_h
#define GroupIconRes_h
#pragma once

#include "ComparePredicates.h"
#include "Image_fwd.h"
#include "ResourceData.h"


#pragma pack( push )
#pragma pack( 2 )				// insure that the structure's packing in memory matches the packing of the EXE or DLL


namespace res
{
	// RT_GROUP_ICON resource data.
	//	For file-based icons, the structures are a little different, see Win32 icons documentation:
	//		https://learn.microsoft.com/en-us/previous-versions/ms997538(v=msdn.10)?redirectedfrom=MSDN)

	struct CGroupIconEntry			// GRPICONDIRENTRY - not defined in Windows headers
	{
		int GetWidth( void ) const { return m_width != 0 ? m_width : 256; }
		int GetHeight( void ) const { return m_height != 0 ? m_height : 256; }
		CSize GetSize( void ) const { return CSize( GetWidth(), GetHeight() ); }
		int GetDimension( void ) const { return std::max( GetWidth(), GetHeight() ); }

		TBitsPerPixel GetBitsPerPixel( void ) const { return m_colorPlanes * m_bitCount; }
	private:
		// encapsulated fields:
		BYTE m_width;				// width of the image (pixels) - 0 means 256 (see Note 1 bellow)
		BYTE m_height;				// height of the image (pixels) - 0 means 256 (see Note 1 bellow)
		BYTE m_colorCount;			// number of colors in image (0 if >= 8bpp)
		BYTE m_reserved;
	public:
		WORD m_colorPlanes;			// color planes
		WORD m_bitCount;			// bits per pixel
		DWORD m_bytesInRes;			// how many bytes in this resource?
		WORD m_id;					// the ID
	};

	struct CGroupIconDir			// GRPICONDIR - not defined in Windows headers
	{
		const CGroupIconEntry* Begin( void ) const { return &m_images[ 0 ]; }
		const CGroupIconEntry* End( void ) const { return &m_images[ m_count ]; }
		const CGroupIconEntry* Last( void ) const { ASSERT( m_count != 0 ); return &m_images[ m_count - 1 ]; }
	public:
		WORD m_reserved;
		WORD m_type;						// resource type: 1 for icons, 0 for cursor
		WORD m_count;						// image count
		CGroupIconEntry m_images[ 1 ];		// the entries for each image
	};

	// Note 1: https://devblogs.microsoft.com/oldnewthing/20101018-00/?p=12513 - the BYTE value of 0 means 256
}


#pragma pack( pop )


class CIcon;


// RT_GROUP_ICON resource info
//
class CGroupIconRes
{
public:
	CGroupIconRes( UINT iconId );

	bool IsValid( void ) const { return m_pGroupIconDir != nullptr; }

	size_t GetSize( void ) const { ASSERT( IsValid() ); return m_pGroupIconDir->m_count; }
	const res::CGroupIconDir* GetIconDir( void ) const { ASSERT( IsValid() ); return m_pGroupIconDir; }
	const res::CGroupIconEntry& GetIconEntryAt( size_t framePos ) const { ASSERT( framePos < GetSize() ); return m_pGroupIconDir->m_images[ framePos ]; }

	CIcon* LoadIconAt( size_t framePos ) const;
	static HICON LoadIconEntry( const res::CGroupIconEntry& iconEntry );		// caller must delete the icon

	enum { AnyBpp = 0 };

	const res::CGroupIconEntry* FindMatch( TBitsPerPixel bitsPerPixel, IconStdSize iconStdSize ) const;
	const res::CGroupIconEntry* FindMatch( IconStdSize iconStdSize ) const { return FindMatch( AnyBpp, iconStdSize ); }		// highest BPP for the desired size

	bool Contains( TBitsPerPixel bitsPerPixel, IconStdSize iconStdSize ) const { return FindMatch( bitsPerPixel, iconStdSize ) != nullptr; }
	bool ContainsSize( IconStdSize iconStdSize, TBitsPerPixel* pOutBitsPerPixel = nullptr ) const;
	bool ContainsBpp( TBitsPerPixel bitsPerPixel, IconStdSize* pOutIconStdSize = nullptr ) const;

	std::pair<TBitsPerPixel, IconStdSize> Front( void ) const { return IsValid() ? ToBppSize( m_pGroupIconDir->Begin() ) : s_nullBppStdSize; }
	std::pair<TBitsPerPixel, IconStdSize> Back( void ) const { return IsValid() ? ToBppSize( m_pGroupIconDir->Last() ) : s_nullBppStdSize; }

	std::pair<TBitsPerPixel, IconStdSize> FindSmallest( void ) const;
	std::pair<TBitsPerPixel, IconStdSize> FindLargest( void ) const;

	void QueryAvailableSizes( std::vector< std::pair<TBitsPerPixel, IconStdSize> >& rIconPairs ) const;
	void QueryAvailableSizes( std::vector< std::pair<TBitsPerPixel, CSize> >& rIconPairs ) const;

	enum { IconVersionNumberFormat = 0x00030000 };
private:
	static std::pair<TBitsPerPixel, IconStdSize> ToBppSize( const res::CGroupIconEntry* pIconEntry )
	{
		ASSERT_PTR( pIconEntry );
		return std::make_pair( pIconEntry->GetBitsPerPixel(), CIconSize::FindStdSize( pIconEntry->GetSize() ) );
	}
private:
	CResourceData m_resGroupIcon;
	const res::CGroupIconDir* m_pGroupIconDir;
	static const std::pair<TBitsPerPixel, IconStdSize> s_nullBppStdSize;
};


#include "IconGroup.h"		// for pred::CompareWidth


namespace pred
{
	struct CompareIcon_Size
	{
		CompareResult operator()( const std::pair<TBitsPerPixel, CSize>& left, const std::pair<TBitsPerPixel, CSize>& right ) const
		{
			return CompareWidth()( left.second, right.second );
		}

		CompareResult operator()( const std::pair<TBitsPerPixel, IconStdSize>& left, const std::pair<TBitsPerPixel, IconStdSize>& right ) const
		{
			return Compare_Scalar( left.second, right.second );
		}

		CompareResult operator()( const res::CGroupIconEntry& left, const res::CGroupIconEntry& right ) const
		{
			return CompareWidth()( left.GetSize(), right.GetSize() );
		}
	};


	struct CompareIcon_BppSize
	{
		CompareResult operator()( const std::pair<TBitsPerPixel, CSize>& left, const std::pair<TBitsPerPixel, CSize>& right ) const
		{
			CompareResult result = Compare_Scalar( left.first, right.first );
			if ( Equal == result )
				result = CompareWidth()( left.second, right.second );
			return result;
		}

		CompareResult operator()( const std::pair<TBitsPerPixel, IconStdSize>& left, const std::pair<TBitsPerPixel, IconStdSize>& right ) const
		{
			CompareResult result = Compare_Scalar( left.first, right.first );
			if ( Equal == result )
				result = Compare_Scalar( left.second, right.second );
			return result;
		}

		CompareResult operator()( const res::CGroupIconEntry& left, const res::CGroupIconEntry& right ) const
		{
			CompareResult result = Compare_Scalar( left.GetBitsPerPixel(), right.GetBitsPerPixel() );
			if ( Equal == result )
				result = CompareWidth()( left.GetSize(), right.GetSize() );
			return result;
		}
	};
}


namespace dbg
{
	std::tstring FormatGroupIconEntry( const res::CGroupIconEntry& entry );
}


#endif // GroupIconRes_h

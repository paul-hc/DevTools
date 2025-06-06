#ifndef GroupIcon_h
#define GroupIcon_h
#pragma once

#include "ComparePredicates.h"
#include "Image_fwd.h"
#include "ResourceData.h"


typedef WORD TBitsPerPixel;


#pragma pack( push )
#pragma pack( 2 )				// insure that the structure's packing in memory matches the packing of the EXE or DLL

namespace res
{
	// RT_GROUP_ICON resource data

	struct CGroupIconEntry			// GRPICONDIRENTRY
	{
		CSize GetSize( void ) const { return CSize( m_width, m_height ); }
		TBitsPerPixel GetBitsPerPixel( void ) const { return m_colorPlanes * m_bitCount; }
	public:
		BYTE m_width;				// width of the image (pixels)
		BYTE m_height;				// height of the image (pixels)
		BYTE m_colorCount;			// number of colors in image (0 if >= 8bpp)
		BYTE m_reserved;
		WORD m_colorPlanes;			// color planes
		WORD m_bitCount;			// bits per pixel
		DWORD m_bytesInRes;			// how many bytes in this resource?
		WORD m_id;					// the ID
	};

	struct CGroupIconDir			// GRPICONDIR
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
}


#pragma pack( pop )


namespace pred
{
	inline CompareResult CompareWidth( const CSize& left, const CSize& right )
	{
		return Compare_Scalar( left.cx, right.cx );
	}


	struct CompareIcon_Size
	{
		CompareResult operator()( const std::pair<TBitsPerPixel, CSize>& left, const std::pair<TBitsPerPixel, CSize>& right ) const
		{
			return CompareWidth( left.second, right.second );
		}

		CompareResult operator()( const std::pair<TBitsPerPixel, IconStdSize>& left, const std::pair<TBitsPerPixel, IconStdSize>& right ) const
		{
			return Compare_Scalar( left.second, right.second );
		}

		CompareResult operator()( const res::CGroupIconEntry& left, const res::CGroupIconEntry& right ) const
		{
			return CompareWidth( left.GetSize(), right.GetSize() );
		}
	};


	struct CompareIcon_BppSize
	{
		CompareResult operator()( const std::pair<TBitsPerPixel, CSize>& left, const std::pair<TBitsPerPixel, CSize>& right ) const
		{
			CompareResult result = Compare_Scalar( left.first, right.first );
			if ( Equal == result )
				result = CompareWidth( left.second, right.second );
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
				result = CompareWidth( left.GetSize(), right.GetSize() );
			return result;
		}
	};
}


class CGroupIcon		// RT_GROUP_ICON resource info
{
public:
	CGroupIcon( UINT iconId );

	bool IsValid( void ) const { return m_pGroupIconDir != nullptr; }
	const res::CGroupIconDir* GetDir( void ) const { return m_pGroupIconDir; }

	enum { AnyBpp = 0 };

	const res::CGroupIconEntry* FindMatch( TBitsPerPixel bitsPerPixel, IconStdSize iconStdSize ) const;

	bool Contains( TBitsPerPixel bitsPerPixel, IconStdSize iconStdSize ) const { return FindMatch( bitsPerPixel, iconStdSize ) != nullptr; }
	bool ContainsSize( IconStdSize iconStdSize, TBitsPerPixel* pBitsPerPixel = nullptr ) const;
	bool ContainsBpp( TBitsPerPixel bitsPerPixel, IconStdSize* pIconStdSize = nullptr ) const;

	std::pair<TBitsPerPixel, IconStdSize> Front( void ) const { return IsValid() ? ToBppSize( m_pGroupIconDir->Begin() ) : m_nullBppStdSize; }
	std::pair<TBitsPerPixel, IconStdSize> Back( void ) const { return IsValid() ? ToBppSize( m_pGroupIconDir->Last() ) : m_nullBppStdSize; }

	std::pair<TBitsPerPixel, IconStdSize> FindSmallest( void ) const;
	std::pair<TBitsPerPixel, IconStdSize> FindLargest( void ) const;

	void QueryAvailableSizes( std::vector< std::pair<TBitsPerPixel, IconStdSize> >& rIconPairs ) const;
	void QueryAvailableSizes( std::vector< std::pair<TBitsPerPixel, CSize> >& rIconPairs ) const;
private:
	static std::pair<TBitsPerPixel, IconStdSize> ToBppSize( const res::CGroupIconEntry* pIconEntry )
	{
		ASSERT_PTR( pIconEntry );
		return std::make_pair( pIconEntry->GetBitsPerPixel(), CIconSize::FindStdSize( pIconEntry->GetSize() ) );
	}
private:
	CResourceData m_resGroupIcon;
	const res::CGroupIconDir* m_pGroupIconDir;
	static const std::pair<TBitsPerPixel, IconStdSize> m_nullBppStdSize;
};


#endif // GroupIcon_h

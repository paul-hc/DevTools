#ifndef IconGroup_h
#define IconGroup_h
#pragma once

#include "Image_fwd.h"


// Owns multiple CIcon objects loaded from the same resource ID, typically corresponding to a structure of res::CGroupIconDir and res::CGroupIconEntry.
//
class CIconGroup
{
public:
	typedef std::pair<ui::CIconEntry, CIcon*> TIconPair;

	CIconGroup( void ) : m_iconResId( 0 ) {}
	~CIconGroup() { Clear(); }

	void Clear( void );

	UINT GetIconResId( void ) const { return m_iconResId; }
	void SetIconResId( UINT iconResId ) { m_iconResId = iconResId; Clear(); }

	size_t LoadAllIcons( UINT iconResId );

	bool IsEmpty( void ) const { return m_frames.empty(); }
	size_t GetSize( void ) const { return m_frames.size(); }

	const TIconPair& GetAt( size_t framePos ) const { ASSERT( framePos < GetSize() ); return m_frames[ framePos ]; }

	const ui::CIconEntry& GetEntryAt( size_t framePos ) const { ASSERT( framePos < GetSize() ); return m_frames[ framePos ].first; }
	CIcon* GetIconAt( size_t framePos ) const { ASSERT( framePos < GetSize() ); return m_frames[ framePos ].second; }

	ui::CIconKey GetIconKeyAt( size_t framePos ) const { ASSERT( framePos < GetSize() ); return ui::CIconKey( m_iconResId, GetEntryAt( framePos ) ); }

	size_t AddIcon( const ui::CIconEntry& entry, CIcon* pIcon );		// in sorted order
	bool AugmentIcon( const ui::CIconEntry& entry, CIcon* pIcon );		// in sorted order
	void DeleteFrameAt( size_t framePos );

	size_t FindPos( const ui::CIconEntry& entry ) const;
	TIconPair* FindEntry( const ui::CIconEntry& entry ) const;
	CIcon* FindIcon( const ui::CIconEntry& entry ) const;

	// best matchin icon - first matching pair with same iconStdSize, maximum BitsPerPixel:
	size_t FindBestMatchingPos( IconStdSize iconStdSize ) const;
	CIcon* FindBestMatchingIcon( IconStdSize iconStdSize ) const;
	const TIconPair* FindBestMatchingPair( IconStdSize iconStdSize ) const;
private:
	UINT m_iconResId;
	std::vector<TIconPair> m_frames;
};


namespace pred
{
	struct CompareBitsPerPixel
	{
		CompareResult operator()( const ui::CIconEntry& left, const ui::CIconEntry& right ) const
		{
			return Compare_Scalar( left.m_bitsPerPixel, right.m_bitsPerPixel );
		}

		CompareResult operator()( const CIconGroup::TIconPair& left, const CIconGroup::TIconPair& right ) const
		{
			return Compare_Scalar( left.first.m_bitsPerPixel, right.first.m_bitsPerPixel );
		}
	};


	struct CompareWidth
	{
		CompareResult operator()( const CSize& left, const CSize& right ) const
		{
			return Compare_Scalar( left.cx, right.cx );
		}

		CompareResult operator()( const ui::CIconEntry& left, const ui::CIconEntry& right ) const
		{
			return Compare_Scalar( left.m_dimension, right.m_dimension );
		}

		CompareResult operator()( const CIconGroup::TIconPair& left, const CIconGroup::TIconPair& right ) const
		{
			return Compare_Scalar( left.first.m_dimension, right.first.m_dimension );
		}
	};


	typedef Descending<CompareBitsPerPixel> TCompareBitsPerPixel_Desc;
	typedef JoinCompare<TCompareBitsPerPixel_Desc, CompareWidth> TCompareIconEntry;		// BitsPerPixel (Desc) | Width


	struct HasIconResId
	{
		HasIconResId( UINT iconResId ) : m_iconResId( iconResId ) {}

		bool operator()( const CIconGroup* pIconGroup ) const
		{
			return pIconGroup->GetIconResId() == m_iconResId;
		}
	private:
		UINT m_iconResId;
	};
}


#endif // IconGroup_h

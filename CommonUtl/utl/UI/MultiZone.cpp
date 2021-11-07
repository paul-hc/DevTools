
#include "stdafx.h"
#include "MultiZone.h"
#include "ComparePredicates.h"
#include "EnumTags.h"
#include "Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CMultiZone implementation

CMultiZone::CMultiZone( Stacking stacking /*= Auto*/ )
	: m_zoneSize( 0, 0 )
	, m_zoneCount( 0 )
	, m_stacking( stacking )
	, m_zoneSpacing( 0 )
{
}

CMultiZone::CMultiZone( const CSize& zoneSize, unsigned int zoneCount, Stacking stacking /*= Auto*/ )
	: m_zoneSize( zoneSize )
	, m_zoneCount( zoneCount )
	, m_stacking( m_zoneCount > 1 ? stacking : VertStacked )
	, m_zoneSpacing( 0 )
{
	ASSERT( IsValid() );
}

void CMultiZone::Init( const CSize& zoneSize, unsigned int zoneCount )
{
	m_zoneSize = zoneSize;
	m_zoneCount = zoneCount;
}

void CMultiZone::SetStacking( Stacking stacking )
{
	m_stacking = stacking;
	if ( IsValid() )
		AdjustStacking();
}
const CEnumTags& CMultiZone::GetTags_Stacking( void )
{
	static const CEnumTags s_tags( _T("Vertical|Horizontal|Auto") );
	return s_tags;
}

CMultiZone::Stacking CMultiZone::GetBestFitStacking( void ) const
{
	ASSERT( IsValid() );
	if ( 1 == m_zoneCount )
		return VertStacked;				// no stacking

	CSize vTotalSize = ComputeTotalSize( VertStacked ), hTotalSize = ComputeTotalSize( HorizStacked );
	double vDist = ui::GetDistFromSquareAspect( vTotalSize ), hDist = ui::GetDistFromSquareAspect( hTotalSize );

	return vDist < hDist ? VertStacked : HorizStacked;
}

CSize CMultiZone::ComputeTotalSize( Stacking stacking ) const
{
	ASSERT( IsValid() );

	CSize totalSize = m_zoneSize;
	switch ( stacking )
	{
		case Auto: ASSERT( false );
		case VertStacked:	totalSize.cy *= m_zoneCount; break;
		case HorizStacked:	totalSize.cx *= m_zoneCount; break;
	}
	return totalSize + ComputeSpacingSize( stacking );
}

CSize CMultiZone::ComputeSpacingSize( Stacking stacking ) const
{
	ASSERT( IsValid() );

	CSize spacingSize( 0, 0 );
	if ( m_zoneCount > 0 )
		switch ( stacking )
		{
			case Auto: ASSERT( false );
			case VertStacked:	spacingSize.cy += m_zoneSpacing * ( m_zoneCount - 1 ); break;
			case HorizStacked:	spacingSize.cx += m_zoneSpacing * ( m_zoneCount - 1 ); break;
		}

	return spacingSize;
}

void CMultiZone::AdjustStacking( void )
{
	ASSERT( IsValid() );
	if ( Auto == m_stacking )
		m_stacking = IsMultiple() ? GetBestFitStacking() : VertStacked;
}


// CMultiZoneIterator implementation

CMultiZoneIterator::CMultiZoneIterator( const CMultiZone& multiZone, const CRect& destRect )
	: CMultiZone( multiZone )
	, m_destRect( destRect )
	, m_iterPos( 0 )
{
	m_zoneSize = m_destRect.Size();
	switch ( m_stacking )
	{
		case Auto: ASSERT( false );
		case VertStacked: ShrinkZoneExtent( m_zoneSize.cy ); break;
		case HorizStacked: ShrinkZoneExtent( m_zoneSize.cx ); break;
	}
	m_zoneSize = ui::MaxSize( m_zoneSize, CSize( 1, 1 ) );
	ENSURE( IsValid() );
}

CMultiZoneIterator::CMultiZoneIterator( const CSize& zoneSize, int zoneSpacing, unsigned int zoneCount, Stacking stacking, const CRect& destRect )
	: CMultiZone( zoneSize, zoneCount, stacking )
	, m_destRect( destRect )
	, m_iterPos( 0 )
{
	ASSERT( m_stacking != Auto );
	ASSERT( IsValid() );
	m_zoneSpacing = zoneSpacing;
}

void CMultiZoneIterator::ShrinkZoneExtent( long& rExtent ) const
{
	rExtent -= m_zoneSpacing * ( m_zoneCount - 1 );
	rExtent = std::max< long >( 0, rExtent );					// safe for extreme m_zoneSpacing
	rExtent /= m_zoneCount;
}

CRect CMultiZoneIterator::GetZoneRect( unsigned int zonePos ) const
{
	ASSERT( IsValid() );
	CRect zoneRect( m_destRect.TopLeft(), m_zoneSize );		// exclude spacing
	zoneRect += GetZoneOffset( zonePos );					// include spacing
	return zoneRect;
}

CSize CMultiZoneIterator::GetZoneOffset( unsigned int zonePos ) const
{
	ASSERT( IsValid() );
	ASSERT( zonePos < m_zoneCount );

	CSize offset( 0, 0 );
	switch ( m_stacking )
	{
		case Auto: ASSERT( false );
		case VertStacked:
			offset.cy = ( m_zoneSize.cy + m_zoneSpacing ) * zonePos;
			break;
		case HorizStacked:
			offset.cx = ( m_zoneSize.cx + m_zoneSpacing ) * zonePos;
			break;
	}
	return offset;
}

namespace pred
{
	struct AsWidth { int operator()( const CSize& size ) const { return size.cx; } };

	typedef CompareAdapter< pred::CompareValue, AsWidth > TCompareWidth;
}

CSize CMultiZoneIterator::FindWidestSize( CDC* pDC, const std::vector< std::tstring >& labels ) const
{
	ASSERT( !labels.empty() );

	std::vector< CSize > textSizes;
	textSizes.reserve( m_zoneCount );
	for ( unsigned int i = 0; i != m_zoneCount; ++i )
		textSizes.push_back( ui::GetTextSize( pDC, labels[ i ].c_str() ) );

	return *std::max_element( textSizes.begin(), textSizes.end(), pred::LessValue< pred::TCompareWidth >() );
}

std::pair<CMultiZoneIterator::LabelLayout, UINT>
CMultiZoneIterator::FindLabelLayout( CDC* pDC, const CRect& clientRect, const std::vector< std::tstring >& labels ) const
{
	CSize labelSize = FindWidestSize( pDC, labels ) + CSize( TextSpacingX, TextSpacingY );

	if ( HorizStacked == m_stacking )
		return m_destRect.top - clientRect.top >= labelSize.cy
			? std::make_pair( Above, DT_CENTER | DT_BOTTOM )
			: std::make_pair( Inside, DT_RIGHT | DT_BOTTOM );

	if ( clientRect.right - m_destRect.right >= labelSize.cx )
		return std::make_pair( Outside, DT_LEFT | DT_TOP );

	return std::make_pair( Inside, DT_RIGHT | DT_BOTTOM );
}

CRect CMultiZoneIterator::MakeLabelRect( LabelLayout layout, const CRect& zoneRect, const CRect& clientRect ) const
{
	CRect textRect = zoneRect;

	switch ( layout )
	{
		case Outside:
			textRect.OffsetRect( zoneRect.Width(), 0 );
			textRect.right = clientRect.right;
			break;
		case Inside:
			break;
		case Above:
			textRect.bottom = textRect.top;
			textRect.top = clientRect.top;
			break;
	}
	textRect.DeflateRect( TextSpacingX, TextSpacingY );
	return textRect;
}

void CMultiZoneIterator::DrawLabels( CDC* pDC, const CRect& clientRect, const std::vector< std::tstring >& labels ) const
{
	ASSERT( IsValid() );
	ASSERT( m_zoneCount == labels.size() );

	std::pair<LabelLayout, UINT> layoutAlign = FindLabelLayout( pDC, clientRect, labels );		// find best fitting text layout

	for ( unsigned int i = 0; i != m_zoneCount; ++i )
	{
		CRect textRect = MakeLabelRect( layoutAlign.first, GetZoneRect( i ), clientRect );
		pDC->DrawText( labels[ i ].c_str(), (int)labels[ i ].length(), &textRect, DT_SINGLELINE | DT_NOPREFIX | DT_END_ELLIPSIS | layoutAlign.second );
	}
}

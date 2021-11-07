#ifndef MultiZone_h
#define MultiZone_h
#pragma once


class CEnumTags;


// enlarge zone to total size

struct CMultiZone
{
	enum Stacking { VertStacked, HorizStacked, Auto };
	static const CEnumTags& GetTags_Stacking( void );

	CMultiZone( Stacking stacking = Auto );
	CMultiZone( const CSize& zoneSize, unsigned int zoneCount, Stacking stacking = Auto );

	void Init( const CSize& zoneSize, unsigned int zoneCount );
	void Clear( void ) { Init( CSize( 0, 0 ), 0 ); }

	bool IsValid( void ) const { return m_zoneCount > 0 && m_zoneSize.cx > 0 && m_zoneSize.cy > 0; };
	bool IsMultiple( void ) const { ASSERT( IsValid() ); return m_zoneCount > 1; };

	unsigned int GetCount( void ) const { return m_zoneCount; }
	const CSize& GetZoneSize( void ) const { return m_zoneSize; }
	CSize GetTotalSize( void ) const { return ComputeTotalSize( EvalStacking() ); }
	CSize GetSpacingSize( void ) const { return ComputeSpacingSize( EvalStacking() ); }

	Stacking GetStacking( void ) const { return m_stacking; }
	void SetStacking( Stacking stacking );
	Stacking& RefStacking( void ) { return m_stacking; }

	Stacking EvalStacking( void ) const { return m_stacking != Auto ? m_stacking : GetBestFitStacking(); }
	Stacking GetBestFitStacking( void ) const;
	void AdjustStacking( void );
protected:
	CSize ComputeTotalSize( Stacking stacking ) const;
	CSize ComputeSpacingSize( Stacking stacking ) const;
protected:
	CSize m_zoneSize;
	unsigned int m_zoneCount;
	Stacking m_stacking;
public:
	int m_zoneSpacing;
};


struct CMultiZoneIterator : public CMultiZone
{
	// split destRect to zones
	CMultiZoneIterator( const CMultiZone& multiZone, const CRect& destRect );

	// as is (no down-scaling)
	CMultiZoneIterator( const CSize& zoneSize, int zoneSpacing, unsigned int zoneCount, Stacking stacking, const CRect& destRect );

	void Restart( void ) { m_iterPos = 0; }
	CRect GetNextZone( void ) { return GetZoneRect( m_iterPos++ ); }		// sequential access
	CRect GetZoneRect( unsigned int zonePos ) const;						// random access

	void DrawLabels( CDC* pDC, const CRect& clientRect, const std::vector< std::tstring >& labels ) const;
protected:
	void ShrinkZoneExtent( long& rExtent ) const;
	CSize GetZoneOffset( unsigned int zonePos ) const;
private:
	enum { TextSpacingX = 3, TextSpacingY = 2 };
	enum LabelLayout { Outside, Inside, Above };

	std::pair<LabelLayout, UINT> FindLabelLayout( CDC* pDC, const CRect& clientRect, const std::vector< std::tstring >& labels ) const;
	CRect MakeLabelRect( LabelLayout layout, const CRect& zoneRect, const CRect& clientRect ) const;
	CSize FindWidestSize( CDC* pDC, const std::vector< std::tstring >& labels ) const;
public:
	CRect m_destRect;
	int m_iterPos;
};


#endif // MultiZone_h

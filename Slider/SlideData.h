#ifndef SlideData_h
#define SlideData_h
#pragma once

#include "ListViewState.h"
#include "ImageNavigator_fwd.h"
#include "Album_fwd.h"


class CSlideData
{
public:
	CSlideData( void );
	~CSlideData();

	void Stream( CArchive& archive );

	const CListViewState& GetCurrListState( void ) const { return m_currListState; }
	CListViewState& RefCurrListState( void ) { m_imageFramePos = 0; return m_currListState; }

	int GetCurrentIndex( void ) const { return m_currListState.GetCaretIndex(); }
	bool SetCurrentIndex( int currIndex, bool resetListState = true );

	// index pair: aware of multi-frame images
	nav::TIndexFramePosPair GetCurrentNavPos( void ) const { return nav::TIndexFramePosPair( GetCurrentIndex(), m_imageFramePos ); }
	bool SetCurrentNavPos( const nav::TIndexFramePosPair& currentPos );

	// Perspective-dependent visibility flags
	bool HasShowFlag( af::AlbumFlags flag ) const { return ::HasFlag( const_cast< CSlideData* >( this )->RefShowFlags(), flag ); }
	void SetShowFlag( af::AlbumFlags flag, bool on = true ) { ::SetFlag( RefShowFlags(), flag, on ); }
	void ToggleShowFlag( af::AlbumFlags flag ) { ::ToggleFlag( RefShowFlags(), flag ); }

	UINT GetActualThumbListColumnCount( void ) const;
	void SetThumbListColumnCount( UINT thumbListColumnCount );
private:
	enum Perspective { Normal, FullScreen, _PerspectiveCount };

	af::TAlbumFlags& RefShowFlags( void );
public:
	// sliding
	persist UINT m_slideDelay;					// in miliseconds
	persist bool m_dirForward;
	persist bool m_wrapMode;

	persist bool m_saveCustomOrderUndoRedo;
private:
	// workspace-inherited data
	persist af::TAlbumFlags m_showFlags[ _PerspectiveCount ];			// flags per perspective (indexed by Perspective)
	persist UINT m_thumbListColumnCount;
private:
	persist CListViewState m_currListState;
	persist UINT m_imageFramePos;
};


#endif // SlideData_h

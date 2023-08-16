
#include "pch.h"
#include "SlideData.h"
#include "Workspace.h"
#include "utl/Serialization.h"
#include "utl/SerializeStdTypes.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CSlideData::CSlideData( void )
	: m_slideDelay( CWorkspace::Instance().GetDefaultSlideDelay() )
	, m_dirForward( true )
	, m_wrapMode( false )
	, m_currListState( StoreByIndex )
	, m_saveCustomOrderUndoRedo( HasFlag( CWorkspace::GetData().m_albumViewFlags, af::SaveCustomOrderUndoRedo ) )
	, m_thumbListColumnCount( CWorkspace::GetData().m_thumbListColumnCount )
	, m_imageFramePos( 0 )
{
	m_showFlags[ Normal ] = ( CWorkspace::GetData().m_albumViewFlags & af::ShowMask );
	m_showFlags[ FullScreen ] = 0;
}

CSlideData::~CSlideData()
{
}

void CSlideData::Stream( CArchive& archive )
{
	if ( archive.IsStoring() )
	{
		archive << m_slideDelay;
		archive & m_dirForward & m_wrapMode;
		archive & m_saveCustomOrderUndoRedo;
		archive << m_showFlags[ Normal ] << m_showFlags[ FullScreen ];
		archive << m_thumbListColumnCount;
		archive << m_imageFramePos;
	}
	else
	{	// check version backwards compatibility hack
		app::ModelSchema docModelSchema = app::GetLoadingSchema( archive );

		archive >> m_slideDelay;
		archive & m_dirForward & m_wrapMode;

		if ( docModelSchema >= app::Slider_v5_8 )
		{
			archive & m_saveCustomOrderUndoRedo;
			archive >> m_showFlags[ Normal ] >> m_showFlags[ FullScreen ];
		}
		else
		{
			int oldViewFlags;
			archive >> oldViewFlags;

			m_saveCustomOrderUndoRedo = ::HasFlag( oldViewFlags, af::SaveCustomOrderUndoRedo );
			m_showFlags[ Normal ] = ( oldViewFlags & af::ShowMask );
			m_showFlags[ FullScreen ] = 0;
		}

		archive >> m_thumbListColumnCount;

		if ( docModelSchema >= app::Slider_v5_1 )
			archive >> m_imageFramePos;
		else
			m_imageFramePos = 0;
	}
	m_currListState.Stream( archive );
}

bool CSlideData::SetCurrentIndex( int currIndex, bool resetListState /*= true*/ )
{
	bool currChanged = currIndex != m_currListState.GetCaretIndex();

	if ( resetListState || currChanged )
	{
		m_currListState.Clear();

		m_currListState.m_pIndexImpl->m_caret = currIndex;
		m_currListState.m_pIndexImpl->m_selItems.resize( 1 );
		m_currListState.m_pIndexImpl->m_selItems[ 0 ] = m_currListState.m_pIndexImpl->m_caret;
		m_currListState.m_pIndexImpl->m_top = LB_ERR;

		m_imageFramePos = 0;
	}
	return currChanged;
}

bool CSlideData::SetCurrentNavPos( const nav::TIndexFramePosPair& currentPos )
{
	bool changed = SetCurrentIndex( currentPos.first );

	if ( utl::ModifyValue( m_imageFramePos, currentPos.second ) )
		changed = true;
	return changed;
}


af::TAlbumFlags& CSlideData::RefShowFlags( void )
{
	return m_showFlags[ CWorkspace::Instance().IsFullScreen() ];
}

UINT CSlideData::GetActualThumbListColumnCount( void ) const
{
	ASSERT( m_thumbListColumnCount != 0 );

	if ( !HasShowFlag( af::ShowThumbView ) )
		return 0;			// will hide the thumb view

	return m_thumbListColumnCount;
}

void CSlideData::SetThumbListColumnCount( UINT thumbListColumnCount )
{
	ASSERT( m_thumbListColumnCount != 0 );		// to hide flip the af::ShowThumbView flag

	m_thumbListColumnCount = thumbListColumnCount;
}

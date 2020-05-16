
#include "stdafx.h"
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
	, m_viewFlags( CWorkspace::GetData().m_albumViewFlags )
	, m_thumbListColumnCount( CWorkspace::GetData().m_thumbListColumnCount )
	, m_imageFramePos( 0 )
{
}

CSlideData::~CSlideData()
{
}

void CSlideData::Stream( CArchive& archive, TFirstDataMember* pExtracted_SlideDelay /*= NULL*/ )
{
	if ( archive.IsStoring() )
	{
		archive << m_slideDelay;
		archive & m_dirForward & m_wrapMode;
		archive << m_viewFlags;
		archive << m_thumbListColumnCount;
		archive << m_imageFramePos;
	}
	else
	{	// check version backwards compatibility hack
		if ( pExtracted_SlideDelay != NULL )
			m_slideDelay = *pExtracted_SlideDelay;		// was already extracted from Slider_v3_1 old archive version
		else
			archive >> m_slideDelay;

		archive & m_dirForward & m_wrapMode;
		archive >> m_viewFlags;
		archive >> m_thumbListColumnCount;

		if ( app::GetLoadingSchema( archive ) >= app::Slider_v5_1 )
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

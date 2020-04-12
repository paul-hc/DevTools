
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
	, m_circular( false )
	, m_currListState( StoreByIndex )
	, m_viewFlags( CWorkspace::GetData().m_albumViewFlags )
	, m_thumbListColCount( CWorkspace::GetData().m_thumbListColCount )
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
		archive & m_dirForward & m_circular;
		archive << m_viewFlags;
		archive << m_thumbListColCount;
	}
	else
	{	// check version backwards compatibility hack
		if ( pExtracted_SlideDelay != NULL )
			m_slideDelay = *pExtracted_SlideDelay;		// was already extracted from Slider_v3_1 old archive version
		else
			archive >> m_slideDelay;

		archive & m_dirForward & m_circular;
		archive >> m_viewFlags;
		archive >> m_thumbListColCount;
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
	}
	return currChanged;
}

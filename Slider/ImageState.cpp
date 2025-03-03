
#include "pch.h"
#include "ImageState.h"
#include "ModelSchema.h"
#include "utl/Serialization.h"
#include "utl/SerializeStdTypes.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


const CPoint CImageState::s_noScrollPos( CW_USEDEFAULT, CW_USEDEFAULT );

CImageState::CImageState( void )
	: m_polyFlags( 0 )
	, m_scalingMode( ui::AutoFitLargeOnly )
	, m_zoomPct( 100 )
	, m_bkColor( CLR_DEFAULT )
	, m_scrollPos( CImageState::s_noScrollPos )
{
}

CImageState::~CImageState()
{
}

void CImageState::Stream( CArchive& archive )
{
	if ( archive.IsStoring() )
		archive << (int)app::Slider_LatestModelSchema;

	bool _stretchToFit = true, _shrinkIfLarger = true;		// dummy old flags

	if ( archive.IsStoring() )
	{
		archive << m_docFilePath;
		archive << m_framePlacement;

		archive << m_polyFlags;
		archive << m_zoomPct;
		archive & _stretchToFit & _shrinkIfLarger;			// bin compat
		archive << m_bkColor;
		archive << m_scrollPos;
		archive << m_scalingMode;
	}
	else
	{
		app::ModelSchema savedModelSchema;
		archive >> (int&)savedModelSchema;

		if ( savedModelSchema < app::Slider_LatestModelSchema )
			TRACE( _T("-- Converting CImageState from version %s to current version %s\n"),
				app::FormatSliderVersion( savedModelSchema ).c_str(), app::FormatSliderVersion( app::Slider_LatestModelSchema ).c_str() );

		archive >> m_docFilePath;
		archive >> m_framePlacement;

		archive >> m_polyFlags;
		archive >> m_zoomPct;
		archive & _stretchToFit & _shrinkIfLarger;		// bin compat; leave the default ui::AutoFitLargeOnly
		archive >> m_bkColor;
		archive >> m_scrollPos;

		if ( savedModelSchema >= app::Slider_v4_0 )
			archive >> (int&)m_scalingMode;
	}
}

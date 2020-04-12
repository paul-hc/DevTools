
#include "stdafx.h"
#include "StockValuesComboBox.h"
#include "IZoomBar.h"
#include "utl/StringUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "StockValuesComboBox.hxx"


namespace ui
{
	// CDurationInSecondsAdapter implementation

	const UINT CDurationInSecondsAdapter::s_defaultMiliseconds[] = { 100, 250, 500, 750, 1000, 1500, 2000, 3000, 4000, 5000, 8000, 10000, 12000, 15000, 17000, 20000, 25000, 30000 };

	CDurationInSecondsAdapter::CDurationInSecondsAdapter( void )
		: m_stockMiliseconds( s_defaultMiliseconds, END_OF( s_defaultMiliseconds ) )
	{
	}

	CDurationInSecondsAdapter::IAdapterBase* CDurationInSecondsAdapter::Instance( void )
	{
		static CDurationInSecondsAdapter s_durationAdapter;
		return &s_durationAdapter;
	}

	const std::vector< UINT >& CDurationInSecondsAdapter::GetStockValues( void ) const
	{
		return m_stockMiliseconds;
	}

	std::tstring CDurationInSecondsAdapter::OutputValue( const UINT& miliseconds ) const
	{
		return num::FormatNumber( ToDisplayValue( miliseconds ) ) + _T(" sec");
	}

	bool CDurationInSecondsAdapter::ParseValue( UINT* pOutMiliseconds, const std::tstring& text ) const
	{
		ASSERT_PTR( pOutMiliseconds );

		double seconds;
		if ( !num::ParseNumber( seconds, text ) )
			return false;			// invalid duration

		*pOutMiliseconds = FromDisplayValue( seconds );
		return true;
	}

	double CDurationInSecondsAdapter::ToDisplayValue( const UINT& miliseconds ) const
	{
		return static_cast< double >( miliseconds ) / 1000.0;
	}

	UINT CDurationInSecondsAdapter::FromDisplayValue( const double& seconds ) const
	{
		return static_cast< UINT >( seconds * 1000.0 );
	}


	// CPercentageAdapterBase implementation

	std::tstring CPercentageAdapterBase::OutputValue( const UINT& zoomPct ) const
	{
		return str::Format( _T("%d %%"), zoomPct );
	}

	bool CPercentageAdapterBase::ParseValue( UINT* pOutZoomPct, const std::tstring& text ) const
	{
		ASSERT_PTR( pOutZoomPct );

		return num::ParseNumber( *pOutZoomPct, text );
	}


	// CZoomPercentageAdapter implementation

	CZoomPercentageAdapter::IAdapterBase* CZoomPercentageAdapter::Instance( void )
	{
		static CZoomPercentageAdapter s_zoomPercentageAdapter;
		return &s_zoomPercentageAdapter;
	}

	const std::vector< UINT >& CZoomPercentageAdapter::GetStockValues( void ) const
	{
		return ui::CStdZoom::Instance().m_zoomPcts;
	}
}


// CDurationComboBox implementation

CDurationComboBox::CDurationComboBox( ui::TValueSetFlags flags /*= ui::LimitMinValue*/, const ui::IValueSetAdapter< UINT >* pStockAdapter /*= ui::CDurationInSecondsAdapter::Instance()*/ )
	: CStockValuesComboBox< UINT >( pStockAdapter, flags )
{
}


// CZoomComboBox implementation

CZoomComboBox::CZoomComboBox( ui::TValueSetFlags flags /*= ui::LimitMinValue | ui::LimitMaxValue*/,
							  const ui::IValueSetAdapter< UINT >* pStockAdapter /*= ui::CZoomPercentageAdapter::Instance()*/ )
	: CStockValuesComboBox< UINT >( pStockAdapter, flags )
{
	SetValidRange( ui::CStdZoom::Instance().m_limits );
}

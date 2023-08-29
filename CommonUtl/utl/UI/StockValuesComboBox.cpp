
#include "pch.h"
#include "StockValuesComboBox.h"
#include "IZoomBar.h"
#include "WndUtils.h"
#include "utl/StringUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CStockValuesComboBox implementation

CStockValuesComboBox::CStockValuesComboBox( const ui::IStockTags* pStockTags )
	: CBaseStockContentCtrl<CComboBox>()
	, m_pStockTags( nullptr )
{
	SetTags( pStockTags );
}

void CStockValuesComboBox::SetTags( const ui::IStockTags* pStockTags )
{
	m_pStockTags = pStockTags;

	if ( m_hWnd != nullptr )
		InitStockContent();
}

void CStockValuesComboBox::InitStockContent( void ) override
{
	std::vector<std::tstring> tags;
	m_pStockTags->QueryStockTags( tags );

	ui::WriteComboItems( *this, tags );
}

bool CStockValuesComboBox::OutputTag( const std::tstring& tag )
{
	bool changed = ui::SetComboEditText( *this, tag, str::Case ).first;

	if ( ui::OwnsFocus( m_hWnd ) )
		SetEditSel( 0, -1 );

	return changed;
}

void CStockValuesComboBox::OnInputError( void ) const
{
	ui::PostCommandToParent( m_hWnd, ui::CN_INPUTERROR );		// give parent a chance to restore previous valid value - posted since notifications are disabled during DDX
}


// CDurationComboBox implementation

CDurationComboBox::CDurationComboBox( const ui::IStockTags* pStockTags /*= ui::CDurationSecondsStockTags::Instance()*/ )
	: CStockValuesComboBox( pStockTags )
{
}

bool CDurationComboBox::OutputMiliseconds( UINT durationMs )
{
	return OutputValue( ui::CDurationSecondsStockTags::FromMiliseconds( durationMs ) );
}

bool CDurationComboBox::InputMiliseconds( OUT UINT* pDurationMs, ui::ComboField byField, bool showErrors /*= false*/ ) const
{
	ASSERT_PTR( pDurationMs );
	double durationSecs;

	if ( !InputValue( &durationSecs, byField, showErrors ) )
		return false;

	*pDurationMs = ui::CDurationSecondsStockTags::ToMiliseconds( durationSecs );
	return true;
}

void CDurationComboBox::DDX_Miliseconds( CDataExchange* pDX, IN OUT UINT& rDurationMs, int comboId )
{
	double durationSecs = ui::CDurationSecondsStockTags::FromMiliseconds( rDurationMs );

	DDX_Value( pDX, durationSecs, comboId );

	if ( DialogSaveChanges == pDX->m_bSaveAndValidate )
		rDurationMs = ui::CDurationSecondsStockTags::ToMiliseconds( durationSecs );
}


// CZoomComboBox implementation

CZoomComboBox::CZoomComboBox( const ui::IStockTags* pStockTags /*= ui::CZoomStockTags::Instance()*/ )
	: CStockValuesComboBox( pStockTags )
{
}

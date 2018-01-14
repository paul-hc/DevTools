
#include "stdafx.h"
#include "EditIdentPage.h"
#include "AppService.h"
#include "wnd/ValueRepository.h"
#include "utl/Utilities.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace layout
{
	static const CLayoutStyle styles[] =
	{
		{ IDC_IDENT_LITERAL_COMBO, StretchX }
	};
}


CEditIdentPage::CEditIdentPage( void )
	: CDetailBasePage( IDD_EDIT_IDENT_PAGE )
	, m_id( 0 )
	, m_decIdentEdit( num::Decimal )
	, m_hexIdentEdit( num::Hexa )
{
	RegisterCtrlLayout( layout::styles, COUNT_OF( layout::styles ) );
}

CEditIdentPage::~CEditIdentPage()
{
}

bool CEditIdentPage::UseWndId( void ) const
{
	CWndSpot* pTargetWnd = app::GetValidTargetWnd();
	return pTargetWnd != NULL && HasFlag( pTargetWnd->GetStyle(), WS_CHILD );
}

bool CEditIdentPage::IsDirty( void ) const
{
	if ( CWndSpot* pTargetWnd = app::GetValidTargetWnd() )
		if ( HasFlag( pTargetWnd->GetStyle(), WS_CHILD ) )
			if ( m_id != pTargetWnd->GetDlgCtrlID() )
				return true;

	return false;
}

void CEditIdentPage::OnTargetWndChanged( const CWndSpot& targetWnd )
{
	bool valid = targetWnd.IsValid();
	ui::EnableWindow( *this, valid );

	bool validId = valid && HasFlag( targetWnd.GetStyle(), WS_CHILD );

	ui::SetWindowText( *this, validId ? _T("Identifier") : _T("Menu") );

	static const UINT decCtrlIds[] = { IDC_IDENT_DEC_EDIT, IDC_IDENT_HEX_EDIT, IDC_IDENT_LITERAL_COMBO };

	ui::EnableControls( m_hWnd, decCtrlIds, COUNT_OF( decCtrlIds ), validId );

	if ( validId )
	{
		m_id = static_cast< short >( targetWnd.GetDlgCtrlID() );
		m_decIdentEdit.SetValue( m_id );
		m_hexIdentEdit.SetValue( m_id );
		m_literalCombo.SetCurSel( FindLiteralPos( m_id ) );
	}
	else
	{
		m_id = 0;
		m_decIdentEdit.SetNull();
		m_hexIdentEdit.SetNull();
		m_literalCombo.SetCurSel( CB_ERR );
	}
	SetModified( false );
}

void CEditIdentPage::ApplyPageChanges( void ) throws_( CRuntimeException )
{
	if ( CWndSpot* pTargetWnd = app::GetValidTargetWnd() )
		if ( UseWndId() )
			if ( m_decIdentEdit.GetValue( &m_id ) )
				pTargetWnd->SetDlgCtrlID( m_id );
}

int CEditIdentPage::FindLiteralPos( int id ) const
{
	for ( int i = 0, count = m_literalCombo.GetCount(); i != count; ++i )
		if ( const CValueInfo* pValueInfo = reinterpret_cast< const CValueInfo* >( m_literalCombo.GetItemDataPtr( i ) ) )
			if ( pValueInfo->m_value == id )
				return i;

	return CB_ERR;
}

void CEditIdentPage::DoDataExchange( CDataExchange* pDX )
{
	bool firstInit = NULL == m_literalCombo.m_hWnd;
	DDX_Control( pDX, IDC_IDENT_DEC_EDIT, m_decIdentEdit );
	DDX_Control( pDX, IDC_IDENT_HEX_EDIT, m_hexIdentEdit );
	DDX_Control( pDX, IDC_IDENT_LITERAL_COMBO, m_literalCombo );

	if ( firstInit )
	{
		ui::SetSpinRange( this, IDC_IDENT_DEC_SPIN, SHRT_MIN, SHRT_MAX );

		const std::vector< CValueStore* >& idStores = CIdentRepository::Instance().m_stores;
		for ( std::vector< CValueStore* >::const_iterator itStore = idStores.begin(); itStore != idStores.end(); ++itStore )
			for ( std::vector< const CValueInfo* >::const_iterator itValue = ( *itStore )->GetValues().begin();
				  itValue != ( *itStore )->GetValues().end(); ++itValue )
			{
				int pos = m_literalCombo.AddString( ( *itValue )->Format().c_str() );
				m_literalCombo.SetItemDataPtr( pos, (void*)*itValue );
			}
	}

	if ( DialogOutput == pDX->m_bSaveAndValidate )
		OutputTargetWnd();

	CDetailBasePage::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CEditIdentPage, CDetailBasePage )
	ON_EN_CHANGE( IDC_IDENT_DEC_EDIT, OnEnChange_DecimalEdit )
	ON_EN_CHANGE( IDC_IDENT_HEX_EDIT, OnEnChange_HexEdit )
	ON_CBN_SELCHANGE( IDC_IDENT_LITERAL_COMBO, OnCbnSelChange_Literal )
END_MESSAGE_MAP()

void CEditIdentPage::OnEnChange_DecimalEdit( void )
{
	if ( m_decIdentEdit.m_hWnd != NULL )			// subclassed
		if ( app::GetValidTargetWnd() != NULL )
			if ( UseWndId() )
				if ( m_decIdentEdit.GetValue( &m_id ) )
				{
					m_hexIdentEdit.SetValue( m_id );
					m_literalCombo.SetCurSel( FindLiteralPos( m_id ) );

					OnFieldModified();
				}
}

void CEditIdentPage::OnEnChange_HexEdit( void )
{
	if ( app::GetValidTargetWnd() != NULL )
		if ( UseWndId() )
		{
			unsigned short id;
			if ( m_hexIdentEdit.GetValue( &id ) )
			{
				m_id = static_cast< short >( id );
				m_decIdentEdit.SetValue( m_id );
				m_literalCombo.SetCurSel( FindLiteralPos( m_id ) );

				OnFieldModified();
			}
		}
}

void CEditIdentPage::OnCbnSelChange_Literal( void )
{
	if ( app::GetValidTargetWnd() != NULL )
		if ( UseWndId() )
		{
			int selIndex = m_literalCombo.GetCurSel();
			if ( selIndex != CB_ERR )
				if ( const CValueInfo* pValueInfo = reinterpret_cast< const CValueInfo* >( m_literalCombo.GetItemDataPtr( selIndex ) ) )
				{
					m_id = pValueInfo->m_value;
					m_decIdentEdit.SetValue( m_id );
					m_hexIdentEdit.SetValue( m_id );

					OnFieldModified();
				}
		}
}

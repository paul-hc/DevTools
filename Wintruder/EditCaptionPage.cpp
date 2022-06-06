
#include "stdafx.h"
#include "EditCaptionPage.h"
#include "PromptDialog.h"
#include "AppService.h"
#include "resource.h"
#include "wnd/WindowClass.h"
#include "wnd/WndUtils.h"
#include "utl/UI/WndUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace layout
{
	static const CLayoutStyle styles[] =
	{
		{ IDC_EDIT_CAPTION_EDIT, Size }
	};
}


CEditCaptionPage::CEditCaptionPage( void )
	: CDetailBasePage( IDD_EDIT_CAPTION_PAGE )
	, m_contentType( wc::CaptionText )
{
	RegisterCtrlLayout( ARRAY_PAIR( layout::styles ) );
}

CEditCaptionPage::~CEditCaptionPage()
{
}

bool CEditCaptionPage::IsDirty( void ) const
{
	if ( CWndSpot* pTargetWnd = app::GetValidTargetWnd() )
		if ( wc::CaptionText == m_contentType )
			if ( m_textContent != wnd::GetWindowText( *pTargetWnd ) )
				return true;

	return false;
}

void CEditCaptionPage::OnTargetWndChanged( const CWndSpot& targetWnd )
{
	bool valid = targetWnd.IsValid();
	ui::EnableWindow( *this, valid );

	std::tostringstream oss;
	m_contentType = valid ? wc::FormatTextContent( oss, targetWnd.GetWnd() ) : wc::CaptionText;
	m_textContent = oss.str();

	m_contentEdit.SetWritable( wc::CaptionText == m_contentType );
	m_contentEdit.SetText( m_textContent );
	SetModified( FALSE );
}

void CEditCaptionPage::ApplyPageChanges( void ) throws_( CRuntimeException )
{
	if ( CWndSpot* pTargetWnd = app::GetValidTargetWnd() )
		if ( m_contentEdit.GetModify() && wc::CaptionText == m_contentType )
		{
			m_textContent = m_contentEdit.GetText();
			ui::SetWindowText( *pTargetWnd, m_textContent );
		}
}

void CEditCaptionPage::DoDataExchange( CDataExchange* pDX )
{
	DDX_Control( pDX, IDC_EDIT_CAPTION_EDIT, m_contentEdit );

	if ( DialogOutput == pDX->m_bSaveAndValidate )
		OutputTargetWnd();

	CDetailBasePage::DoDataExchange( pDX );
}


// message handlers

BEGIN_MESSAGE_MAP( CEditCaptionPage, CDetailBasePage )
	ON_EN_CHANGE( IDC_EDIT_CAPTION_EDIT, OnEnChange_Caption )
END_MESSAGE_MAP()

void CEditCaptionPage::OnEnChange_Caption( void )
{
	if ( wc::CaptionText == m_contentType )
	{
		m_textContent = wnd::GetWindowText( m_contentEdit );
		OnFieldModified();
	}
}

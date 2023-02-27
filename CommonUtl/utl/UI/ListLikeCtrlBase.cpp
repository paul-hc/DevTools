
#include "stdafx.h"
#include "ListLikeCtrlBase.h"
#include "CustomDrawImager.h"
#include "CmdIdStore.h"
#include "WndUtils.h"
#include "VisualTheme.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CListLikeCtrlBase implementation

CListLikeCtrlBase::CListLikeCtrlBase( CWnd* pCtrl, UINT ctrlAccelId /*= 0*/ )
	: CObjectCtrlBase( pCtrl, ctrlAccelId )
	, m_useExplorerTheme( true )
	, m_pTextEffectCallback( nullptr )
{
	CFileItemsThumbnailStore::Instance().RegisterControl( this );
}

CListLikeCtrlBase::~CListLikeCtrlBase()
{
	CFileItemsThumbnailStore::Instance().UnregisterControl( this );
}

void CListLikeCtrlBase::SetUseExplorerTheme( bool useExplorerTheme /*= true*/ )
{
	m_useExplorerTheme = useExplorerTheme;

	if ( m_pCtrl->m_hWnd != nullptr )
		CVisualTheme::SetWindowTheme( m_pCtrl->m_hWnd, GetUseExplorerTheme() ? L"Explorer" : L"", nullptr );		// enable Explorer vs classic theme
}

CBaseCustomDrawImager* CListLikeCtrlBase::GetCustomDrawImager( void ) const
{
	return m_pCustomImager.get();
}

void CListLikeCtrlBase::SetCustomFileGlyphDraw( bool showGlyphs /*= true*/ )
{
	m_pCustomImager.reset( showGlyphs ? new CFileGlyphCustomDrawImager( ui::SmallGlyph ) : nullptr );
}

void CListLikeCtrlBase::SetupControl( void )
{
	if ( GetUseExplorerTheme() )
		CVisualTheme::SetWindowTheme( m_pCtrl->m_hWnd, L"Explorer", nullptr );		// enable Explorer theme
}

ui::CFontEffectCache* CListLikeCtrlBase::GetFontEffectCache( void )
{
	if ( nullptr == m_pFontCache.get() )
		m_pFontCache.reset( new ui::CFontEffectCache( m_pCtrl->GetFont() ) );
	return m_pFontCache.get();
}

void CListLikeCtrlBase::CombineTextEffectAt( ui::CTextEffect& rTextEffect, LPARAM rowKey, int subItem, CListLikeCtrlBase* pCtrl ) const
{
	ASSERT( this == pCtrl );
	if ( m_pTextEffectCallback != nullptr )
		m_pTextEffectCallback->CombineTextEffectAt( rTextEffect, rowKey, subItem, pCtrl );
}

void CListLikeCtrlBase::ModifyDiffTextEffectAt( lv::CMatchEffects& rEffects, LPARAM rowKey, int subItem, CReportListControl* pCtrl ) const
{
	//ASSERT( this == pCtrl );		// for this check: don't bother to #include "ReportListControl.h"
	if ( m_pTextEffectCallback != nullptr )
		m_pTextEffectCallback->ModifyDiffTextEffectAt( rEffects, rowKey, subItem, pCtrl );
}

bool CListLikeCtrlBase::ParentHandles( UINT cmdMessage, UINT notifyCode )
{
	if ( nullptr == m_pParentHandlesCache.get() )
		m_pParentHandlesCache.reset( new ui::CHandledNotificationsCache( m_pCtrl->GetParent() ) );		// lazy instantiation so that list parent is available

	return m_pParentHandlesCache->HandlesMessage( m_pCtrl->GetDlgCtrlID(), cmdMessage, notifyCode );
}


// CListLikeCustomDrawBase implementation

bool CListLikeCustomDrawBase::s_useDefaultDraw = false;
bool CListLikeCustomDrawBase::s_dbgGuides = false;

CListLikeCustomDrawBase::CListLikeCustomDrawBase( NMCUSTOMDRAW* pDraw )
	: m_pDraw( safe_ptr( pDraw ) )
	, m_pDC( CDC::FromHandle( m_pDraw->hdc ) )
{
}

bool CListLikeCustomDrawBase::IsTooltipDraw( const NMCUSTOMDRAW* pDraw )
{
	ASSERT_PTR( pDraw );
	static const CRect s_emptyRect( 0, 0, 0, 0 );
	if ( s_emptyRect == pDraw->rc )
		return true;						// tooltip custom draw
	return false;
}


#ifdef _DEBUG
#include "EnumTags.h"
#include "FlagTags.h"
#include "StringUtilities.h"
#endif //_DEBUG

namespace dbg
{
	const TCHAR* FormatDrawStage( DWORD dwDrawStage )
	{
	#ifdef _DEBUG
		static const CEnumTags enumTags( _T("CDDS_PREPAINT|CDDS_POSTPAINT|CDDS_PREERASE|CDDS_POSTERASE"), nullptr, -1, CDDS_PREPAINT );		// mask 0x0000000F
		static const CFlagTags::FlagDef flagDefs[] =
		{
			{ CDDS_ITEM, _T("CDDS_ITEM") },
			{ CDDS_SUBITEM, _T("CDDS_SUBITEM") }
		};
		static const CFlagTags flagTags( flagDefs, COUNT_OF( flagDefs ) );
		static const TCHAR sep[] = _T(" | ");

		static std::tstring s_text;
		s_text = enumTags.FormatUi( dwDrawStage & 0x0000000F );
		stream::Tag( s_text, flagTags.FormatKey( dwDrawStage, sep ), sep );

		return s_text.c_str();
	#else
		dwDrawStage;
		return _T("");
	#endif //_DEBUG
	}
}


#include "stdafx.h"
#include "DetailBasePage.h"
#include "AppService.h"
#include "utl/LayoutBasePropertySheet.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CDetailBasePage::CDetailBasePage( UINT templateId )
	: CLayoutPropertyPage( templateId )
{
	app::GetSvc().AddObserver( this );
}

CDetailBasePage::~CDetailBasePage()
{
	app::GetSvc().RemoveObserver( this );
}

void CDetailBasePage::SetModified( bool changed /*= true*/ )
{
	CLayoutPropertyPage::SetModified( changed );

	if ( changed )
		app::GetSvc().SetDirtyDetails();

	if ( !m_pageTitle.empty() )
	{
		CLayoutBasePropertySheet* pSheet = GetParentSheet();
		int pageIndex = pSheet->GetPageIndex( this );
		if ( pageIndex != -1 )
		{
			std::tstring pageTitle = m_pageTitle;
			if ( changed )
				pageTitle += _T(" *");

			pSheet->SetPageTitle( pageIndex, pageTitle );
		}
	}
}

void CDetailBasePage::DoDataExchange( CDataExchange* pDX )
{
	if ( DialogOutput == pDX->m_bSaveAndValidate )
		if ( m_pageTitle.empty() )
		{
			CLayoutBasePropertySheet* pSheet = GetParentSheet();
			int pageIndex = pSheet->GetPageIndex( this );
			if ( pageIndex != -1 )
				m_pageTitle = pSheet->GetPageTitle( pageIndex );
		}

	CLayoutPropertyPage::DoDataExchange( pDX );
}

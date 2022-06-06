
#include "stdafx.h"
#include "TestMiscDialog.h"
#include "utl/AppTools.h"
#include "utl/UI/ImageStore.h"
#include "utl/UI/WndUtils.h"
#include "utl/UI/resource.h"
#include "resource.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace reg
{
	static const TCHAR section_dialog[] = _T("MiscDialog");
}

namespace layout
{
	static CLayoutStyle styles[] =
	{
		{ IDOK, Move }
	};
}


CTestMiscDialog::CTestMiscDialog( CWnd* pParent )
	: CLayoutDialog( IDD_TEST_MISC_DIALOG, pParent )
	, m_toolbarDisabledGrayScale( gdi::DisabledGrayScale )
	, m_toolbarDisabledGray( gdi::DisabledGrayOut )
	, m_toolbarDisabledEffect( gdi::DisabledEffect )
	, m_toolbarDisabledBlendColor( gdi::DisabledBlendColor )
{
	m_regSection = reg::section_dialog;
	RegisterCtrlLayout( ARRAY_PAIR( layout::styles ) );

	RegisterOwnCmds();

	static const UINT s_stdButtons[] = { IdFileNew, IdFileOpen, IdFileSave, 0, IdEditCut, IdEditCopy, IdEditPaste, 0, IdFilePrint, 0, IdAppAbout };
	m_toolbarStdEnabled.GetStrip().AddButtons( ARRAY_PAIR( s_stdButtons ) );
	m_toolbarStdDisabled.GetStrip().AddButtons( ARRAY_PAIR( s_stdButtons ) );

	m_toolbarDisabledGrayScale.GetStrip().AddButtons( ARRAY_PAIR( s_stdButtons ) );
	m_toolbarDisabledGray.GetStrip().AddButtons( ARRAY_PAIR( s_stdButtons ) );
	m_toolbarDisabledEffect.GetStrip().AddButtons( ARRAY_PAIR( s_stdButtons ) );
	m_toolbarDisabledBlendColor.GetStrip().AddButtons( ARRAY_PAIR( s_stdButtons ) );
}

CTestMiscDialog::~CTestMiscDialog()
{
}

void CTestMiscDialog::RegisterOwnCmds( void )
{
	static bool registered = false;
	if ( registered )
		return;

	static const CImageStore::CCmdAlias s_aliases[] =
	{
		{ IdFileNew, ID_FILE_NEW },
		{ IdFileOpen, ID_FILE_OPEN },
		{ IdFileSave, ID_FILE_SAVE },
		{ IdEditCut, ID_EDIT_CUT },
		{ IdEditCopy, ID_EDIT_COPY },
		{ IdEditPaste, ID_EDIT_PASTE },
		{ IdFilePrint, ID_FILE_PRINT },
		{ IdAppAbout, ID_APP_ABOUT }
	};
	app::GetSharedImageStore()->RegisterAliases( ARRAY_PAIR( s_aliases ) );
	registered = true;
}

void CTestMiscDialog::DoDataExchange( CDataExchange* pDX )
{
//	bool firstInit = NULL == m_toolbarStdEnabled.m_hWnd;
	enum Align { BarAlign = H_AlignLeft | V_AlignCenter };

	m_toolbarStdEnabled.DDX_Placeholder( pDX, IDC_TOOLBAR_PLACEHOLDER, BarAlign );
	m_toolbarStdDisabled.DDX_Placeholder( pDX, IDC_STRIP_BAR_1, BarAlign );

	m_toolbarDisabledGrayScale.DDX_Placeholder( pDX, IDC_STRIP_BAR_2, BarAlign );
	m_toolbarDisabledGray.DDX_Placeholder( pDX, IDC_STRIP_BAR_3, BarAlign );
	m_toolbarDisabledEffect.DDX_Placeholder( pDX, IDC_STRIP_BAR_4, BarAlign );
	m_toolbarDisabledBlendColor.DDX_Placeholder( pDX, IDC_STRIP_BAR_5, BarAlign );

	CLayoutDialog::DoDataExchange( pDX );
}

BEGIN_MESSAGE_MAP( CTestMiscDialog, CLayoutDialog )
	ON_WM_DESTROY()
	ON_UPDATE_COMMAND_UI_RANGE( IdFileNew, IdAppAbout, OnUpdateButton )
END_MESSAGE_MAP()

void CTestMiscDialog::OnDestroy( void )
{
	//AfxGetApp()->WriteProfileInt( reg::section_dialog, reg::entry_dialogUsage, m_usageButton.GetSelValue() );

	__super::OnDestroy();
}

void CTestMiscDialog::OnUpdateButton( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( &m_toolbarStdEnabled == pCmdUI->m_pOther );
}

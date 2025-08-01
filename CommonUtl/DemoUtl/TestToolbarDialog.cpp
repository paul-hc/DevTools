
#include "pch.h"
#include "TestToolbarDialog.h"
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


CTestToolbarDialog::CTestToolbarDialog( CWnd* pParent )
	: CLayoutDialog( IDD_TEST_TOOLBAR_DIALOG, pParent )
	, m_toolbarDis_FadeGray( gdi::Dis_FadeGray )
	, m_toolbarDis_GrayScale( gdi::Dis_GrayScale )
	, m_toolbarDis_GrayOut( gdi::Dis_GrayOut )
	, m_toolbarDis_Effect( gdi::Dis_DisabledEffect )
	, m_toolbarDis_BlendColor( gdi::Dis_BlendColor )
{
	m_regSection = reg::section_dialog;
	RegisterCtrlLayout( ARRAY_SPAN( layout::styles ) );

	RegisterOwnCmds();

	static const UINT s_stdButtons[] = { IdFileNew, IdFileOpen, IdFileSave, 0, IdEditCut, IdEditCopy, IdEditPaste, 0, IdFilePrint, 0, IdAppAbout };

	m_toolbarStdEnabled.GetStrip().AddButtons( ARRAY_SPAN( s_stdButtons ) );
	m_toolbarStdDisabled.GetStrip().AddButtons( ARRAY_SPAN( s_stdButtons ) );

	m_toolbarDis_FadeGray.GetStrip().AddButtons( ARRAY_SPAN( s_stdButtons ) );
	m_toolbarDis_GrayScale.GetStrip().AddButtons( ARRAY_SPAN( s_stdButtons ) );
	m_toolbarDis_GrayOut.GetStrip().AddButtons( ARRAY_SPAN( s_stdButtons ) );
	m_toolbarDis_Effect.GetStrip().AddButtons( ARRAY_SPAN( s_stdButtons ) );
	m_toolbarDis_BlendColor.GetStrip().AddButtons( ARRAY_SPAN( s_stdButtons ) );
}

CTestToolbarDialog::~CTestToolbarDialog()
{
}

void CTestToolbarDialog::RegisterOwnCmds( void )
{
	static bool registered = false;
	if ( registered )
		return;

	static const ui::CCmdAlias s_aliases[] =
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
	app::GetSharedImageStore()->RegisterAliases( ARRAY_SPAN( s_aliases ) );
	registered = true;
}

void CTestToolbarDialog::DoDataExchange( CDataExchange* pDX )
{
//	bool firstInit = nullptr == m_toolbarStdEnabled.m_hWnd;
	enum Align { BarAlign = H_AlignLeft | V_AlignCenter };

	m_toolbarStdEnabled.DDX_Placeholder( pDX, IDC_TOOLBAR_PLACEHOLDER, BarAlign );
	m_toolbarStdDisabled.DDX_Placeholder( pDX, IDC_STRIP_BAR_1, BarAlign );

	m_toolbarDis_FadeGray.DDX_Placeholder( pDX, IDC_STRIP_BAR_2, BarAlign );
	m_toolbarDis_GrayScale.DDX_Placeholder( pDX, IDC_STRIP_BAR_3, BarAlign );
	m_toolbarDis_GrayOut.DDX_Placeholder( pDX, IDC_STRIP_BAR_4, BarAlign );
	m_toolbarDis_Effect.DDX_Placeholder( pDX, IDC_STRIP_BAR_5, BarAlign );
	m_toolbarDis_BlendColor.DDX_Placeholder( pDX, IDC_STRIP_BAR_6, BarAlign );

	__super::DoDataExchange( pDX );
}

BEGIN_MESSAGE_MAP( CTestToolbarDialog, CLayoutDialog )
	ON_WM_DESTROY()
	ON_UPDATE_COMMAND_UI_RANGE( IdFileNew, IdAppAbout, OnUpdateButton )
END_MESSAGE_MAP()

void CTestToolbarDialog::OnDestroy( void )
{
	//AfxGetApp()->WriteProfileInt( reg::section_dialog, reg::entry_dialogUsage, m_usageButton.GetSelValue() );

	__super::OnDestroy();
}

void CTestToolbarDialog::OnUpdateButton( CCmdUI* pCmdUI )
{
	pCmdUI->Enable( &m_toolbarStdEnabled == pCmdUI->m_pOther );		// enable all buttons in only StdEnabled toolbar
}

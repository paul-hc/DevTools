
#include "stdafx.h"
#include "ModuleSession.h"
#include "IdeUtilities.h"
#include "OptionsSheet.h"
#include "Application.h"
#include "resource.h"
#include "utl/Path.h"
#include "utl/Registry.h"
#include "utl/StringUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace reg
{
	const TCHAR section_settings[] = _T("Settings");
	const TCHAR entry_developerName[] = _T("Developer name");
	const TCHAR entry_codeTemplateFile[] = _T("Code template file");
	const TCHAR entry_splitMaxColumn[] = _T("Split max column");
	const TCHAR entry_menuVertSplitCount[] = _T("Menu vertical split count");
	const TCHAR entry_singleLineCommentToken[] = _T("Default comment token");

	const TCHAR entry_autoCodeGeneration[] = _T("Auto code generation");
	const TCHAR entry_displayErrorMessages[] = _T("Display error messages");
	const TCHAR entry_useCommentDecoration[] = _T("Auto comment decoration");
	const TCHAR entry_duplicateLineMoveDown[] = _T("DuplicateLine move down");
	const TCHAR entry_browseInfoPath[] = _T("BrowseInfo Path");

	const TCHAR section_settings_prefixes[] = _T("Settings\\Prefixes");
	const TCHAR entry_classPrefix[] = _T("Class prefix");
	const TCHAR entry_structPrefix[] = _T("Struct prefix");
	const TCHAR entry_enumPrefix[] = _T("Enum prefix");

	const TCHAR section_settings_Additional[] = _T("Settings\\IncludeOptions\\Additional");
	const TCHAR entry_additionalAssocFolders[] = _T("Associated folders");
}


IMPLEMENT_DYNCREATE( CModuleSession, CCmdTarget )

CModuleSession::CModuleSession( void )
	: CCmdTarget()
	, m_codeTemplatePath( GetDefaultCodeTemplatePath() )
	, m_splitMaxColumn( 140 )
	, m_menuVertSplitCount( 40 )
	, m_singleLineCommentToken( _T("#") )
	, m_autoCodeGeneration( true )
	, m_displayErrorMessages( true )
	, m_useCommentDecoration( false )
	, m_duplicateLineMoveDown( false )
	, m_classPrefix( _T("C") )
	, m_vsTabSizeCpp( 4 )
	, m_vsKeepTabsCpp( true )
	, m_vsUseStandardWindowsMenu( false )
	, m_moreAdditionalIncludePath( inc::AdditionalPath )
{
	TCHAR developerName[ 128 ];
	::GetEnvironmentVariable( _T("UserName"), developerName, COUNT_OF( developerName ) );
	m_developerName = developerName;
	if ( m_developerName.empty() )
		m_developerName = _T("<YourName>");

	fs::CPath mfcSrcFolderPath = ide::vs6::GetVC98DirPath();
	if ( !mfcSrcFolderPath.IsEmpty() )
		m_browseInfoPath = mfcSrcFolderPath / fs::CPath( _T("MFC\\SRC\\*.bsc|MFC") );
}

CModuleSession::~CModuleSession()
{
}

fs::CPath CModuleSession::GetDefaultCodeTemplatePath( void )
{
	static const std::tstring nameExt = _T("StdCodeTemplates.ctf");

	fs::CPath codeTemplatePath = app::GetDefaultConfigDirPath() / nameExt;

	if ( !codeTemplatePath.FileExist() )
		codeTemplatePath = ide::vs6::GetMacrosDirPath() / nameExt;

	return codeTemplatePath;
}

void CModuleSession::LoadFromRegistry( void )
{
	CWinApp* pApp = AfxGetApp();

	m_developerName = pApp->GetProfileString( reg::section_settings, reg::entry_developerName, m_developerName.c_str() );
	m_codeTemplatePath.Set( pApp->GetProfileString( reg::section_settings, reg::entry_codeTemplateFile, m_codeTemplatePath.GetPtr() ).GetString() );
	m_splitMaxColumn = pApp->GetProfileInt( reg::section_settings, reg::entry_splitMaxColumn, m_splitMaxColumn );
	m_menuVertSplitCount = pApp->GetProfileInt( reg::section_settings, reg::entry_menuVertSplitCount, m_menuVertSplitCount );
	m_menuVertSplitCount = std::max( m_menuVertSplitCount, 1u );
	m_singleLineCommentToken = pApp->GetProfileString( reg::section_settings, reg::entry_singleLineCommentToken, m_singleLineCommentToken.c_str() );

	m_autoCodeGeneration = pApp->GetProfileInt( reg::section_settings, reg::entry_autoCodeGeneration, m_autoCodeGeneration ) != FALSE;
	m_displayErrorMessages = pApp->GetProfileInt( reg::section_settings, reg::entry_displayErrorMessages, m_displayErrorMessages ) != FALSE;
	m_useCommentDecoration = pApp->GetProfileInt( reg::section_settings, reg::entry_useCommentDecoration, m_useCommentDecoration ) != FALSE;
	m_duplicateLineMoveDown = pApp->GetProfileInt( reg::section_settings, reg::entry_duplicateLineMoveDown, m_duplicateLineMoveDown ) != FALSE;

	m_browseInfoPath.Set( pApp->GetProfileString( reg::section_settings, reg::entry_browseInfoPath, m_browseInfoPath.GetPtr() ).GetString() );

	m_classPrefix = pApp->GetProfileString( reg::section_settings_prefixes, reg::entry_classPrefix, m_classPrefix.c_str() );
	m_structPrefix = pApp->GetProfileString( reg::section_settings_prefixes, reg::entry_structPrefix, m_structPrefix.c_str() );
	m_enumPrefix = pApp->GetProfileString( reg::section_settings_prefixes, reg::entry_enumPrefix, m_enumPrefix.c_str() );

	m_additionalAssocFolders = (LPCTSTR)pApp->GetProfileString( reg::section_settings_Additional, reg::entry_additionalAssocFolders, m_additionalAssocFolders.c_str() );

	{
		reg::CKey key;
		if ( key.Open( HKEY_CURRENT_USER, _T("Software\\Microsoft\\DevStudio\\6.0\\Text Editor\\Tabs/Language Settings\\C/C++"), KEY_READ ) )
		{
			m_vsTabSizeCpp = key.ReadNumberValue< int >( _T("TabSize"), m_vsTabSizeCpp );
			m_vsKeepTabsCpp = ( 0 == key.ReadNumberValue< int >( _T("InsertSpaces"), m_vsKeepTabsCpp ) );
		}
	}
	{
		reg::CKey key;
		if ( key.Open( HKEY_CURRENT_USER, _T("Software\\Microsoft\\DevStudio\\6.0\\General"), KEY_READ ) )
			m_vsUseStandardWindowsMenu = key.ReadNumberValue< int >( _T("TraditionalMenu"), m_vsUseStandardWindowsMenu ) != 0;
	}
}

void CModuleSession::SaveToRegistry( void ) const
{
	CWinApp* pApp = AfxGetApp();

	pApp->WriteProfileString( reg::section_settings, reg::entry_developerName, m_developerName.c_str() );
	pApp->WriteProfileString( reg::section_settings, reg::entry_codeTemplateFile, m_codeTemplatePath.GetPtr() );
	pApp->WriteProfileInt( reg::section_settings, reg::entry_splitMaxColumn, m_splitMaxColumn );
	pApp->WriteProfileInt( reg::section_settings, reg::entry_menuVertSplitCount, m_menuVertSplitCount );
	pApp->WriteProfileString( reg::section_settings, reg::entry_singleLineCommentToken, m_singleLineCommentToken.c_str() );

	pApp->WriteProfileInt( reg::section_settings, reg::entry_autoCodeGeneration, m_autoCodeGeneration );
	pApp->WriteProfileInt( reg::section_settings, reg::entry_displayErrorMessages, m_displayErrorMessages );
	pApp->WriteProfileInt( reg::section_settings, reg::entry_useCommentDecoration, m_useCommentDecoration );
	pApp->WriteProfileInt( reg::section_settings, reg::entry_duplicateLineMoveDown, m_duplicateLineMoveDown );

	// save it only when modified (options dialog)
//	if ( m_pCodeFormatterOptions.get() != NULL )
//		m_pCodeFormatterOptions->SaveToRegistry();

	pApp->WriteProfileString( reg::section_settings, reg::entry_browseInfoPath, m_browseInfoPath.GetPtr() );

	pApp->WriteProfileString( reg::section_settings_prefixes, reg::entry_classPrefix, m_classPrefix.c_str() );
	pApp->WriteProfileString( reg::section_settings_prefixes, reg::entry_structPrefix, m_structPrefix.c_str() );
	pApp->WriteProfileString( reg::section_settings_prefixes, reg::entry_enumPrefix, m_enumPrefix.c_str() );

	pApp->WriteProfileString( reg::section_settings_Additional, reg::entry_additionalAssocFolders, m_additionalAssocFolders.c_str() );
}

code::CFormatterOptions& CModuleSession::GetCodeFormatterOptions( void )
{
	if ( NULL == m_pCodeFormatterOptions.get() )
	{	// lazy creation/registry loading
		m_pCodeFormatterOptions.reset( new code::CFormatterOptions() );
		m_pCodeFormatterOptions->LoadFromRegistry();
	}

	return *m_pCodeFormatterOptions;
}

bool CModuleSession::EditOptions( void )
{
	ide::CScopedWindow scopedIDE;
	COptionsSheet sheet( scopedIDE.GetMainWnd() );

	CGeneralOptionsPage* pGeneralPage = sheet.GetPageAs<CGeneralOptionsPage>( COptionsSheet::GeneralPage );

	pGeneralPage->m_developerName = m_developerName;
	pGeneralPage->m_codeTemplatePath = m_codeTemplatePath;
	pGeneralPage->m_menuVertSplitCount = m_menuVertSplitCount;
	pGeneralPage->m_singleLineCommentToken = m_singleLineCommentToken;

	pGeneralPage->m_autoCodeGeneration = m_autoCodeGeneration;
	pGeneralPage->m_displayErrorMessages = m_displayErrorMessages;
	pGeneralPage->m_useCommentDecoration = m_useCommentDecoration;
	pGeneralPage->m_duplicateLineMoveDown = m_duplicateLineMoveDown;

	pGeneralPage->m_classPrefix = m_classPrefix.c_str();
	pGeneralPage->m_structPrefix = m_structPrefix.c_str();
	pGeneralPage->m_enumPrefix = m_enumPrefix.c_str();

	CCodingStandardPage* pCodingStandardPage = sheet.GetPageAs<CCodingStandardPage>( COptionsSheet::CodingStandardPage );

	pCodingStandardPage->m_splitMaxColumn = m_splitMaxColumn;

	CBscPathPage* pBscPathPage = sheet.GetPageAs<CBscPathPage>( COptionsSheet::BscPathPage );
	pBscPathPage->m_browseInfoPath = m_browseInfoPath;

	if ( sheet.DoModal() != IDOK )
		return FALSE;

	m_developerName = pGeneralPage->m_developerName;
	m_codeTemplatePath = pGeneralPage->m_codeTemplatePath;
	m_menuVertSplitCount = pGeneralPage->m_menuVertSplitCount;
	m_singleLineCommentToken = pGeneralPage->m_singleLineCommentToken;

	m_autoCodeGeneration = pGeneralPage->m_autoCodeGeneration;
	m_displayErrorMessages = pGeneralPage->m_displayErrorMessages;
	m_useCommentDecoration = pGeneralPage->m_useCommentDecoration;
	m_duplicateLineMoveDown = pGeneralPage->m_duplicateLineMoveDown;

	m_classPrefix = pGeneralPage->m_classPrefix;
	m_structPrefix = pGeneralPage->m_structPrefix;
	m_enumPrefix = pGeneralPage->m_enumPrefix;

	code::CFormatterOptions& formattingOptions = GetCodeFormatterOptions();

	formattingOptions.m_preserveMultipleWhiteSpace = pCodingStandardPage->m_preserveMultipleWhiteSpace;
	formattingOptions.m_deleteTrailingWhiteSpace = pCodingStandardPage->m_deleteTrailingWhiteSpace;
	formattingOptions.m_breakSeparators = pCodingStandardPage->m_breakSeparators;
	formattingOptions.m_braceRules = pCodingStandardPage->m_braceRules;
	formattingOptions.SetOperatorRules( pCodingStandardPage->m_operatorRules );
	m_splitMaxColumn = pCodingStandardPage->m_splitMaxColumn;

	CCppImplFormattingPage* pCppFormattingPage = sheet.GetPageAs<CCppImplFormattingPage>( COptionsSheet::CppFormattingPage );
	formattingOptions.m_returnTypeOnSeparateLine = pCppFormattingPage->m_returnTypeOnSeparateLine;
	formattingOptions.m_commentOutDefaultParams = pCppFormattingPage->m_commentOutDefaultParams;
	formattingOptions.m_linesBetweenFunctionImpls = pCppFormattingPage->m_linesBetweenFunctionImpls;

	m_browseInfoPath = pBscPathPage->m_browseInfoPath;

	// save right away
	SaveToRegistry();
	GetCodeFormatterOptions().SaveToRegistry();

	return TRUE;
}


// message handlers

BEGIN_MESSAGE_MAP(CModuleSession, CCmdTarget)
END_MESSAGE_MAP()

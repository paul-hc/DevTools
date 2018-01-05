
#include "stdafx.h"
#include "ModuleSession.h"
#include "IdeUtilities.h"
#include "OptionsPages.h"
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
	const TCHAR entry_debugBreak[] = _T("DebugBreak");
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
	const TCHAR entry_additionalIncludePath[] = _T("Include path");
	const TCHAR entry_additionalAssocFolders[] = _T("Associated folders");
}


IMPLEMENT_DYNCREATE( CModuleSession, CCmdTarget )

CModuleSession::CModuleSession( void )
	: CCmdTarget()
	, m_codeTemplateFile( CModuleSession::GetVStudioMacrosDirPath() + _T("StdCodeTemplates.ctf") )
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
{
	TCHAR developerName[ 128 ];
	::GetEnvironmentVariable( _T("UserName"), developerName, COUNT_OF( developerName ) );
	m_developerName = developerName;
	if ( m_developerName.empty() )
		m_developerName = _T("<YourName>");

	std::tstring mfcSrcFolderPath = CModuleSession::GetVStudioVC98DirPath();

	if ( !mfcSrcFolderPath.empty() )
		m_browseInfoPath = mfcSrcFolderPath + _T("MFC\\SRC\\*.bsc|MFC");
}

CModuleSession::~CModuleSession()
{
}

DebugBreakType CModuleSession::GetDebugBreakType( void )
{
	return (DebugBreakType)AfxGetApp()->GetProfileInt( reg::section_settings, reg::entry_debugBreak, NoBreak );
}

void CModuleSession::StoreDebugBreakType( DebugBreakType debugBreakType )
{
	AfxGetApp()->WriteProfileInt( reg::section_settings, reg::entry_debugBreak, debugBreakType );
}

bool CModuleSession::IsDebugBreakEnabled( void )
{
	switch ( GetDebugBreakType() )
	{
		case NoBreak:
			return false;
		case Break:
			return true;
		default:
			ASSERT( false );
		case PromptBreak:
			return IDYES == AfxMessageBox( IDS_PROMPT_DEBUG_BREAK, MB_YESNO | MB_ICONQUESTION );
	}
}

std::tstring CModuleSession::GetExpandedAdditionalIncludePath( void ) const
{
	return str::ExpandEnvironmentStrings( m_additionalIncludePath.c_str() );
}

void CModuleSession::LoadFromRegistry( void )
{
	CWinApp* pApp = AfxGetApp();

	m_developerName = pApp->GetProfileString( reg::section_settings, reg::entry_developerName, m_developerName.c_str() );
	m_codeTemplateFile = pApp->GetProfileString( reg::section_settings, reg::entry_codeTemplateFile, m_codeTemplateFile.c_str() );
	m_splitMaxColumn = pApp->GetProfileInt( reg::section_settings, reg::entry_splitMaxColumn, m_splitMaxColumn );
	m_menuVertSplitCount = pApp->GetProfileInt( reg::section_settings, reg::entry_menuVertSplitCount, m_menuVertSplitCount );
	m_menuVertSplitCount = __max( m_menuVertSplitCount, 1 );
	m_singleLineCommentToken = pApp->GetProfileString( reg::section_settings, reg::entry_singleLineCommentToken, m_singleLineCommentToken.c_str() );

	m_autoCodeGeneration = pApp->GetProfileInt( reg::section_settings, reg::entry_autoCodeGeneration, m_autoCodeGeneration ) != FALSE;
	m_displayErrorMessages = pApp->GetProfileInt( reg::section_settings, reg::entry_displayErrorMessages, m_displayErrorMessages ) != FALSE;
	m_useCommentDecoration = pApp->GetProfileInt( reg::section_settings, reg::entry_useCommentDecoration, m_useCommentDecoration ) != FALSE;
	m_duplicateLineMoveDown = pApp->GetProfileInt( reg::section_settings, reg::entry_duplicateLineMoveDown, m_duplicateLineMoveDown ) != FALSE;

	m_browseInfoPath = pApp->GetProfileString( reg::section_settings, reg::entry_browseInfoPath, m_browseInfoPath.c_str() );

	m_classPrefix = pApp->GetProfileString( reg::section_settings_prefixes, reg::entry_classPrefix, m_classPrefix.c_str() );
	m_structPrefix = pApp->GetProfileString( reg::section_settings_prefixes, reg::entry_structPrefix, m_structPrefix.c_str() );
	m_enumPrefix = pApp->GetProfileString( reg::section_settings_prefixes, reg::entry_enumPrefix, m_enumPrefix.c_str() );

	m_additionalIncludePath = (LPCTSTR)pApp->GetProfileString( reg::section_settings_Additional, reg::entry_additionalIncludePath, m_additionalIncludePath.c_str() );
	m_additionalAssocFolders = (LPCTSTR)pApp->GetProfileString( reg::section_settings_Additional, reg::entry_additionalAssocFolders, m_additionalAssocFolders.c_str() );

	{
		reg::CKey key( HKEY_CURRENT_USER, _T("Software\\Microsoft\\DevStudio\\6.0\\Text Editor\\Tabs/Language Settings\\C/C++") );

		m_vsTabSizeCpp = (int)key.ReadNumber( _T("TabSize"), m_vsTabSizeCpp );
		m_vsKeepTabsCpp = key.ReadNumber( _T("InsertSpaces"), m_vsKeepTabsCpp ) == 0L;
	}
	{
		reg::CKey key( HKEY_CURRENT_USER, _T("Software\\Microsoft\\DevStudio\\6.0\\General") );
		m_vsUseStandardWindowsMenu = key.ReadNumber( _T("TraditionalMenu"), m_vsUseStandardWindowsMenu ) != 0L;
	}
}

void CModuleSession::SaveToRegistry( void ) const
{
	CWinApp* pApp = AfxGetApp();

	pApp->WriteProfileString( reg::section_settings, reg::entry_developerName, m_developerName.c_str() );
	pApp->WriteProfileString( reg::section_settings, reg::entry_codeTemplateFile, m_codeTemplateFile.c_str() );
	pApp->WriteProfileInt( reg::section_settings, reg::entry_splitMaxColumn, m_splitMaxColumn );
	pApp->WriteProfileInt( reg::section_settings, reg::entry_menuVertSplitCount, m_menuVertSplitCount );
	pApp->WriteProfileString( reg::section_settings, reg::entry_singleLineCommentToken, m_singleLineCommentToken.c_str() );

	pApp->WriteProfileInt( reg::section_settings, reg::entry_autoCodeGeneration, m_autoCodeGeneration );
	pApp->WriteProfileInt( reg::section_settings, reg::entry_displayErrorMessages, m_displayErrorMessages );
	pApp->WriteProfileInt( reg::section_settings, reg::entry_useCommentDecoration, m_useCommentDecoration );
	pApp->WriteProfileInt( reg::section_settings, reg::entry_duplicateLineMoveDown, m_duplicateLineMoveDown );

	// save it only when modified (options dialog)
//	if ( m_codeFormatterOptionsPtr.get() != NULL )
//		m_codeFormatterOptionsPtr->SaveToRegistry();

	pApp->WriteProfileString( reg::section_settings, reg::entry_browseInfoPath, m_browseInfoPath.c_str() );

	pApp->WriteProfileString( reg::section_settings_prefixes, reg::entry_classPrefix, m_classPrefix.c_str() );
	pApp->WriteProfileString( reg::section_settings_prefixes, reg::entry_structPrefix, m_structPrefix.c_str() );
	pApp->WriteProfileString( reg::section_settings_prefixes, reg::entry_enumPrefix, m_enumPrefix.c_str() );

	pApp->WriteProfileString( reg::section_settings_Additional, reg::entry_additionalIncludePath, m_additionalIncludePath.c_str() );
	pApp->WriteProfileString( reg::section_settings_Additional, reg::entry_additionalAssocFolders, m_additionalAssocFolders.c_str() );
}

code::CFormatterOptions& CModuleSession::GetCodeFormatterOptions( void )
{
	if ( m_codeFormatterOptionsPtr.get() == NULL )
	{	// lazy creation/registry loading
		m_codeFormatterOptionsPtr = std::auto_ptr< code::CFormatterOptions >( new code::CFormatterOptions );
		m_codeFormatterOptionsPtr->LoadFromRegistry();
	}

	return *m_codeFormatterOptionsPtr;
}

bool CModuleSession::EditOptions( void )
{
	COptionsSheet optionsSheet( ide::getRootWindow() );

	optionsSheet.m_generalPage.m_developerName = m_developerName.c_str();
	optionsSheet.m_generalPage.m_codeTemplateFile = m_codeTemplateFile.c_str();
	optionsSheet.m_generalPage.m_menuVertSplitCount = m_menuVertSplitCount;
	optionsSheet.m_generalPage.m_singleLineCommentToken = m_singleLineCommentToken.c_str();

	optionsSheet.m_generalPage.m_autoCodeGeneration = m_autoCodeGeneration;
	optionsSheet.m_generalPage.m_displayErrorMessages = m_displayErrorMessages;
	optionsSheet.m_generalPage.m_useCommentDecoration = m_useCommentDecoration;
	optionsSheet.m_generalPage.m_duplicateLineMoveDown = m_duplicateLineMoveDown;

	optionsSheet.m_generalPage.m_classPrefix = m_classPrefix.c_str();
	optionsSheet.m_generalPage.m_structPrefix = m_structPrefix.c_str();
	optionsSheet.m_generalPage.m_enumPrefix = m_enumPrefix.c_str();

	optionsSheet.m_formattingPage.m_splitMaxColumn = m_splitMaxColumn;

	optionsSheet.m_bscPathPage.m_browseInfoPath = m_browseInfoPath.c_str();

	if ( optionsSheet.DoModal() != IDOK )
		return FALSE;

	m_developerName = optionsSheet.m_generalPage.m_developerName;
	m_codeTemplateFile = optionsSheet.m_generalPage.m_codeTemplateFile;
	m_menuVertSplitCount = optionsSheet.m_generalPage.m_menuVertSplitCount;
	m_singleLineCommentToken = optionsSheet.m_generalPage.m_singleLineCommentToken;

	m_autoCodeGeneration = optionsSheet.m_generalPage.m_autoCodeGeneration;
	m_displayErrorMessages = optionsSheet.m_generalPage.m_displayErrorMessages;
	m_useCommentDecoration = optionsSheet.m_generalPage.m_useCommentDecoration;
	m_duplicateLineMoveDown = optionsSheet.m_generalPage.m_duplicateLineMoveDown;

	m_classPrefix = optionsSheet.m_generalPage.m_classPrefix;
	m_structPrefix = optionsSheet.m_generalPage.m_structPrefix;
	m_enumPrefix = optionsSheet.m_generalPage.m_enumPrefix;

	code::CFormatterOptions& formattingOptions = GetCodeFormatterOptions();

	formattingOptions.m_preserveMultipleWhiteSpace = optionsSheet.m_formattingPage.m_preserveMultipleWhiteSpace;
	formattingOptions.m_deleteTrailingWhiteSpace = optionsSheet.m_formattingPage.m_deleteTrailingWhiteSpace;
	formattingOptions.m_breakSeparators = optionsSheet.m_formattingPage.m_breakSeparators;
	formattingOptions.m_braceRules = optionsSheet.m_formattingPage.m_braceRules;
	formattingOptions.m_operatorRules = optionsSheet.m_formattingPage.m_operatorRules;
	m_splitMaxColumn = optionsSheet.m_formattingPage.m_splitMaxColumn;

	formattingOptions.m_returnTypeOnSeparateLine = optionsSheet.m_cppImplFormattingPage.m_returnTypeOnSeparateLine;
	formattingOptions.m_commentOutDefaultParams = optionsSheet.m_cppImplFormattingPage.m_commentOutDefaultParams;
	formattingOptions.m_linesBetweenFunctionImpls = optionsSheet.m_cppImplFormattingPage.m_linesBetweenFunctionImpls;

	m_browseInfoPath = optionsSheet.m_bscPathPage.m_browseInfoPath;

	// save right away
	SaveToRegistry();
	GetCodeFormatterOptions().SaveToRegistry();

	return TRUE;
}

std::tstring CModuleSession::GetVStudioCommonDirPath( bool trailSlash /*= true*/ )
{
	reg::CKey key( HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\VisualStudio\\6.0\\Setup") );
	std::tstring vStudioCommonDirPath;

	if ( key.IsValid() )
	{
		vStudioCommonDirPath = key.ReadString( _T("VsCommonDir") );
		path::SetBackslash( vStudioCommonDirPath, trailSlash );
	}
	return vStudioCommonDirPath;
}

std::tstring CModuleSession::GetVStudioMacrosDirPath( bool trailSlash /*= true*/ )
{
	std::tstring vStudioMacrosDir( GetVStudioCommonDirPath() );
	if ( !vStudioMacrosDir.empty() )
	{
		vStudioMacrosDir += _T("MSDev98\\Macros");
		path::SetBackslash( vStudioMacrosDir, trailSlash );
	}
	return vStudioMacrosDir;
}

std::tstring CModuleSession::GetVStudioVC98DirPath( bool trailSlash /*= true*/ )
{
	reg::CKey key( HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\VisualStudio\\6.0\\Setup\\Microsoft Visual C++") );
	//reg::CKey key( HKEY_LOCAL_MACHINE, _T("SOFTWARE\\Microsoft\\DevStudio\\6.0\\Products\\Microsoft Visual C++") );
	std::tstring vStudioVC98DirPath;

	if ( key.IsValid() )
	{
		vStudioVC98DirPath = key.ReadString( _T("ProductDir") );
		path::SetBackslash( vStudioVC98DirPath, trailSlash );
	}
	return vStudioVC98DirPath;
}


// message handlers

BEGIN_MESSAGE_MAP(CModuleSession, CCmdTarget)
END_MESSAGE_MAP()

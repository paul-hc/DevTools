
#include "pch.h"
#include "ModuleSession.h"
#include "ModuleOptions.h"
#include "IdeUtilities.h"
#include "Application.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


IMPLEMENT_DYNCREATE(ModuleOptions, CCmdTarget)


ModuleOptions::ModuleOptions( void )
{
	EnableAutomation();

	AfxOleLockApp();		// To keep the application running as long as an OLE automation object is active, the constructor calls AfxOleLockApp.
}

ModuleOptions::~ModuleOptions()
{
	// To terminate the application when all objects created with with OLE automation, the destructor calls AfxOleUnlockApp.
	AfxOleUnlockApp();
}

void ModuleOptions::OnFinalRelease( void )
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.	Add additional cleanup required for your
	// object before calling the base class.

	__super::OnFinalRelease();
}

BEGIN_MESSAGE_MAP(ModuleOptions, CCmdTarget)
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(ModuleOptions, CCmdTarget)
	DISP_PROPERTY_EX_ID(ModuleOptions, "developerName", dispid_developerName, GetDeveloperName, SetDeveloperName, VT_BSTR)
	DISP_PROPERTY_EX_ID(ModuleOptions, "codeTemplateFile", dispid_codeTemplateFile, GetCodeTemplateFile, SetCodeTemplateFile, VT_BSTR)
	DISP_PROPERTY_EX_ID(ModuleOptions, "splitMaxColumn", dispid_splitMaxColumn, GetSplitMaxColumn, SetSplitMaxColumn, VT_I4)
	DISP_PROPERTY_EX_ID(ModuleOptions, "menuVertSplitCount", dispid_menuVertSplitCount, GetMenuVertSplitCount, SetMenuVertSplitCount, VT_I4)
	DISP_PROPERTY_EX_ID(ModuleOptions, "singleLineCommentToken", dispid_singleLineCommentToken, GetSingleLineCommentToken, SetSingleLineCommentToken, VT_BSTR)
	DISP_PROPERTY_EX_ID(ModuleOptions, "classPrefix", dispid_classPrefix, GetClassPrefix, SetClassPrefix, VT_BSTR)
	DISP_PROPERTY_EX_ID(ModuleOptions, "structPrefix", dispid_structPrefix, GetStructPrefix, SetStructPrefix, VT_BSTR)
	DISP_PROPERTY_EX_ID(ModuleOptions, "enumPrefix", dispid_enumPrefix, GetEnumPrefix, SetEnumPrefix, VT_BSTR)
	DISP_PROPERTY_EX_ID(ModuleOptions, "autoCodeGeneration", dispid_autoCodeGeneration, GetAutoCodeGeneration, SetAutoCodeGeneration, VT_BOOL)
	DISP_PROPERTY_EX_ID(ModuleOptions, "displayErrorMessages", dispid_displayErrorMessages, GetDisplayErrorMessages, SetDisplayErrorMessages, VT_BOOL)
	DISP_PROPERTY_EX_ID(ModuleOptions, "useCommentDecoration", dispid_useCommentDecoration, GetUseCommentDecoration, SetUseCommentDecoration, VT_BOOL)
	DISP_PROPERTY_EX_ID(ModuleOptions, "duplicateLineMoveDown", dispid_duplicateLineMoveDown, GetDuplicateLineMoveDown, SetDuplicateLineMoveDown, VT_BOOL)
	DISP_PROPERTY_EX_ID(ModuleOptions, "browseInfoPath", dispid_browseInfoPath, GetBrowseInfoPath, SetBrowseInfoPath, VT_BSTR)
	DISP_PROPERTY_EX_ID(ModuleOptions, "additionalIncludePath", dispid_additionalIncludePath, GetAdditionalIncludePath, SetAdditionalIncludePath, VT_BSTR)
	DISP_PROPERTY_EX_ID(ModuleOptions, "additionalAssocFolders", dispid_additionalAssocFolders, GetAdditionalAssocFolders, SetAdditionalAssocFolders, VT_BSTR)
	DISP_PROPERTY_EX_ID(ModuleOptions, "linesBetweenFunctionImpls", dispid_linesBetweenFunctionImpls, GetLinesBetweenFunctionImpls, SetNotSupported, VT_I4)
	DISP_PROPERTY_EX_ID(ModuleOptions, "returnTypeOnSeparateLine", dispid_returnTypeOnSeparateLine, GetReturnTypeOnSeparateLine, SetNotSupported, VT_BOOL)
	DISP_FUNCTION_ID(ModuleOptions, "GetVStudioCommonDirPath", dispidGetVStudioCommonDirPath, GetVStudioCommonDirPath, VT_BSTR, VTS_BOOL)
	DISP_FUNCTION_ID(ModuleOptions, "GetVStudioMacrosDirPath", dispidGetVStudioMacrosDirPath, GetVStudioMacrosDirPath, VT_BSTR, VTS_BOOL)
	DISP_FUNCTION_ID(ModuleOptions, "GetVStudioVC98DirPath", dispidGetVStudioVC98DirPath, GetVStudioVC98DirPath, VT_BSTR, VTS_BOOL)
	DISP_FUNCTION_ID(ModuleOptions, "EditOptions", dispidEditOptions, EditOptions, VT_BOOL, VTS_NONE)
	DISP_FUNCTION_ID(ModuleOptions, "LoadOptions", dispidLoadOptions, LoadOptions, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION_ID(ModuleOptions, "SaveOptions", dispidSaveOptions, SaveOptions, VT_EMPTY, VTS_NONE)
END_DISPATCH_MAP()

// Note: we add support for IID_IModuleOptions to support typesafe binding
//	from VBA.  This IID must match the GUID that is attached to the
//	dispinterface in the .IDL file.

// {4804CCBD-E60C-45C9-A6C0-3E1E96DA3725}
static const IID IID_IModuleOptions =
{ 0x4804ccbd, 0xe60c, 0x45c9, { 0xa6, 0xc0, 0x3e, 0x1e, 0x96, 0xda, 0x37, 0x25 } };

BEGIN_INTERFACE_MAP(ModuleOptions, CCmdTarget)
	INTERFACE_PART(ModuleOptions, IID_IModuleOptions, Dispatch)
END_INTERFACE_MAP()

// {4064259A-55DC-4CD5-8A17-DD1CC7B59673}
IMPLEMENT_OLECREATE(ModuleOptions, "IDETools.ModuleOptions", 0x4064259a, 0x55dc, 0x4cd5, 0x8a, 0x17, 0xdd, 0x1c, 0xc7, 0xb5, 0x96, 0x73)


// automation methods

BSTR ModuleOptions::GetDeveloperName( void )
{
	return str::AllocSysString( app::GetModuleSession().m_developerName );
}

void ModuleOptions::SetDeveloperName( LPCTSTR lpszNewValue )
{
	app::GetModuleSession().m_developerName = lpszNewValue;
}

BSTR ModuleOptions::GetCodeTemplateFile( void )
{
	return str::AllocSysString( app::GetModuleSession().m_codeSnippetsPath.Get() );
}

void ModuleOptions::SetCodeTemplateFile( LPCTSTR lpszNewValue )
{
	app::GetModuleSession().m_codeSnippetsPath.Set( lpszNewValue );
}

long ModuleOptions::GetSplitMaxColumn( void )
{
	return app::GetModuleSession().m_splitMaxColumn;
}

void ModuleOptions::SetSplitMaxColumn( long nNewValue )
{
	app::GetModuleSession().m_splitMaxColumn = __max( nNewValue, 1 );
}

long ModuleOptions::GetMenuVertSplitCount( void )
{
	return app::GetModuleSession().m_menuVertSplitCount;
}

void ModuleOptions::SetMenuVertSplitCount( long nNewValue )
{
	app::GetModuleSession().m_menuVertSplitCount = (UINT)nNewValue;
}

BSTR ModuleOptions::GetSingleLineCommentToken( void )
{
	return str::AllocSysString( app::GetModuleSession().m_singleLineCommentToken );
}

void ModuleOptions::SetSingleLineCommentToken( LPCTSTR lpszNewValue )
{
	app::GetModuleSession().m_singleLineCommentToken = lpszNewValue;
}

BSTR ModuleOptions::GetClassPrefix( void )
{
	return str::AllocSysString( app::GetModuleSession().m_classPrefix );
}

void ModuleOptions::SetClassPrefix( LPCTSTR lpszNewValue )
{
	app::GetModuleSession().m_classPrefix = lpszNewValue;
}

BSTR ModuleOptions::GetStructPrefix( void )
{
	return str::AllocSysString( app::GetModuleSession().m_structPrefix );
}

void ModuleOptions::SetStructPrefix( LPCTSTR lpszNewValue )
{
	app::GetModuleSession().m_structPrefix = lpszNewValue;
}

BSTR ModuleOptions::GetEnumPrefix( void )
{
	return str::AllocSysString( app::GetModuleSession().m_enumPrefix );
}

void ModuleOptions::SetEnumPrefix( LPCTSTR lpszNewValue )
{
	app::GetModuleSession().m_enumPrefix = lpszNewValue;
}

BOOL ModuleOptions::GetAutoCodeGeneration( void )
{
	return app::GetModuleSession().m_autoCodeGeneration;
}

void ModuleOptions::SetAutoCodeGeneration( BOOL bNewValue )
{
	app::GetModuleSession().m_autoCodeGeneration = bNewValue != FALSE;
}

BOOL ModuleOptions::GetDisplayErrorMessages( void )
{
	return app::GetModuleSession().m_displayErrorMessages;
}

void ModuleOptions::SetDisplayErrorMessages( BOOL bNewValue )
{
	app::GetModuleSession().m_displayErrorMessages = bNewValue != FALSE;
}

BOOL ModuleOptions::GetUseCommentDecoration( void )
{
	return app::GetModuleSession().m_useCommentDecoration;
}

void ModuleOptions::SetUseCommentDecoration( BOOL bNewValue )
{
	app::GetModuleSession().m_useCommentDecoration = bNewValue != FALSE;
}

BOOL ModuleOptions::GetDuplicateLineMoveDown( void )
{
	return app::GetModuleSession().m_duplicateLineMoveDown;
}

void ModuleOptions::SetDuplicateLineMoveDown( BOOL bNewValue )
{
	app::GetModuleSession().m_duplicateLineMoveDown = bNewValue != FALSE;
}

BSTR ModuleOptions::GetBrowseInfoPath( void )
{
	return str::AllocSysString( app::GetModuleSession().m_browseInfoPath.Get() );
}

void ModuleOptions::SetBrowseInfoPath( LPCTSTR lpszNewValue )
{
	app::GetModuleSession().m_browseInfoPath.Set( lpszNewValue );
}

BSTR ModuleOptions::GetAdditionalIncludePath( void )
{
	return str::AllocSysString( app::GetModuleSession().m_moreAdditionalIncludePath.Join() );
}

void ModuleOptions::SetAdditionalIncludePath( LPCTSTR lpszNewValue )
{
	app::GetModuleSession().m_moreAdditionalIncludePath.Split( lpszNewValue );
}

BSTR ModuleOptions::GetAdditionalAssocFolders( void )
{
	return str::AllocSysString( app::GetModuleSession().GetAdditionalAssocFolders() );
}

void ModuleOptions::SetAdditionalAssocFolders( LPCTSTR lpszNewValue )
{
	app::GetModuleSession().SetAdditionalAssocFolders( lpszNewValue );
}

long ModuleOptions::GetLinesBetweenFunctionImpls( void )
{
	return app::GetModuleSession().GetCodeFormatterOptions().m_linesBetweenFunctionImpls;
}

BOOL ModuleOptions::GetReturnTypeOnSeparateLine( void )
{
	return app::GetModuleSession().GetCodeFormatterOptions().m_returnTypeOnSeparateLine;
}

BSTR ModuleOptions::GetVStudioCommonDirPath( BOOL addTrailingSlash )
{
	return str::AllocSysString( ide::vs6::GetCommonDirPath( addTrailingSlash != FALSE ).Get() );
}

BSTR ModuleOptions::GetVStudioMacrosDirPath( BOOL addTrailingSlash )
{
	return str::AllocSysString( ide::vs6::GetMacrosDirPath( addTrailingSlash != FALSE ).Get() );
}

BSTR ModuleOptions::GetVStudioVC98DirPath( BOOL addTrailingSlash )
{
	return str::AllocSysString( ide::vs6::GetVC98DirPath( addTrailingSlash != FALSE ).Get() );
}

BOOL ModuleOptions::EditOptions( void )
{
	return app::GetModuleSession().EditOptions();
}

void ModuleOptions::LoadOptions( void )
{
	app::GetModuleSession().LoadFromRegistry();
}

void ModuleOptions::SaveOptions( void )
{
	app::GetModuleSession().SaveToRegistry();
}

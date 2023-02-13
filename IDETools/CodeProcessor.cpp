
#include "pch.h"
#include "CodeProcessor.h"
#include "Formatter.h"
#include "CppImplementationFormatter.h"
#include "ModuleSession.h"
#include "Application.h"
#include <sstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#define PROCESS_CODE_NOTRYCATCH( newCodeText, codeText, processedCodeText )\
	newCodeText = ( codeText );\
	newCodeText = ( processedCodeText );\


#ifdef _DEBUG

#define PROCESS_CODE( newCodeText, codeText, processedCodeText )\
	try\
	{\
		newCodeText = ( processedCodeText );\
	}\
	catch ( CException* exc )\
	{\
		newCodeText = ( codeText );\
		exc->ReportError();\
		exc->Delete();\
	}\
	catch ( const std::exception& exc )\
	{\
		AfxMessageBox( str::formatString( _T("ERROR:\r\n%s"), exc.what() ), MB_OK | MB_ICONERROR | MB_APPLMODAL );\
		newCodeText = ( codeText );\
	}\

#else // !_DEBUG

#define PROCESS_CODE( newCodeText, codeText, processedCodeText )\
	try\
	{\
		newCodeText = ( processedCodeText );\
	}\
	catch ( CException* exc )\
	{\
		newCodeText = ( codeText );\
		exc->ReportError();\
		exc->Delete();\
	}\
	catch ( const std::exception& exc )\
	{\
		AfxMessageBox( str::formatString( _T("ERROR:\r\n%s"), exc.what() ), MB_OK | MB_ICONERROR | MB_APPLMODAL );\
		newCodeText = ( codeText );\
	}\
	catch ( ... )\
	{\
		newCodeText = ( codeText );\
		AfxMessageBox( _T("Handled uncaught exception in CodeProcessor!"), MB_OK | MB_ICONWARNING );\
	}\

#endif


// CodeProcessor implementation

IMPLEMENT_DYNCREATE(CodeProcessor, CCmdTarget)

CodeProcessor::CodeProcessor()
	: CCmdTarget()
	, m_docLanguage( _T("C/C++") )
	, m_tabSize( app::GetModuleSession().m_vsTabSizeCpp )
	, m_useTabs( app::GetModuleSession().m_vsKeepTabsCpp )
{
	EnableAutomation();

	// To keep the application running as long as an OLE automation
	//	object is active, the constructor calls AfxOleLockApp.
	AfxOleLockApp();
}

CodeProcessor::~CodeProcessor()
{
	// To terminate the application when all objects created with
	// 	with OLE automation, the destructor calls AfxOleUnlockApp.
	AfxOleUnlockApp();
}

void CodeProcessor::OnFinalRelease()
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.

	__super::OnFinalRelease();
}

BEGIN_MESSAGE_MAP(CodeProcessor, CCmdTarget)
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CodeProcessor, CCmdTarget)
	DISP_PROPERTY_EX_ID(CodeProcessor, "DocLanguage", dispidDocLanguage, GetNotSupported, SetDocLanguage, VT_BSTR)
	DISP_PROPERTY_EX_ID(CodeProcessor, "TabSize", dispidTabSize, GetNotSupported, SetTabSize, VT_I4)
	DISP_PROPERTY_EX_ID(CodeProcessor, "UseTabs", dispidUseTabs, GetNotSupported, SetUseTabs, VT_BOOL)
	DISP_PROPERTY_EX_ID(CodeProcessor, "CancelTag", dispidCancelTag, GetCancelTag, SetNotSupported, VT_BSTR)
	DISP_FUNCTION_ID(CodeProcessor, "AutoFormatCode", dispidAutoFormatCode, AutoFormatCode, VT_BSTR, VTS_BSTR)
	DISP_FUNCTION_ID(CodeProcessor, "SplitArgumentList", dispidSplitArgumentList, SplitArgumentList, VT_BSTR, VTS_BSTR VTS_I4 VTS_I4)
	DISP_FUNCTION_ID(CodeProcessor, "ExtractTypeDescriptor", dispidExtractTypeDescriptor, ExtractTypeDescriptor, VT_BSTR, VTS_BSTR VTS_BSTR)
	DISP_FUNCTION_ID(CodeProcessor, "ImplementMethods", dispidImplementMethods, ImplementMethods, VT_BSTR, VTS_BSTR VTS_BSTR VTS_BOOL)
	DISP_FUNCTION_ID(CodeProcessor, "ToggleComment", dispidToggleComment, ToggleComment, VT_BSTR, VTS_BSTR)
	DISP_FUNCTION_ID(CodeProcessor, "FormatWhitespaces", dispidFormatWhitespaces, FormatWhitespaces, VT_BSTR, VTS_BSTR)
	DISP_FUNCTION_ID(CodeProcessor, "GenerateConsecutiveNumbers", dispidGenerateConsecutiveNumbers, GenerateConsecutiveNumbers, VT_BSTR, VTS_BSTR)
	DISP_FUNCTION_ID(CodeProcessor, "SortLines", dispidSortLines, SortLines, VT_BSTR, VTS_BSTR VTS_BOOL)
	DISP_FUNCTION_ID(CodeProcessor, "AutoMakeCode", dispidAutoMakeCode, AutoMakeCode, VT_BSTR, VTS_BSTR)
	DISP_FUNCTION_ID(CodeProcessor, "TokenizeText", dispidTokenizeText, TokenizeText, VT_BSTR, VTS_BSTR)
END_DISPATCH_MAP()

// Note: we add support for IID_ICodeProcessor to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the
//  dispinterface in the .IDL file.

// {397B64A6-EF38-4E9E-8343-D1A9102284D6}
static const IID IID_ICodeProcessor =
{ 0x397b64a6, 0xef38, 0x4e9e, { 0x83, 0x43, 0xd1, 0xa9, 0x10, 0x22, 0x84, 0xd6 } };

BEGIN_INTERFACE_MAP(CodeProcessor, CCmdTarget)
	INTERFACE_PART(CodeProcessor, IID_ICodeProcessor, Dispatch)
END_INTERFACE_MAP()

// {F70182C0-AE07-4DEB-AFEB-31BCC6BB244C}
IMPLEMENT_OLECREATE(CodeProcessor, "IDETools.CodeProcessor", 0xf70182c0, 0xae07, 0x4deb, 0xaf, 0xeb, 0x31, 0xbc, 0xc6, 0xbb, 0x24, 0x4c)


// message handlers

void CodeProcessor::SetDocLanguage( LPCTSTR lpszNewValue )
{
	m_docLanguage = lpszNewValue;
}

void CodeProcessor::SetTabSize( long nNewValue )
{
	m_tabSize = (int)nNewValue;
}

void CodeProcessor::SetUseTabs( BOOL bNewValue )
{
	m_useTabs = ( bNewValue != FALSE );
}

BSTR CodeProcessor::GetCancelTag( void )
{
	return code::CFormatter::m_cancelTag.AllocSysString();
}

BSTR CodeProcessor::AutoFormatCode( LPCTSTR codeText )
{
	code::CFormatter codeFormatter( app::GetModuleSession().GetCodeFormatterOptions() );

	codeFormatter.setDocLanguage( m_docLanguage );
	codeFormatter.setTabSize( m_tabSize );
	codeFormatter.setUseTabs( m_useTabs != FALSE );

	CString newCodeText;

	PROCESS_CODE( newCodeText, codeText, codeFormatter.formatCode( codeText ) )
	return newCodeText.AllocSysString();
}

BSTR CodeProcessor::SplitArgumentList( LPCTSTR codeText, long splitAtColumn, long targetBracketLevel )
{
	code::CFormatter codeFormatter( app::GetModuleSession().GetCodeFormatterOptions() );

	codeFormatter.setDocLanguage( m_docLanguage );
	codeFormatter.setTabSize( m_tabSize );
	codeFormatter.setUseTabs( m_useTabs != FALSE );

	CString newCodeText;

	PROCESS_CODE( newCodeText, codeText, codeFormatter.splitArgumentList( codeText, splitAtColumn, targetBracketLevel ) )
	return newCodeText.AllocSysString();
}

BSTR CodeProcessor::ExtractTypeDescriptor( LPCTSTR functionImplLine, LPCTSTR docFileExt )
{
	code::CppImplementationFormatter cppCodeFormatter( app::GetModuleSession().GetCodeFormatterOptions() );
	CString typeDescriptor;

	PROCESS_CODE( typeDescriptor, _T("<cancel>"), cppCodeFormatter.extractTypeDescriptor( functionImplLine, docFileExt ) )
	return typeDescriptor.AllocSysString();
}

BSTR CodeProcessor::ImplementMethods( LPCTSTR methodPrototypes, LPCTSTR typeDescriptor, BOOL isInline )
{
	code::CppImplementationFormatter cppCodeFormatter( app::GetModuleSession().GetCodeFormatterOptions() );

	cppCodeFormatter.setDocLanguage( m_docLanguage );
	cppCodeFormatter.setTabSize( m_tabSize );
	cppCodeFormatter.setUseTabs( m_useTabs != FALSE );

	CString newCodeText;

	PROCESS_CODE( newCodeText, methodPrototypes,
				  cppCodeFormatter.implementMethodBlock( methodPrototypes, typeDescriptor, isInline != FALSE ) )
	return newCodeText.AllocSysString();
}

BSTR CodeProcessor::ToggleComment( LPCTSTR codeText )
{
	code::CFormatter codeFormatter( app::GetModuleSession().GetCodeFormatterOptions() );

	codeFormatter.setDocLanguage( m_docLanguage );
	codeFormatter.setTabSize( m_tabSize );
	codeFormatter.setUseTabs( m_useTabs != FALSE );

	CString newCodeText;

	PROCESS_CODE( newCodeText, codeText, codeFormatter.toggleComment( codeText ) )
	return newCodeText.AllocSysString();
}

BSTR CodeProcessor::FormatWhitespaces( LPCTSTR codeText )
{
	code::CFormatter codeFormatter( app::GetModuleSession().GetCodeFormatterOptions() );

	codeFormatter.setDocLanguage( m_docLanguage );
	codeFormatter.setTabSize( m_tabSize );
	codeFormatter.setUseTabs( m_useTabs != FALSE );

	CString newCodeText;

	PROCESS_CODE( newCodeText, codeText, codeFormatter.formatCode( codeText, true, true ) )
	return newCodeText.AllocSysString();
}

BSTR CodeProcessor::GenerateConsecutiveNumbers( LPCTSTR codeText )
{
	code::CFormatter codeFormatter( app::GetModuleSession().GetCodeFormatterOptions() );

	codeFormatter.setDocLanguage( m_docLanguage );
	codeFormatter.setTabSize( m_tabSize );
	codeFormatter.setUseTabs( m_useTabs != FALSE );

	CString newCodeText;

	PROCESS_CODE( newCodeText, codeText, codeFormatter.generateConsecutiveNumbers( codeText ) )

	return newCodeText.AllocSysString();
}

BSTR CodeProcessor::SortLines( LPCTSTR codeText, BOOL ascending )
{
	code::CFormatter codeFormatter( app::GetModuleSession().GetCodeFormatterOptions() );

	codeFormatter.setDocLanguage( m_docLanguage );
	codeFormatter.setTabSize( m_tabSize );
	codeFormatter.setUseTabs( m_useTabs != FALSE );

	CString newCodeText;

	PROCESS_CODE( newCodeText, codeText, codeFormatter.sortLines( codeText, ascending != FALSE ) )
	return newCodeText.AllocSysString();
}

BSTR CodeProcessor::AutoMakeCode( LPCTSTR codeText )
{
	code::CppImplementationFormatter cppCodeFormatter( app::GetModuleSession().GetCodeFormatterOptions() );

	cppCodeFormatter.setDocLanguage( m_docLanguage );
	cppCodeFormatter.setTabSize( m_tabSize );
	cppCodeFormatter.setUseTabs( m_useTabs != FALSE );

	CString newCodeText;

	PROCESS_CODE( newCodeText, codeText, cppCodeFormatter.autoMakeCode( codeText ) )
	return newCodeText.AllocSysString();
}

BSTR CodeProcessor::TokenizeText( LPCTSTR codeText )
{
	code::CppImplementationFormatter cppCodeFormatter( app::GetModuleSession().GetCodeFormatterOptions() );

	cppCodeFormatter.setDocLanguage( m_docLanguage );
	cppCodeFormatter.setTabSize( m_tabSize );
	cppCodeFormatter.setUseTabs( m_useTabs != FALSE );

	CString newCodeText;

	PROCESS_CODE( newCodeText, codeText, cppCodeFormatter.tokenizeText( codeText ) )
	return newCodeText.AllocSysString();
}

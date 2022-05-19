
#include "stdafx.h"
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
		AfxMessageBox( _T("Handled uncaught exception in CCodeProcessor!"), MB_OK | MB_ICONWARNING );\
	}\

#endif


// CCodeProcessor implementation

IMPLEMENT_DYNCREATE(CCodeProcessor, CCmdTarget)

CCodeProcessor::CCodeProcessor()
	: CAutomationBase()
	, m_docLanguage( _T("C/C++") )
	, m_tabSize( app::GetModuleSession().m_vsTabSizeCpp )
	, m_useTabs( app::GetModuleSession().m_vsKeepTabsCpp )
{
	EnableAutomation();

	// To keep the application running as long as an OLE automation
	//	object is active, the constructor calls AfxOleLockApp.
	AfxOleLockApp();
}

CCodeProcessor::~CCodeProcessor()
{
	// To terminate the application when all objects created with
	// 	with OLE automation, the destructor calls AfxOleUnlockApp.
	AfxOleUnlockApp();
}

void CCodeProcessor::OnFinalRelease()
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.

	__super::OnFinalRelease();
}

BEGIN_MESSAGE_MAP(CCodeProcessor, CCmdTarget)
	//{{AFX_MSG_MAP(CCodeProcessor)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

BEGIN_DISPATCH_MAP(CCodeProcessor, CCmdTarget)
	//{{AFX_DISPATCH_MAP(CCodeProcessor)
	DISP_PROPERTY_EX(CCodeProcessor, "docLanguage", GetNotSupported, SetDocLanguage, VT_BSTR)
	DISP_PROPERTY_EX(CCodeProcessor, "tabSize", GetNotSupported, SetTabSize, VT_I4)
	DISP_PROPERTY_EX(CCodeProcessor, "useTabs", GetNotSupported, SetUseTabs, VT_BOOL)
	DISP_PROPERTY_EX(CCodeProcessor, "cancelTag", GetCancelTag, SetNotSupported, VT_BSTR)
	DISP_FUNCTION(CCodeProcessor, "AutoFormatCode", AutoFormatCode, VT_BSTR, VTS_BSTR)
	DISP_FUNCTION(CCodeProcessor, "SplitArgumentList", SplitArgumentList, VT_BSTR, VTS_BSTR VTS_I4 VTS_I4)
	DISP_FUNCTION(CCodeProcessor, "ExtractTypeDescriptor", ExtractTypeDescriptor, VT_BSTR, VTS_BSTR VTS_BSTR)
	DISP_FUNCTION(CCodeProcessor, "ImplementMethods", ImplementMethods, VT_BSTR, VTS_BSTR VTS_BSTR VTS_BOOL)
	DISP_FUNCTION(CCodeProcessor, "ToggleComment", ToggleComment, VT_BSTR, VTS_BSTR)
	DISP_FUNCTION(CCodeProcessor, "FormatWhitespaces", FormatWhitespaces, VT_BSTR, VTS_BSTR)
	DISP_FUNCTION(CCodeProcessor, "GenerateConsecutiveNumbers", GenerateConsecutiveNumbers, VT_BSTR, VTS_BSTR)
	DISP_FUNCTION(CCodeProcessor, "SortLines", SortLines, VT_BSTR, VTS_BSTR VTS_BOOL)
	DISP_FUNCTION(CCodeProcessor, "AutoMakeCode", AutoMakeCode, VT_BSTR, VTS_BSTR)
	DISP_FUNCTION(CCodeProcessor, "TokenizeText", TokenizeText, VT_BSTR, VTS_BSTR)
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_ICodeProcessor to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the
//  dispinterface in the .ODL file.

// {397B64A6-EF38-4E9E-8343-D1A9102284D6}
static const IID IID_ICodeProcessor =
{ 0x397b64a6, 0xef38, 0x4e9e, { 0x83, 0x43, 0xd1, 0xa9, 0x10, 0x22, 0x84, 0xd6 } };

BEGIN_INTERFACE_MAP(CCodeProcessor, CCmdTarget)
	INTERFACE_PART(CCodeProcessor, IID_ICodeProcessor, Dispatch)
END_INTERFACE_MAP()

// {F70182C0-AE07-4DEB-AFEB-31BCC6BB244C}
IMPLEMENT_OLECREATE(CCodeProcessor, "IDETools.CodeProcessor", 0xf70182c0, 0xae07, 0x4deb, 0xaf, 0xeb, 0x31, 0xbc, 0xc6, 0xbb, 0x24, 0x4c)


// message handlers

void CCodeProcessor::SetDocLanguage( LPCTSTR lpszNewValue )
{
	m_docLanguage = lpszNewValue;
}

void CCodeProcessor::SetTabSize( long nNewValue )
{
	m_tabSize = (int)nNewValue;
}

void CCodeProcessor::SetUseTabs( BOOL bNewValue )
{
	m_useTabs = ( bNewValue != FALSE );
}

BSTR CCodeProcessor::GetCancelTag( void )
{
	return code::CFormatter::m_cancelTag.AllocSysString();
}

BSTR CCodeProcessor::AutoFormatCode( LPCTSTR codeText )
{
	code::CFormatter codeFormatter( app::GetModuleSession().GetCodeFormatterOptions() );

	codeFormatter.setDocLanguage( m_docLanguage );
	codeFormatter.setTabSize( m_tabSize );
	codeFormatter.setUseTabs( m_useTabs != FALSE );

	CString newCodeText;

	PROCESS_CODE( newCodeText, codeText, codeFormatter.formatCode( codeText ) )
	return newCodeText.AllocSysString();
}

BSTR CCodeProcessor::SplitArgumentList( LPCTSTR codeText, long splitAtColumn, long targetBracketLevel )
{
	code::CFormatter codeFormatter( app::GetModuleSession().GetCodeFormatterOptions() );

	codeFormatter.setDocLanguage( m_docLanguage );
	codeFormatter.setTabSize( m_tabSize );
	codeFormatter.setUseTabs( m_useTabs != FALSE );

	CString newCodeText;

	PROCESS_CODE( newCodeText, codeText, codeFormatter.splitArgumentList( codeText, splitAtColumn, targetBracketLevel ) )
	return newCodeText.AllocSysString();
}

BSTR CCodeProcessor::ExtractTypeDescriptor( LPCTSTR functionImplLine, LPCTSTR docFileExt )
{
	code::CppImplementationFormatter cppCodeFormatter( app::GetModuleSession().GetCodeFormatterOptions() );
	CString typeDescriptor;

	PROCESS_CODE( typeDescriptor, _T("<cancel>"), cppCodeFormatter.extractTypeDescriptor( functionImplLine, docFileExt ) )
	return typeDescriptor.AllocSysString();
}

BSTR CCodeProcessor::ImplementMethods( LPCTSTR methodPrototypes, LPCTSTR typeDescriptor, BOOL isInline )
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

BSTR CCodeProcessor::ToggleComment( LPCTSTR codeText )
{
	code::CFormatter codeFormatter( app::GetModuleSession().GetCodeFormatterOptions() );

	codeFormatter.setDocLanguage( m_docLanguage );
	codeFormatter.setTabSize( m_tabSize );
	codeFormatter.setUseTabs( m_useTabs != FALSE );

	CString newCodeText;

	PROCESS_CODE( newCodeText, codeText, codeFormatter.toggleComment( codeText ) )
	return newCodeText.AllocSysString();
}

BSTR CCodeProcessor::FormatWhitespaces( LPCTSTR codeText )
{
	code::CFormatter codeFormatter( app::GetModuleSession().GetCodeFormatterOptions() );

	codeFormatter.setDocLanguage( m_docLanguage );
	codeFormatter.setTabSize( m_tabSize );
	codeFormatter.setUseTabs( m_useTabs != FALSE );

	CString newCodeText;

	PROCESS_CODE( newCodeText, codeText, codeFormatter.formatCode( codeText, true, true ) )
	return newCodeText.AllocSysString();
}

BSTR CCodeProcessor::GenerateConsecutiveNumbers( LPCTSTR codeText )
{
	code::CFormatter codeFormatter( app::GetModuleSession().GetCodeFormatterOptions() );

	codeFormatter.setDocLanguage( m_docLanguage );
	codeFormatter.setTabSize( m_tabSize );
	codeFormatter.setUseTabs( m_useTabs != FALSE );

	CString newCodeText;

	PROCESS_CODE( newCodeText, codeText, codeFormatter.generateConsecutiveNumbers( codeText ) )

	return newCodeText.AllocSysString();
}

BSTR CCodeProcessor::SortLines( LPCTSTR codeText, BOOL ascending )
{
	code::CFormatter codeFormatter( app::GetModuleSession().GetCodeFormatterOptions() );

	codeFormatter.setDocLanguage( m_docLanguage );
	codeFormatter.setTabSize( m_tabSize );
	codeFormatter.setUseTabs( m_useTabs != FALSE );

	CString newCodeText;

	PROCESS_CODE( newCodeText, codeText, codeFormatter.sortLines( codeText, ascending != FALSE ) )
	return newCodeText.AllocSysString();
}

BSTR CCodeProcessor::AutoMakeCode( LPCTSTR codeText )
{
	code::CppImplementationFormatter cppCodeFormatter( app::GetModuleSession().GetCodeFormatterOptions() );

	cppCodeFormatter.setDocLanguage( m_docLanguage );
	cppCodeFormatter.setTabSize( m_tabSize );
	cppCodeFormatter.setUseTabs( m_useTabs != FALSE );

	CString newCodeText;

	PROCESS_CODE( newCodeText, codeText, cppCodeFormatter.autoMakeCode( codeText ) )
	return newCodeText.AllocSysString();
}

BSTR CCodeProcessor::TokenizeText( LPCTSTR codeText )
{
	code::CppImplementationFormatter cppCodeFormatter( app::GetModuleSession().GetCodeFormatterOptions() );

	cppCodeFormatter.setDocLanguage( m_docLanguage );
	cppCodeFormatter.setTabSize( m_tabSize );
	cppCodeFormatter.setUseTabs( m_useTabs != FALSE );

	CString newCodeText;

	PROCESS_CODE( newCodeText, codeText, cppCodeFormatter.tokenizeText( codeText ) )
	return newCodeText.AllocSysString();
}

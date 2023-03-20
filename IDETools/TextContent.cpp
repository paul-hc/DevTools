
#include "pch.h"
#include "TextContent.h"
#include "CodeSnippetsParser.h"
#include "StringUtilitiesEx.h"
#include "utl/AppTools.h"
#include "utl/RuntimeException.h"
#include <fstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


IMPLEMENT_DYNCREATE(TextContent, CCmdTarget)


TextContent::TextContent( void )
	: CCmdTarget()
	, m_showErrors( TRUE )
{
	EnableAutomation();
	// To keep the application running as long as an OLE automation
	//	object is active, the constructor calls AfxOleLockApp.
	AfxOleLockApp();
}

TextContent::~TextContent()
{
	// To terminate the application when all objects created with
	// 	with OLE automation, the destructor calls AfxOleUnlockApp.
	AfxOleUnlockApp();
}

void TextContent::OnFinalRelease( void )
{
	// When the last reference for an automation object is released
	// OnFinalRelease is called.  The base class will automatically
	// deletes the object.  Add additional cleanup required for your
	// object before calling the base class.
	__super::OnFinalRelease();
}


// message handlers

BEGIN_MESSAGE_MAP(TextContent, CCmdTarget)
END_MESSAGE_MAP()


BEGIN_DISPATCH_MAP(TextContent, CCmdTarget)
	DISP_PROPERTY_NOTIFY_ID(TextContent, "ShowErrors", dispidShowErrors, m_showErrors, OnShowErrorsChanged, VT_BOOL)
	DISP_PROPERTY_EX_ID(TextContent, "Text", dispidText, GetText, SetText, VT_BSTR)
	DISP_PROPERTY_EX_ID(TextContent, "TextLen", dispidTextLen, GetTextLen, SetNotSupported, VT_I4)
	DISP_FUNCTION_ID(TextContent, "LoadFile", dispidLoadFile, LoadFile, VT_BOOL, VTS_BSTR)
	DISP_FUNCTION_ID(TextContent, "LoadFileSection", dispidLoadFileSection, LoadFileSection, VT_BOOL, VTS_BSTR VTS_BSTR)
	DISP_FUNCTION_ID(TextContent, "FindText", dispidFindText, FindText, VT_I4, VTS_BSTR VTS_I4 VTS_BOOL)
	DISP_FUNCTION_ID(TextContent, "ReplaceText", dispidReplaceText, ReplaceText, VT_I4, VTS_BSTR VTS_BSTR VTS_BOOL)
	DISP_FUNCTION_ID(TextContent, "AddEmbeddedContent", dispidAddEmbeddedContent, AddEmbeddedContent, VT_BOOL, VTS_BSTR VTS_BSTR VTS_BOOL)
	DISP_FUNCTION_ID(TextContent, "Tokenize", dispidTokenize, Tokenize, VT_BSTR, VTS_BSTR)
	DISP_FUNCTION_ID(TextContent, "GetNextToken", dispidGetNextToken, GetNextToken, VT_BSTR, VTS_NONE)
	DISP_FUNCTION_ID(TextContent, "MultiLinesToSingleParagraph", dispidMultiLinesToSingleParagraph, MultiLinesToSingleParagraph, VT_I4, VTS_BSTR VTS_BOOL)
	DISP_FUNCTION_ID(TextContent, "AddFieldReplacement", dispidAddFieldReplacement, AddFieldReplacement, VT_EMPTY, VTS_BSTR VTS_BSTR)
	DISP_FUNCTION_ID(TextContent, "ClearFieldReplacements", dispidClearFieldReplacements, ClearFieldReplacements, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION_ID(TextContent, "FormatTimestamp", dispidFormatTimestamp, FormatTimestamp, VT_BSTR, VTS_DATE VTS_BSTR)
END_DISPATCH_MAP()

// Note: we add support for IID_ITextContent to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the
//  dispinterface in the .IDL file.

// {E37FE176-CBB7-11D4-B57C-00D0B74ECB52}
static const IID IID_ITextContent =
{ 0xe37fe176, 0xcbb7, 0x11d4, { 0xb5, 0x7c, 0x0, 0xd0, 0xb7, 0x4e, 0xcb, 0x52 } };

BEGIN_INTERFACE_MAP(TextContent, CCmdTarget)
	INTERFACE_PART(TextContent, IID_ITextContent, Dispatch)
END_INTERFACE_MAP()

// {E37FE177-CBB7-11D4-B57C-00D0B74ECB52}
IMPLEMENT_OLECREATE(TextContent, "IDETools.TextContent", 0xe37fe177, 0xcbb7, 0x11d4, 0xb5, 0x7c, 0x0, 0xd0, 0xb7, 0x4e, 0xcb, 0x52)


// interface methods

BSTR TextContent::GetText( void )
{
	return str::AllocSysString( m_textContent );
}

void TextContent::SetText( LPCTSTR lpszNewValue )
{
	m_textContent = lpszNewValue;
}

long TextContent::GetTextLen()
{
	return static_cast<long>( m_textContent.length() );
}

void TextContent::OnShowErrorsChanged( void )
{
}

BOOL TextContent::LoadFile( LPCTSTR textFilePath )
{
	m_textContent.clear();

	try
	{
		std::ifstream input( textFilePath, std::ios_base::in | std::ios_base::binary );		// read line ends as "\r\n"
		if ( !input.is_open() )
			throw CRuntimeException( str::Format( _T("Unable to open text file %s."), textFilePath ), UTL_FILE_LINE );

		std::ostringstream oss;
		oss << input.rdbuf();
		m_textContent = str::FromUtf8( oss.str().c_str() );

		return TRUE;
	}
	catch ( const std::exception& exc )
	{
		m_showErrors ? app::ReportException( exc ) : app::TraceException( exc );
		return FALSE;
	}
}

BOOL TextContent::LoadFileSection( LPCTSTR compoundFilePath, LPCTSTR sectionName )
{
	m_textContent.clear();

	try
	{
		CCodeSnippetsParser textParser;
		fs::CPath filePath( compoundFilePath );

		textParser.StoreLiterals( m_fieldReplacements );
		textParser.LoadFile( filePath );

		m_textContent = textParser.ExpandSection( sectionName );

		return !m_textContent.empty();
	}
	catch ( const std::exception& exc )
	{
		m_showErrors ? app::ReportException( exc ) : app::TraceException( exc );
		return FALSE;
	}
}

long TextContent::FindText( LPCTSTR pattern, long startPos, BOOL caseSensitive )
{
	return caseSensitive
		? static_cast<long>( str::Find<str::Case>( m_textContent.c_str(), pattern, startPos ) )
		: static_cast<long>( str::Find<str::IgnoreCase>( m_textContent.c_str(), pattern, startPos ) );
}

long TextContent::ReplaceText( LPCTSTR pattern, LPCTSTR replacement, BOOL caseSensitive )
{
	if ( caseSensitive )
		return static_cast<long>( str::Replace<str::Case>( &m_textContent, pattern, replacement ) );
	else
		return static_cast<long>( str::Replace<str::IgnoreCase>( &m_textContent, pattern, replacement ) );
}

// Adds embedded content to already existing one by replacing the 'matchCoreID'
// occurrence (if any), otherwise by appending the new content
BOOL TextContent::AddEmbeddedContent( LPCTSTR matchCoreID, LPCTSTR embeddedContent, BOOL caseSensitive )
{
	if ( ReplaceText( matchCoreID, embeddedContent, caseSensitive ) > 0 )
		return TRUE;

	m_textContent += embeddedContent;
	return FALSE;
}

// Returns the next token (if any), otherwise an empty string.
// Semantics: return strtok( content, sep );
BSTR TextContent::Tokenize( LPCTSTR separatorCharSet )
{
	m_tokenizedBuffer.assign( m_textContent.c_str(), m_textContent.c_str() + m_textContent.length() + 1 );	// include terminating zero
	m_tokenizedSeps = separatorCharSet;

	m_currToken = _tcstok( &m_tokenizedBuffer.front(), m_tokenizedSeps.c_str() );

	return str::AllocSysString( m_currToken );
}

// Returns the next token (if any), otherwise an empty string.
// Semantics: return strtok( nullptr, sep );
BSTR TextContent::GetNextToken( void )
{
	if ( !m_currToken.empty() )
		m_currToken = _tcstok( nullptr, m_tokenizedSeps.c_str() );

	return str::AllocSysString( m_currToken );
}

long TextContent::MultiLinesToSingleParagraph( LPCTSTR multiLinesText, BOOL doTrimTrailingSpaces )
{
	int lineCount = 0;

	m_textContent.clear();

	if ( multiLinesText != nullptr && multiLinesText[ 0 ] != '\0' )
	{
		std::auto_ptr<TCHAR> textBuffer( new TCHAR[ _tcslen( multiLinesText ) + 1 ] );

		if ( textBuffer.get() != nullptr )
		{
			_tcscpy( textBuffer.get(), multiLinesText );

			static const TCHAR* trimSeps = _T(" \t");
			static const TCHAR* seps = _T("\r\n");
			TCHAR* lineToken = _tcstok( textBuffer.get(), seps );
			CString line;

			while ( lineToken != nullptr )
			{
				++lineCount;
				line = lineToken;

				if ( doTrimTrailingSpaces )
				{
					line.TrimLeft( trimSeps );
					line.TrimRight( trimSeps );
				}
				if ( !line.IsEmpty() )
				{
					if ( doTrimTrailingSpaces && !m_textContent.empty() )
						m_textContent += _T(" ");

					m_textContent += line;
				}
				else
					TRACE( _T("Ignoring empty m_line!\n") );

				lineToken = _tcstok( nullptr, seps );
			}
		}
		else
			TRACE( _T("Cannot allocate a duplicate buffer for tokenizing!\n") );
	}

	if ( !m_textContent.empty() )
		m_textContent += _T("\r\n");

	return lineCount;
}

void TextContent::AddFieldReplacement( LPCTSTR fieldText, LPCTSTR replaceWith )
{
	m_fieldReplacements[ fieldText ] = replaceWith;
}

void TextContent::ClearFieldReplacements( void )
{
	m_fieldReplacements.clear();
}

BSTR TextContent::FormatTimestamp( DATE timestamp, LPCTSTR strftimeFormat )
{
	const TCHAR* format = strftimeFormat != nullptr && strftimeFormat[ 0 ] != '\0' ? strftimeFormat : _T("%d-%b-%Y");
	CString timestampAsString = COleDateTime( timestamp ).Format( format );

	return timestampAsString.AllocSysString();
}

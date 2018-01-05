
#include "stdafx.h"
#include "TextContent.h"
#include "StringUtilitiesEx.h"
#include "CompoundTextParser.h"
#include <memory>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


IMPLEMENT_DYNCREATE(TextContent, CCmdTarget)


TextContent::TextContent( void )
	: CCmdTarget()
	, m_TextContent()
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
	CCmdTarget::OnFinalRelease();
}


// message handlers

BEGIN_MESSAGE_MAP(TextContent, CCmdTarget)
	//{{AFX_MSG_MAP(TextContent)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


BEGIN_DISPATCH_MAP(TextContent, CCmdTarget)
	//{{AFX_DISPATCH_MAP(TextContent)
	DISP_PROPERTY_NOTIFY(TextContent, "ShowErrors", m_showErrors, OnShowErrorsChanged, VT_BOOL)
	DISP_PROPERTY_EX(TextContent, "Text", GetText, SetText, VT_BSTR)
	DISP_PROPERTY_EX(TextContent, "TextLen", GetTextLen, SetNotSupported, VT_I4)
	DISP_FUNCTION(TextContent, "LoadFile", LoadFile, VT_BOOL, VTS_BSTR)
	DISP_FUNCTION(TextContent, "LoadFileSection", LoadFileSection, VT_BOOL, VTS_BSTR VTS_BSTR)
	DISP_FUNCTION(TextContent, "LoadCompoundFileSections", LoadCompoundFileSections, VT_I4, VTS_BSTR VTS_BSTR VTS_BOOL)
	DISP_FUNCTION(TextContent, "FindText", FindText, VT_I4, VTS_BSTR VTS_I4 VTS_BOOL)
	DISP_FUNCTION(TextContent, "ReplaceText", ReplaceText, VT_I4, VTS_BSTR VTS_BSTR VTS_BOOL)
	DISP_FUNCTION(TextContent, "AddEmbeddedContent", AddEmbeddedContent, VT_BOOL, VTS_BSTR VTS_BSTR VTS_BOOL)
	DISP_FUNCTION(TextContent, "Tokenize", Tokenize, VT_BSTR, VTS_BSTR)
	DISP_FUNCTION(TextContent, "GetNextToken", GetNextToken, VT_BSTR, VTS_NONE)
	DISP_FUNCTION(TextContent, "MultiLinesToSingleParagraph", MultiLinesToSingleParagraph, VT_I4, VTS_BSTR VTS_BOOL)
	DISP_FUNCTION(TextContent, "AddFieldReplacement", AddFieldReplacement, VT_EMPTY, VTS_BSTR VTS_BSTR)
	DISP_FUNCTION(TextContent, "ClearFieldReplacements", ClearFieldReplacements, VT_EMPTY, VTS_NONE)
	DISP_FUNCTION(TextContent, "FormatTimestamp", FormatTimestamp, VT_BSTR, VTS_DATE VTS_BSTR)
	//}}AFX_DISPATCH_MAP
END_DISPATCH_MAP()

// Note: we add support for IID_ITextContent to support typesafe binding
//  from VBA.  This IID must match the GUID that is attached to the
//  dispinterface in the .ODL file.

// {E37FE176-CBB7-11D4-B57C-00D0B74ECB52}
static const IID IID_ITextContent =
{ 0xe37fe176, 0xcbb7, 0x11d4, { 0xb5, 0x7c, 0x0, 0xd0, 0xb7, 0x4e, 0xcb, 0x52 } };

BEGIN_INTERFACE_MAP(TextContent, CCmdTarget)
	INTERFACE_PART(TextContent, IID_ITextContent, Dispatch)
END_INTERFACE_MAP()

// {E37FE177-CBB7-11D4-B57C-00D0B74ECB52}
IMPLEMENT_OLECREATE(TextContent, "IDETools.TextContent", 0xe37fe177, 0xcbb7, 0x11d4, 0xb5, 0x7c, 0x0, 0xd0, 0xb7, 0x4e, 0xcb, 0x52)


// message handlers

BSTR TextContent::GetText( void )
{
	// Make a full copy of m_TextContent before returning it. This is because conversion to BSTR (AllocSysString())
	// may set the length of the BSTR incorrectly!!!
	return CString( (LPCTSTR)m_TextContent ).AllocSysString();
}

void TextContent::SetText( LPCTSTR lpszNewValue )
{
	m_TextContent = lpszNewValue;
}

long TextContent::GetTextLen()
{
	return 0;
}

void TextContent::OnShowErrorsChanged( void )
{
}

BOOL TextContent::LoadFile( LPCTSTR textFilePath )
{
	m_TextContent.Empty();

	try
	{
		CStdioFile file( textFilePath, CFile::modeRead | CFile::typeBinary );
		TCHAR lineBuffer[ 2048 ];

		// Read each line and append it to text content member:
		while ( file.ReadString( lineBuffer, COUNT_OF( lineBuffer ) ) != NULL )
			m_TextContent += lineBuffer;
	}
	catch ( CException* exc )
	{
		if ( m_showErrors )
			exc->ReportError();
		exc->Delete();
		return FALSE;
	}
	return TRUE;
}

BOOL TextContent::LoadFileSection( LPCTSTR compoundFilePath, LPCTSTR sectionName )
{
	CompoundTextParser textParser( compoundFilePath, m_fieldReplacements );

	m_TextContent.Empty();
	if ( textParser.parseFile() )
	{
		textParser.makeFieldReplacements();
		m_TextContent = textParser.getSectionContent( sectionName );
	}
	return !m_TextContent.IsEmpty();
}

// Loads all the existing sections in the specified compound text file.
// Sections are lines into the text content that can be tokenized by "\r\n".
long TextContent::LoadCompoundFileSections( LPCTSTR compoundFilePath, LPCTSTR sectionFilter,
											BOOL caseSensitive )
{
	int				sectionCount = 0;

	m_TextContent.Empty();
	try
	{
		CStdioFile		file( compoundFilePath, CFile::modeRead | CFile::typeBinary );
		SectionParser	sectionParser( NULL, _T("[["), _T("]]"), _T("EOS") );
		TCHAR			lineBuffer[ 4096 ];

		// Read each line in order to find all the section names in the file:
		while ( file.ReadString( lineBuffer, COUNT_OF( lineBuffer ) ) )
			sectionParser.extractSection( lineBuffer, sectionFilter,
										  caseSensitive ? str::Case : str::IgnoreCase, _T("\r\n") );
		m_TextContent = sectionParser.textContent;
	}
	catch ( CException* exc )
	{
		if ( m_showErrors )
			exc->ReportError();
		exc->Delete();
		return FALSE;
	}

	return sectionCount;
}

long TextContent::FindText( LPCTSTR match, long startPos, BOOL caseSensitive )
{
	return (long)str::findStringPos( m_TextContent, match, (int)startPos, caseSensitive ? str::Case : str::IgnoreCase ).m_start;
}

long TextContent::ReplaceText( LPCTSTR match, LPCTSTR replacement, BOOL caseSensitive )
{
	return (long)str::stringReplace( m_TextContent, match, replacement, caseSensitive ? str::Case : str::IgnoreCase );
}

// Adds embedded content to already existing one by replacing the 'matchCoreID'
// occurrence (if any), otherwise by appending the new content
BOOL TextContent::AddEmbeddedContent( LPCTSTR matchCoreID, LPCTSTR embeddedContent, BOOL caseSensitive )
{
	if ( str::stringReplace( m_TextContent, matchCoreID, embeddedContent, caseSensitive ? str::Case : str::IgnoreCase ) > 0 )
		return TRUE;
	m_TextContent += embeddedContent;
	return FALSE;
}

// Returns the next token (if any), otherwise an empty string.
// Semantics: return strtok( content, sep );
BSTR TextContent::Tokenize( LPCTSTR separatorCharSet )
{
	// Avoid refcount on copy since we'll modify the string buffer -> force a true copy
	tokenizedBuffer = LPCTSTR( m_TextContent );
	tokenizedSeps = separatorCharSet;
	currToken = _tcstok( (LPTSTR)(LPCTSTR)tokenizedBuffer, tokenizedSeps );
	return currToken.AllocSysString();
}

// Returns the next token (if any), otherwise an empty string.
// Semantics: return strtok( NULL, sep );
BSTR TextContent::GetNextToken( void )
{
	if ( !currToken.IsEmpty() )
		currToken = _tcstok( NULL, tokenizedSeps );
	return currToken.AllocSysString();
}

long TextContent::MultiLinesToSingleParagraph( LPCTSTR multiLinesText, BOOL doTrimTrailingSpaces )
{
	int lineCount = 0;

	m_TextContent.Empty();

	if ( multiLinesText != NULL && multiLinesText[ 0 ] != _T('\0') )
	{
		std::auto_ptr< TCHAR >	textBuffer( new TCHAR[ _tcslen( multiLinesText ) + 1 ] );

		if ( textBuffer.get() != NULL )
		{
			_tcscpy( textBuffer.get(), multiLinesText );

			static const TCHAR* trimSeps = _T(" \t");
			static const TCHAR* seps = _T("\r\n");
			TCHAR*			lineToken = _tcstok( textBuffer.get(), seps );
			CString			line;

			while ( lineToken != NULL )
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
					if ( doTrimTrailingSpaces && !m_TextContent.IsEmpty() )
						m_TextContent += _T(" ");
					m_TextContent += line;
				}
				else
					TRACE( _T("Ignoring empty line!\n") );

				lineToken = _tcstok( NULL, seps );
			}
		}
		else
			TRACE( _T("Cannot allocate a duplicate buffer for tokenizing!\n") );
	}
	if ( !m_TextContent.IsEmpty() )
		m_TextContent += _T("\r\n");

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
	const TCHAR* format = strftimeFormat != NULL && strftimeFormat[ 0 ] != _T('\0') ? strftimeFormat : _T("%d-%b-%Y");
	CString timestampAsString = COleDateTime( timestamp ).Format( format );

	return timestampAsString.AllocSysString();
}

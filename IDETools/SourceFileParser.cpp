
#include "pch.h"
#include "SourceFileParser.h"
#include "IncludeNode.h"
#include "SearchPathEngine.h"
#include "utl/Algorithms.h"
#include "utl/ContainerOwnership.h"
#include "utl/TokenIterator.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/TextFileIo.hxx"


CSourceFileParser::CSourceFileParser( const fs::CPath& rootFilePath )
	: m_rootFilePath( path::MakeCanonical( rootFilePath.GetPtr() ) )
	, m_fileType( ft::FindFileType( m_rootFilePath.GetPtr() ) )
	, m_localDirPath( m_rootFilePath.GetParentPath( false ) )
{
	AddSourceFile( m_rootFilePath );
}

CSourceFileParser::~CSourceFileParser()
{
	ClearIncludeNodes();
}

void CSourceFileParser::ClearIncludeNodes( void )
{
	utl::ClearOwningContainer( m_includeNodes );
}

void CSourceFileParser::AddSourceFile( const fs::CPath& sourceFilePath )
{
	CIncludeTag sourceTag( sourceFilePath.GetPtr(), true );
	AddIncludeFile( sourceTag, 0 );
}

void CSourceFileParser::RemoveDuplicates( void )
{
	std::vector<CIncludeNode*> duplicates;
	utl::Uniquify<pred::TLess_TreeItemPath>( m_includeNodes, &duplicates );
	utl::ClearOwningContainer( duplicates );
}

void CSourceFileParser::ParseRootFile( int maxParseLines /*= 1000*/ )
{
	ClearIncludeNodes();
	if ( !IsValidFile() )
		return;

	try
	{
		io::CTextFileParser<std::tstring> parser( this );
		parser.SetMaxLineCount( maxParseLines );
		parser.ParseFile( m_rootFilePath );
	}
	catch ( std::exception& exc )
	{
		exc;
		TRACE( _T("# Error: %s\n"), str::FromUtf8( exc.what() ).c_str() );
	}

	RemoveDuplicates();
}

bool CSourceFileParser::OnParseLine( const std::tstring& line, unsigned int lineNo )
{
	CIncludeTag includeTag;
	if ( ParseIncludeStatement( &includeTag, line ) )
		AddIncludeFile( includeTag, lineNo );

	return true;
}

// include/import directive syntax can be:
//	C++:
//		#include <fname.ext>
//		#include "fname.ext"
//		#	include "fname.ext"
//	IDL:
//		import "oaidl.idl";
//		importlib( "stdole32.tlb" );
//
bool CSourceFileParser::ParseIncludeStatement( CIncludeTag* pIncludeTag, const std::tstring& line )
{
	ASSERT_PTR( pIncludeTag );

	str::CTokenIterator<> it( line );
	if ( it.SkipWhiteSpace().AtEnd() )
		return false;

	if ( it.Matches( _T("//") ) )
		return false;									// ignore commented line
	else if ( it.Matches( _T("/*") ) )					// skip "/* some comment */"
	{
		if ( !it.FindToken( _T("*/") ) )
			return false;								// comment not ended on current line
		if ( it.SkipWhiteSpace().AtEnd() )
			return false;
	}

	bool isCppPreprocessor = false;
	if ( it.Matches( _T('#') ) )
	{
		isCppPreprocessor = true;
		if ( it.SkipWhiteSpace().AtEnd() )
			return false;
	}
	else if ( !IsIDLFile() )
		return false;									// not a preprocessor directive in a C++ source file

	const TCHAR* pDirective = nullptr;
	const TCHAR* pToken = nullptr;
	if ( !isCppPreprocessor && IsIDLFile() )			// special case for IDLs: test for importlib or import directives
		if ( it.Matches( pToken = _T("importlib") ) )
		{
			pDirective = pToken;
			if ( !it.FindToken( _T("(") ) )
				return false;
			if ( it.SkipWhiteSpace().AtEnd() )
				return false;
		}

	if ( nullptr == pDirective && !it.AtEnd() )			// check for import directive (used both in C++ and IDL syntax)
		if ( it.Matches( pToken = _T("import") ) )
		{
			pDirective = pToken;
			if ( it.SkipWhiteSpace().AtEnd() )
				return false;
		}

	if ( nullptr == pDirective && !it.AtEnd() )			// check for include directive (used both in C++ and IDL syntax)
		if ( it.Matches( pToken = _T("include") ) )
		{
			pDirective = pToken;
			if ( it.SkipWhiteSpace().AtEnd() )
				return false;
		}

	if ( nullptr == pDirective )
		return false;

	pIncludeTag->Setup( it.CurrentPtr() );
	pIncludeTag->SetIncludeDirectiveFormat( pDirective, isCppPreprocessor );
	return !pIncludeTag->IsEmpty();
}

CIncludeNode* CSourceFileParser::AddIncludeFile( const CIncludeTag& includeTag, int lineNo )
{
	// returns the full path for the file specified by includeTag if file successfully located (empty string if not)
	inc::TSearchFlags searchFlags = inc::Mask_AllIncludePaths;

	switch ( includeTag.GetFileType() )
	{
		case ft::TLB:
		case ft::DLL:
			searchFlags = inc::Flag_BinaryPath;		// search for type libraries only in binary path
			break;
	}

	inc::CSearchPathEngine searchEngine( m_localDirPath, searchFlags );
	inc::TPathLocPair foundFile = searchEngine.FindFirstIncludeFile( includeTag );
	if ( foundFile.first.IsEmpty() )
	{
		//TRACE( _T(" - include file not found in the specified path: %s\n"), includeTag.GetSafeFileName().c_str() );
		return nullptr;
	}
	CIncludeNode* pItem = new CIncludeNode( includeTag, fs::CPath( foundFile.first ), foundFile.second, lineNo );
	m_includeNodes.push_back( pItem );
	return pItem;
}

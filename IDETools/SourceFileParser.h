#ifndef SourceFileParser_h
#define SourceFileParser_h
#pragma once

#include "utl/Path.h"
#include "utl/TextFileParser.h"
#include "FileType.h"
#include "IncludeTag.h"


class CSearchPathEngine;
struct CIncludeNode;


struct CSourceFileParser : private ILineParserCallback
{
	CSourceFileParser( const CSearchPathEngine* pSearchPath, const fs::CPath& rootFilePath );
	~CSourceFileParser();

	bool IsValidFile( void ) const { return m_rootFilePath.FileExist( fs::Read ); }
	void AddSourceFile( const fs::CPath& sourceFilePath );

	void ParseRootFile( int m_maxParseLines = 1000 );

	const std::vector< CIncludeNode* >& GetIncludeNodes( void ) const { return m_includeNodes; }
	void SwapIncludeNodes( std::vector< CIncludeNode* >& rIncludeNodes ) { rIncludeNodes.swap( m_includeNodes ); }		// called gets ownership
	void ClearIncludeNodes( void );
private:
	// ILineParserCallback interface
	virtual bool OnParseLine( const std::tstring& line, unsigned int lineNo );

	bool ParseIncludeStatement( CIncludeTag* pIncludeTag, const std::tstring& line );
	CIncludeNode* AddIncludeFile( const CIncludeTag& includeTag, int lineNo );
	void RemoveDuplicates( void );

	bool IsIDLFile( void ) const { return ft::IDL == m_fileType; }
private:
	const CSearchPathEngine* m_pSearchPath;
	fs::CPath m_rootFilePath;							// "D:\My\Tools\Iterable.h"
	ft::FileType m_fileType;
	std::tstring m_localDirPath;						// "D:\My\Tools"
	std::vector< CIncludeNode* > m_includeNodes;			// has ownership
};


#endif // SourceFileParser_h

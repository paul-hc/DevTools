#ifndef SourceFileParser_h
#define SourceFileParser_h
#pragma once

#include "utl/Path.h"
#include "utl/TextFileIo.h"
#include "FileType.h"
#include "IncludeTag.h"


struct CIncludeNode;


struct CSourceFileParser : private io::ILineParserCallback< std::tstring >
{
	CSourceFileParser( const fs::CPath& rootFilePath );
	~CSourceFileParser();

	bool IsValidFile( void ) const { return m_rootFilePath.FileExist( fs::Read ); }
	void AddSourceFile( const fs::CPath& sourceFilePath );

	template< typename Container >
	void AddSourceFiles( const Container& srcPaths )
	{
		std::for_each( srcPaths.begin(), srcPaths.end(), std::bind( &CSourceFileParser::AddSourceFile, this, std::placeholders::_1 ) );
	}

	void ParseRootFile( int m_maxParseLines = 1000 );

	const std::vector< CIncludeNode* >& GetIncludeNodes( void ) const { return m_includeNodes; }
	void SwapIncludeNodes( std::vector< CIncludeNode* >& rIncludeNodes ) { rIncludeNodes.swap( m_includeNodes ); }		// called gets ownership
	void ClearIncludeNodes( void );
private:
	// io::ILineParserCallback interface
	virtual bool OnParseLine( const std::tstring& line, unsigned int lineNo );

	bool ParseIncludeStatement( CIncludeTag* pIncludeTag, const std::tstring& line );
	CIncludeNode* AddIncludeFile( const CIncludeTag& includeTag, int lineNo );
	void RemoveDuplicates( void );

	bool IsIDLFile( void ) const { return ft::IDL == m_fileType; }
private:
	fs::CPath m_rootFilePath;							// "D:\My\Tools\Iterable.h"
	ft::FileType m_fileType;
	fs::CPath m_localDirPath;							// "D:\My\Tools"
	std::vector< CIncludeNode* > m_includeNodes;		// has ownership
};


#endif // SourceFileParser_h

#ifndef Directory_h
#define Directory_h
#pragma once

#include <iosfwd>


struct CCmdLineOptions;
class CTreeGuides;
class CTextCell;


class CDirectory : private utl::noncopyable
{
	CDirectory( const CDirectory* pParent, const fs::CPath& subDirPath );
	CDirectory( const CDirectory* pParent, const CTextCell* pTableFolder );

	void ListDir( std::wostream& os, const CTreeGuides& guideParts, const std::wstring& parentNodePrefix );
	void ListTableFolder( std::wostream& os, const CTreeGuides& guideParts, const std::wstring& parentNodePrefix );
public:
	CDirectory( const CCmdLineOptions& options );		// root node constructor

	void ListContents( std::wostream& os, const CTreeGuides& guideParts );

	static double GetTotalElapsedEnum( void ) { return s_totalElapsedEnum; }
private:
	const CCmdLineOptions& m_options;

	const fs::CPath& m_dirPath;
	const CTextCell* m_pTableFolder;
	size_t m_depth;

	static const TCHAR s_wildSpec[];
	static double s_totalElapsedEnum;		// total elapsed time in seconds spent enumerating files
	static fs::CPath s_nullPath;
};


#endif // Directory_h

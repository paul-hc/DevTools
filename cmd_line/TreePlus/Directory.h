#ifndef Directory_h
#define Directory_h
#pragma once

#include <iosfwd>


struct CCmdLineOptions;
class CTreeGuides;


class CDirectory : private utl::noncopyable
{
	CDirectory( const CDirectory* pParent, const fs::CPath& subDirPath );

	void List( std::wostream& os, const CTreeGuides& guideParts, const std::wstring& parentNodePrefix );
public:
	CDirectory( const CCmdLineOptions& options );		// root node constructor

	void ListContents( std::wostream& os, const CTreeGuides& guideParts ) { List( os, guideParts, std::wstring() ); }
private:
	const CCmdLineOptions& m_options;

	const fs::CPath& m_dirPath;
	size_t m_depth;

	static const TCHAR s_wildSpec[];
};


#endif // Directory_h

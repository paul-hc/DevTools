#ifndef IncludeDirectories_h
#define IncludeDirectories_h
#pragma once

#include "utl/vector_map.h"
#include "IncludePaths.h"


class CIncludeDirectories : private utl::noncopyable
{
	CIncludeDirectories( void );
public:
	CIncludeDirectories( const CIncludeDirectories& right ) { Assign( right ); }
	~CIncludeDirectories() { Clear(); }

	void Assign( const CIncludeDirectories& right );
public:
	static CIncludeDirectories& Instance( void );
	static bool Created( void ) { return s_created; }

	void Reset( void );

	void Load( void );
	void Save( void ) const;

	const utl::vector_map<std::tstring, CIncludePaths*>& Get( void ) const { return m_includePaths; }
	CIncludePaths* Add( const std::tstring& setName, bool doSelect = true );

	void RemoveCurrent( void );
	void RenameCurrent( const std::tstring& newSetName );

	const CIncludePaths* GetCurrentPaths( void ) const;
	CIncludePaths* RefCurrentPaths( void );

	size_t GetCurrentPos( void ) const { return m_currSetPos; }
	void SetCurrentPos( size_t currSetPos );

	const std::tstring& GetCurrentName( void ) const { ASSERT( m_currSetPos != utl::npos ); return m_includePaths.at( m_currSetPos ).first; }

	const std::vector<inc::TDirSearchPair>& GetSearchSpecs( void ) const;
private:
	void Clear( void );
private:
	utl::vector_map<std::tstring, CIncludePaths*> m_includePaths;		// set-name to include paths
	size_t m_currSetPos;
	mutable std::vector<inc::TDirSearchPair> m_searchSpecs;			// self-encapsulated cache of search specs

	static const std::tstring s_defaultSetName;
	static bool s_created;
};


#endif // IncludeDirectories_h

#ifndef IncludePaths_h
#define IncludePaths_h
#pragma once

#include "PathGroup.h"


struct CIncludePaths
{
public:
	CIncludePaths( void );

	void InitFromIde( void );			// for the app singleton

	void Load( const TCHAR section[] );
	void Save( const TCHAR section[] ) const;

	// from environment variables
	static const inc::CDirPathGroup& Get_INCLUDE( void );
	static const inc::CDirPathGroup& Get_LIB( void );
	static const inc::CDirPathGroup& Get_PATH( void );
public:
	inc::CDirPathGroup m_standard;			// INCLUDE path directories + INCLUDE environment variable
	inc::CDirPathGroup m_source;			// SOURCE path directories
	inc::CDirPathGroup m_library;			// LIBRARY path directories + LIB environment variable
	inc::CDirPathGroup m_binary;			// BINARY path directories
};


#endif // IncludePaths_h

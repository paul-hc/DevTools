#ifndef IncludePaths_h
#define IncludePaths_h
#pragma once

#include "PathGroup.h"


class CIncludePaths
{
public:
	CIncludePaths( void );
	~CIncludePaths();

	void InitFromIde( void );			// for the app singleton

	const inc::CDirPathGroup& GetStandard( void ) const { return m_standard; }
	const inc::CDirPathGroup& GetSource( void ) const { return m_source; }
	const inc::CDirPathGroup& GetLibrary( void ) const { return m_library; }
	const inc::CDirPathGroup& GetBinary( void ) const { return m_binary; }

	// from environment variables
	static const inc::CDirPathGroup& Get_INCLUDE( void );
	static const inc::CDirPathGroup& Get_LIB( void );
	static const inc::CDirPathGroup& Get_PATH( void );
private:
	inc::CDirPathGroup m_standard;			// INCLUDE path directories + INCLUDE environment variable
	inc::CDirPathGroup m_source;			// SOURCE path directories
	inc::CDirPathGroup m_library;			// LIBRARY path directories + LIB environment variable
	inc::CDirPathGroup m_binary;			// BINARY path directories
};


#endif // IncludePaths_h

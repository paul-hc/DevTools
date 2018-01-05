#ifndef IncludeOptions_h
#define IncludeOptions_h
#pragma once

#include "utl/EnumTags.h"
#include "PublicEnums.h"


struct CIncludeOptions
{
	CIncludeOptions( void );
	~CIncludeOptions();

	void ReadProfile( void );
	void WriteProfile( void ) const;

	void SetupArrayExclude( const TCHAR* pFnExclude, const TCHAR* pSep );
	void SetupArrayMore( const TCHAR* pFnMore, const TCHAR* pSep );

	static const CEnumTags& GetTags_DepthLevel( void );
	static const CEnumTags& GetTags_ViewMode( void );
	static const CEnumTags& GetTags_Ordering( void );
public:
	// persistent options
	ViewMode m_viewMode;
	Ordering m_ordering;
	int m_maxNestingLevel;
	int m_maxParseLines;
	bool m_noDuplicates;
	bool m_openBlownUp;
	bool m_selRecover;
	bool m_lazyParsing;
	std::tstring m_lastBrowsedFile;
	std::tstring m_fnExclude, m_fnMore;
public:
	std::vector< std::tstring > m_fnArrayExclude;
	std::vector< std::tstring > m_fnArrayMore;
public:
	static const TCHAR m_sectionName[];
};


#endif // IncludeOptions_h

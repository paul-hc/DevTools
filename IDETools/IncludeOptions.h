#ifndef IncludeOptions_h
#define IncludeOptions_h
#pragma once

#include "utl/EnumTags.h"
#include "PublicEnums.h"
#include "DirPathGroup.h"


struct CIncludeOptions
{
private:
	CIncludeOptions( void );
	~CIncludeOptions();
public:
	static CIncludeOptions& Instance( void );

	void Load( void );
	void Save( void ) const;

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
	fs::CPath m_lastBrowsedFile;

	fs::TFilePathGroup m_fnIgnored;					// "Exclude files": force out
	fs::TFilePathGroup m_fnAdded;					// "Also include files": force in
	inc::CDirPathGroup m_additionalIncludePath;
};


#endif // IncludeOptions_h

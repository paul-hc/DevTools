#ifndef Application_fwd_h
#define Application_fwd_h
#pragma once


class CModuleSession;
struct CIncludePaths;


namespace code { class CFormatterOptions; }


namespace app
{
	enum ContextPopup { IncludeTreePopup, IncludeTree_NoDupsPopup, MenuBrowserOptionsPopup, FileSortOrderPopup, ProfileListContextPopup, FoundListContextPopup, AutoMakeCodePopup, _UnusedPopup };


	CModuleSession& GetModuleSession( void );
	const code::CFormatterOptions& GetCodeFormatterOptions( void );
	const CIncludePaths* GetIncludePaths( void );

	UINT GetMenuVertSplitCount( void );
}


#endif // Application_fwd_h

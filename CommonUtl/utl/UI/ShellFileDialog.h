#ifndef ShellFileDialog_h
#define ShellFileDialog_h
#pragma once

#include "FilterStore.h"


namespace fs
{
	namespace impl
	{
		struct CFilterData
		{
			CFilterData( const CFilterJoiner* pFilterJoiner, shell::BrowseMode browseMode )
				: m_pFilterJoiner( pFilterJoiner )
				, m_filters( m_pFilterJoiner->MakeFilters( browseMode ) )
			{
				ASSERT_PTR( m_pFilterJoiner );
			}
		public:
			const CFilterJoiner* m_pFilterJoiner;
			std::tstring m_filters;
		};
	}
}


class CShellFileDialog : private fs::impl::CFilterData
					   , public CFileDialog
{
public:
	CShellFileDialog( shell::BrowseMode browseMode, const TCHAR* pFilePath, const fs::CFilterJoiner* pFilterJoiner = &GetDefaultJoiner(),
					  DWORD flags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, const TCHAR* pDefaultExt = NULL,
					  CWnd* pParentWnd = NULL, const TCHAR* pTitle = NULL );
	virtual ~CShellFileDialog();

	bool RunModal( fs::CPath* pFilePath );
	bool RunModal( std::vector< fs::CPath >& rFilePaths );
private:
	static const fs::CFilterJoiner& GetDefaultJoiner( void );			// "All files (*.*)"
	static DWORD GetFlags( shell::BrowseMode browseMode, DWORD flags );
	static COMDLG_FILTERSPEC InputSpec( const TCHAR*& rpFilter );

	// filter index (1-based)
	DWORD FindFilterIndex( const std::tstring& spec ) const;
	DWORD FindFilterWithExt( const TCHAR* pFilePath ) const;
	DWORD FindMatchingFilterIndex( const std::tstring& spec, const TCHAR* pFilePath ) const;
	COMDLG_FILTERSPEC LookupFilter( DWORD filterIndex ) const;

	// Vista style
	CComPtr< IFileDialog > GetFileDialog( void );
	bool ReplaceExt( std::tstring& rFilePath, const COMDLG_FILTERSPEC& filterSpec ) const;

	static bool IsValidFilterSpec( const COMDLG_FILTERSPEC& filterSpec ) { return filterSpec.pszName != NULL && filterSpec.pszSpec != NULL; }
private:
	enum Flags
	{
		UserTypeChange = BIT_FLAG( 0 )		// for ignoring first OnTypeChange notification, which is not done by user
	};

	int m_flags;
protected:
	// base overrides
	virtual void OnTypeChange( void );
};


#endif // ShellFileDialog_h

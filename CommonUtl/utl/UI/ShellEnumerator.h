#ifndef ShellEnumerator_h
#define ShellEnumerator_h
#pragma once

#include "utl/FileEnumerator.h"
#include "ShellTypes.h"


namespace shell
{
	enum ShellPatternResult { ValidItem, ValidFolder, InvalidPattern };

	struct CSearchPatternParts
	{
		CSearchPatternParts( void );

		ShellPatternResult Split( const shell::TPatternPath& searchShellPath );
	public:
		shell::TFolderPath m_folderPath;
		std::tstring m_wildSpec;
		ShellPatternResult m_result;
		bool m_isFileSystem;
	};


	// shell directory/folder path enumeration:

	enum EnumFlags
	{
		DefaultEnumFlags = SHCONTF_FOLDERS | SHCONTF_NONFOLDERS | SHCONTF_FASTITEMS,
		HiddenEnumFlags = DefaultEnumFlags | SHCONTF_INCLUDEHIDDEN
	};


	class CEnumContext
	{
	public:
		CEnumContext( SHCONTF enumFlags = DefaultEnumFlags, HWND hWnd = AfxGetMainWnd()->GetSafeHwnd() )
			: m_enumFlags( enumFlags )
			, m_hWnd( hWnd )
			, m_pWildSpec( s_wildSpec )
		{
		}

		void EnumItems( OUT fs::IEnumerator* pEnumerator, const shell::TFolderPath& folderPath, const TCHAR* pWildSpec = s_wildSpec ) const;
		ShellPatternResult SearchEnumItems( OUT fs::IEnumerator* pEnumerator, const shell::TPatternPath& searchShellPath ) const;

		// shell folder enumeration
		void EnumFolderItems( OUT fs::IEnumerator* pEnumerator, const shell::TFolderPath& folderPath, const TCHAR* pWildSpec = s_wildSpec ) const;
		void EnumFolderItems( OUT fs::IEnumerator* pEnumerator, PCIDLIST_ABSOLUTE folderPidl ) const;
	private:
		void EnumFolder( OUT fs::IEnumerator* pEnumerator, IShellFolder* pFolder, const shell::TFolderPath& folderPath ) const;
	private:
		SHCONTF m_enumFlags;
		HWND m_hWnd;							// owner for EnumObjects() UI
		mutable const TCHAR* m_pWildSpec;		// can be multiple: "*.*", "*.doc;*.txt"
	public:
		static const TCHAR s_wildSpec[];		// _T("*")
	};
}


#endif // ShellEnumerator_h

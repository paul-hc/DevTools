
#include "pch.h"
#include "ShellEnumerator.h"
#include "ShellPidl.h"
#include "utl/FileState.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace shell
{
	static void ReadFileState( OUT fs::CFileState& rNodeState, const shell::TPath& shellPath, SFGAOF nodeAttribs )
	{
		if ( HasFlag( nodeAttribs, SFGAO_FILESYSTEM ) )
			rNodeState.Retrieve( shellPath );
		else
		{
			// set up rNodeState partially:
			rNodeState.m_fullPath = shellPath;

			rNodeState.m_attributes = HasFlag( nodeAttribs, SFGAO_FOLDER ) ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
			SetFlag( rNodeState.m_attributes, FILE_ATTRIBUTE_READONLY, HasFlag( nodeAttribs, SFGAO_READONLY ) );
			SetFlag( rNodeState.m_attributes, FILE_ATTRIBUTE_HIDDEN, HasFlag( nodeAttribs, SFGAO_HIDDEN ) );
			SetFlag( rNodeState.m_attributes, FILE_ATTRIBUTE_SYSTEM, HasFlag( nodeAttribs, SFGAO_SYSTEM ) );
		}
	}


	// CSearchPatternParts implementation

	CSearchPatternParts::CSearchPatternParts( void )
		: m_wildSpec( _T("*") )
		, m_result( InvalidPattern )
		, m_isFileSystem( false )
	{
	}

	ShellPatternResult CSearchPatternParts::Split( const shell::TPatternPath& searchShellPath )
	{
		*this = CSearchPatternParts();

		SFGAOF shAttribs = GetShellAttributes( searchShellPath.GetPtr() );

		if ( shAttribs != 0 )
		{
			if ( HasFlag( shAttribs, SFGAO_FOLDER ) )
			{	// pattern is directory/folder
				m_folderPath = searchShellPath;
				m_result = ValidFolder;
			}
			else if ( HasFlag( shAttribs, SFGAO_CANLINK ) )
			{	// pattern is file/applet
				m_folderPath = searchShellPath.GetParentPath();
				m_wildSpec = searchShellPath.GetFilename();
				m_result = ValidItem;
			}

			m_isFileSystem = HasFlag( shAttribs, SFGAO_FILESYSTEM );
		}
		else if ( searchShellPath.HasWildcardPattern() )
		{
			m_folderPath = searchShellPath.GetParentPath();
			m_wildSpec = searchShellPath.GetFilename();
		}
		else
		{
			m_folderPath = searchShellPath;
			m_wildSpec.clear();
		}

		if ( InvalidPattern == m_result && 0 == shAttribs )
		{
			shAttribs = GetShellAttributes( m_folderPath.GetPtr() );
			if ( shAttribs != 0 )
			{
				m_result = HasFlag( shAttribs, SFGAO_FOLDER ) ? ValidFolder : ValidItem;
				m_isFileSystem = HasFlag( shAttribs, SFGAO_FILESYSTEM );
			}
		}

		return m_result;
	}
}


namespace shell
{
	// CEnumContext implementation

	const TCHAR CEnumContext::s_wildSpec[] = _T("*");

	void CEnumContext::EnumItems( OUT fs::IEnumerator* pEnumerator, const shell::TFolderPath& folderPath, const TCHAR* pWildSpec /*= s_wildSpec*/ ) const
	{	// pWildSpec can be multiple: "*.*", "*.doc;*.txt"
		if ( fs::IsValidDirectory( folderPath.GetPtr() ) )
			fs::EnumFiles( pEnumerator, folderPath, pWildSpec );		// enumerate the file-system directory
		else
			EnumFolderItems( pEnumerator, folderPath );
	}

	ShellPatternResult CEnumContext::SearchEnumItems( OUT fs::IEnumerator* pEnumerator, const shell::TPatternPath& searchShellPath ) const
	{
		shell::CSearchPatternParts parts;

		parts.Split( searchShellPath );

		switch ( parts.m_result )
		{
			case shell::ValidFolder:
				if ( parts.m_isFileSystem )
					fs::EnumFiles( pEnumerator, parts.m_folderPath, parts.m_wildSpec.c_str() );
				else
					EnumFolderItems( pEnumerator, parts.m_folderPath, parts.m_wildSpec.c_str() );
				break;
			case shell::ValidItem:
				ASSERT( false );		// should've been handled by the if statement
				break;
		}

		return parts.m_result;
	}

	void CEnumContext::EnumFolderItems( OUT fs::IEnumerator* pEnumerator, const shell::TFolderPath& folderPath, const TCHAR* pWildSpec /*= s_wildSpec*/ ) const
	{
		m_pWildSpec = !str::IsEmpty( pWildSpec ) ? pWildSpec : s_wildSpec;

		if ( CComPtr<IShellFolder> pFolder = shell::MakeShellFolder( folderPath.GetPtr() ) )
			EnumFolder( pEnumerator, pFolder, folderPath );
	}

	void CEnumContext::EnumFolderItems( OUT fs::IEnumerator* pEnumerator, PCIDLIST_ABSOLUTE folderPidl ) const
	{
		if ( CComPtr<IShellFolder> pFolder = shell::MakeShellFolder( folderPidl ) )
			EnumFolder( pEnumerator, pFolder, pidl::GetShellPath( folderPidl ) );
	}

	void CEnumContext::EnumFolder( OUT fs::IEnumerator* pEnumerator, IShellFolder* pFolder, const shell::TFolderPath& folderPath ) const
	{
		ASSERT_PTR( pFolder );

		utl::CScopedIncrement depthLevel( pEnumerator->GetDepthCounter() );
		CComPtr<IEnumIDList> pEnum;

		if ( !HR_OK( pFolder->EnumObjects( m_hWnd, m_enumFlags, &pEnum ) ) )
			return;

		typedef std::pair< fs::CPath, CComPtr<IShellFolder> > TNameSubFolderPair;	// use fs::CPath for SIGDN_PARENTRELATIVEEDITING display name for natural sorting (put the name first for debugging)
		std::vector<TNameSubFolderPair> subFolders;
		enum { ShAttribsMask = SFGAO_FILESYSTEM | SFGAO_FOLDER | SFGAO_READONLY | SFGAO_HIDDEN | SFGAO_SYSTEM | SFGAO_LINK };

		CPidlChild childPidl;
		ULONG fetchedCount;

		while ( !pEnumerator->MustStop() && HR_OK( pEnum->Next( 1, &childPidl, &fetchedCount ) ) && fetchedCount != 0 )
			if ( SFGAOF itemAttribs = shell::GetItemAttributes( pFolder, childPidl, ShAttribsMask ) )
			{
				std::tstring filename = childPidl.GetParsingName( pFolder );
				fs::CFileState nodeState;

				shell::ReadFileState( nodeState, folderPath / filename, itemAttribs );

				if ( pEnumerator->CanIncludeNode( nodeState ) )		// item passes the filter?
					if ( HasFlag( itemAttribs, SFGAO_FOLDER ) )
					{
						CComPtr<IShellFolder> pSubFolder;

						if ( SUCCEEDED( pFolder->BindToObject( childPidl, nullptr, IID_PPV_ARGS( &pSubFolder ) ) ) )
							subFolders.push_back( TNameSubFolderPair( childPidl.GetEditingName( pFolder ), pSubFolder ) );
					}
					else
					{
						if ( path::MatchWildcard( nodeState.m_fullPath.GetPtr(), m_pWildSpec ) )
							pEnumerator->OnAddFileInfo( nodeState );
					}
			}

		if ( !pEnumerator->HasEnumFlag( fs::EF_NoSortSubDirs ) )
			std::sort( subFolders.begin(), subFolders.end(), pred::OrderByValue< pred::CompareFirst<pred::CompareValue> >() );		// sort by EditingName in natural path order

		const bool canRecurse = pEnumerator->CanRecurse();

		// progress reporting: ensure the sub-directory (stage) is always displayed first, then the files (items) under it
		for ( std::vector<TNameSubFolderPair>::const_iterator itSubDirPair = subFolders.begin(); itSubDirPair != subFolders.end(); ++itSubDirPair )
			if ( pEnumerator->AddFoundSubDir( shell::GetFolderPath( itSubDirPair->second ) ) )		// sub-directory is not ignored?
				if ( canRecurse && !pEnumerator->MustStop() )
					EnumFolder( pEnumerator, itSubDirPair->second, folderPath );
	}
}

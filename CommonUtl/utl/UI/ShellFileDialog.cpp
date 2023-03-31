
#include "pch.h"
#include "ShellFileDialog.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CShellFileDialog::CShellFileDialog( shell::BrowseMode browseMode, const TCHAR* pFilePath, const fs::CFilterJoiner* pFilterJoiner /*= GetDefaultJoiner()*/,
									DWORD flags /*= OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT*/, const TCHAR* pDefaultExt /*= nullptr*/,
									CWnd* pParentWnd /*= nullptr*/, const TCHAR* pTitle /*= nullptr*/ )
	: fs::impl::CFilterData( pFilterJoiner, browseMode )
	, CFileDialog( browseMode, pDefaultExt, pFilePath, GetFlags( browseMode, flags ), m_filters.c_str(), pParentWnd, 0, shell::s_useVistaStyle )
	, m_flags( 0 )
{
	if ( pTitle != nullptr )
		m_ofn.lpstrTitle = pTitle;

	m_ofn.nFilterIndex = FindMatchingFilterIndex( m_pFilterJoiner->RetrieveSelFilterSpec(), pFilePath );
}

CShellFileDialog::~CShellFileDialog()
{
}

bool CShellFileDialog::RunModal( fs::CPath* pFilePath )
{
	ASSERT_PTR( pFilePath );
	bool okay = IDOK == DoModal();
	if ( okay )
		pFilePath->Set( GetPathName().GetString() );
	return okay;
}

bool CShellFileDialog::RunModal( std::vector<fs::CPath>& rFilePaths )
{
	rFilePaths.clear();
	if ( DoModal() != IDOK )
		return false;

	for ( POSITION pos = GetStartPosition(); pos != nullptr; )
		rFilePaths.push_back( fs::CPath( GetNextPathName( pos ).GetString() ) );

	return true;
}

const fs::CFilterJoiner& CShellFileDialog::GetDefaultJoiner( void )
{
	static const fs::CFilterJoiner allFilesJoiner;
	return allFilesJoiner;
}

DWORD CShellFileDialog::GetFlags( shell::BrowseMode browseMode, DWORD flags )
{
	SetFlag( flags, OFN_NOTESTFILECREATE | OFN_PATHMUSTEXIST | OFN_ENABLESIZING );
	SetFlag( flags, shell::FileOpen == browseMode ? OFN_FILEMUSTEXIST : OFN_OVERWRITEPROMPT );
	return flags;
}

COMDLG_FILTERSPEC CShellFileDialog::InputSpec( const TCHAR*& rpFilter )
{
	COMDLG_FILTERSPEC filterSpec;

	filterSpec.pszName = rpFilter;
	rpFilter += _tcslen( rpFilter ) + 1;		// skip name

	filterSpec.pszSpec = rpFilter;
	rpFilter += _tcslen( rpFilter ) + 1;		// skip spec
	return filterSpec;
}

DWORD CShellFileDialog::FindFilterIndex( const std::tstring& spec ) const
{
	if ( !spec.empty() )
	{
		DWORD filterIndex = 1;			// 1-based
		for ( const TCHAR* pFilter = m_ofn.lpstrFilter; pFilter[ 0 ] != _T('\0'); ++filterIndex )
		{
			COMDLG_FILTERSPEC filterSpec = InputSpec( pFilter );
			if ( spec == filterSpec.pszSpec )
				return filterIndex;
		}
	}
	return 0;							// 0 means default filter index
}

DWORD CShellFileDialog::FindFilterWithExt( const TCHAR* pFilePath ) const
{
	const TCHAR* pExt = path::FindExt( pFilePath );
	if ( !str::IsEmpty( pExt ) )
	{
		DWORD filterIndex = 1;			// 1-based
		for ( const TCHAR* pFilter = m_ofn.lpstrFilter; pFilter[ 0 ] != _T('\0'); ++filterIndex )
		{
			COMDLG_FILTERSPEC filterSpec = InputSpec( pFilter );

			fs::CKnownExtensions extensions;
			extensions.RegisterSpecs( filterSpec.pszSpec, 0 );
			if ( extensions.ContainsExt( pFilePath ) )
				return filterIndex;
		}
	}
	return 0;
}

DWORD CShellFileDialog::FindMatchingFilterIndex( const std::tstring& spec, const TCHAR* pFilePath ) const
{
	DWORD filterIndex = FindFilterWithExt( pFilePath );		// try to find by file extension (SaveAs mode)
	if ( 0 == filterIndex )
		filterIndex = FindFilterIndex( spec );				// not found by current extension: try to find by last selected spec

	return filterIndex;
}

COMDLG_FILTERSPEC CShellFileDialog::LookupFilter( DWORD filterIndex ) const
{
	COMDLG_FILTERSPEC filterSpec = { nullptr, nullptr };

	for ( const TCHAR* pFilter = m_ofn.lpstrFilter; pFilter[ 0 ] != _T('\0'); --filterIndex )
		if ( 1 == filterIndex )			// found: last 1-based index
		{
			filterSpec = InputSpec( pFilter );
			break;
		}
		else
		{
			pFilter += _tcslen( pFilter ) + 1;
			pFilter += _tcslen( pFilter ) + 1;
		}

	return filterSpec;
}

bool CShellFileDialog::ReplaceExt( std::tstring& rFilePath, const COMDLG_FILTERSPEC& filterSpec ) const
{
	if ( !rFilePath.empty() )
	{
		fs::CKnownExtensions extensions;
		extensions.RegisterSpecs( filterSpec.pszSpec, 0 );
		if ( !extensions.IsEmpty() )
			if ( !extensions.ContainsExt( rFilePath.c_str() ) )
			{
				fs::CPathParts parts( rFilePath );
				parts.m_ext = extensions.GetDefaultExt();
				rFilePath = parts.MakePath().Get();
				return true;
			}
	}
	return false;
}

CComPtr<IFileDialog> CShellFileDialog::GetFileDialog( void )
{
	CComPtr<IFileDialog> pFileDialog;
	pFileDialog.Attach( GetIFileOpenDialog() );			// no additional AddRef on ptr ctor

	if ( nullptr == pFileDialog )
		pFileDialog.Attach( GetIFileSaveDialog() );
	return pFileDialog;
}

void CShellFileDialog::OnTypeChange( void )
{
	if ( !HasFlag( m_flags, UserTypeChange ) )
	{
		SetFlag( m_flags, UserTypeChange );
		return;					// ignore first OnTypeChange notification, which is always called on dialog open
	}

	COMDLG_FILTERSPEC selSpec = LookupFilter( m_ofn.nFilterIndex );
	if ( !IsValidFilterSpec( selSpec ) )
		return;

	TRACE( _T("- CShellFileDialog::OnTypeChange() filterIndex=%d  tag=%s  spec=%s\n"), m_ofn.nFilterIndex, selSpec.pszName, selSpec.pszSpec );

	m_pFilterJoiner->StoreSelFilterSpec( selSpec );

	if ( IFileSaveDialog* pSaveDialog = GetIFileSaveDialog() )
	{
		std::tstring fnameExt = GetFileName().GetString();
		if ( ReplaceExt( fnameExt, selSpec ) )
			HR_AUDIT( pSaveDialog->SetFileName( fnameExt.c_str() ) );

		pSaveDialog->Release();
	}
}

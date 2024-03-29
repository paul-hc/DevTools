
#include "pch.h"
#include "FilterStore.h"
#include "Registry.h"
#include "ShellUtilities.h"
#include "ShellFileDialog.h"
#include "StringUtilities.h"
#include "StdHashValue.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace fs
{
	// CKnownExtensions implementation

	size_t CKnownExtensions::FindExtFilterPos( const TCHAR* pFilePath ) const
	{
		CPath ext( path::FindExt( pFilePath ) );
		std::unordered_map<fs::CPath, size_t>::const_iterator itFound = m_knownExts.find( ext );
		return itFound != m_knownExts.end() ? itFound->second : utl::npos;
	}

	std::tstring CKnownExtensions::MakeAllExts( void ) const
	{
		std::tstring allExtensions = m_allSpecs;
		str::Replace( allExtensions, _T("*"), _T("") );				// ".bmp;.dib;.rle"
		return allExtensions;
	}

	void CKnownExtensions::QueryAllExts( std::vector<std::tstring>& rAllExts ) const
	{
		str::Split( rAllExts, MakeAllExts().c_str(), CFilterStore::s_specSep );
	}

	void CKnownExtensions::Register( const std::vector<std::tstring>& exts, size_t filterPos )
	{
		for ( std::vector<std::tstring>::const_iterator itExtension = exts.begin(); itExtension != exts.end(); ++itExtension )
			Register( *itExtension, filterPos );
	}

	bool CKnownExtensions::Register( const std::tstring& item, size_t filterPos )
	{
		ASSERT( !item.empty() );

		CPath ext( path::FindExt( item.c_str() ) );

		if ( m_knownExts.find( ext ) != m_knownExts.end() )		// extension already registered?
		{
			TRACE( _T(" Warning: ignoring extension '%s' that is already registered!\n"), ext.GetPtr() );
			return false;
		}

		m_knownExts[ ext ] = filterPos;

		if ( m_defaultExt.empty() )
			m_defaultExt = ext.Get();

		if ( _T('*') == item[ 0 ] )
			stream::Tag( m_allSpecs, item, CFilterStore::s_specSep );
		else
			stream::Tag( m_allSpecs, std::tstring( _T("*") ) + item, CFilterStore::s_specSep );

		return true;		// registered
	}

	void CKnownExtensions::RegisterSpecs( const std::tstring& specs, size_t filterPos )
	{
		std::vector<std::tstring> exts;
		str::Split( exts, specs.c_str(), CFilterStore::s_specSep );
		Register( exts, filterPos );
	}


	// CFilterStore implementation

	const TFilterPair CFilterStore::s_allFiles( _T("All Files"), _T("*.*") );
	const TCHAR CFilterStore::s_specSep[] = _T(";");
	const TCHAR CFilterStore::s_filterSep[] = _T("|");

	CFilterStore::CFilterStore( const std::tstring& classTag, BrowseFlags browseFlags /*= BrowseBoth*/ )
		: m_classTag( classTag )
		, m_browseFlags( browseFlags )
	{
		CFilterRepository::Instance().Register( this, m_browseFlags );
	}

	CFilterStore::~CFilterStore()
	{
		CFilterRepository::Instance().Unregister( this, m_browseFlags );
	}

	void CFilterStore::AddFilter( const TFilterPair& filter )
	{
		std::vector<std::tstring> extensions;
		str::Split( extensions, filter.second.c_str(), s_specSep );
		m_knownExts.Register( extensions, m_filters.size() );		// register known extension to entry pos

		m_filters.push_back( filter );
	}

	void CFilterStore::StreamFilters( std::tostringstream& oss ) const
	{
		for ( std::vector<TFilterPair>::const_iterator itFilter = m_filters.begin(); itFilter != m_filters.end(); ++itFilter )
			StreamFilter( oss, *itFilter );
	}

	void CFilterStore::StreamFilters( std::tostringstream& oss, const UINT positions[], size_t count ) const
	{
		for ( size_t i = 0; i != count; ++i )
			StreamFilter( oss, m_filters[ positions[ i ] ] );
	}

	void CFilterStore::StreamFilter( std::tostringstream& oss, const TFilterPair& filter )
	{
		// example "BMP Decoder (*.bmp;*.dib;*.rle)|*.bmp;*.dib;*.rle|"
		oss << filter.first << _T(" (") << filter.second << _T(")") << s_filterSep << _T(" ") << filter.second << s_filterSep;
	}

	std::tstring CFilterStore::MakeFilters( void ) const
	{
		std::tostringstream oss;

		StreamFilters( oss );
		StreamClassFilter( oss );
		StreamAllFiles( oss );
		StreamArrayEnd( oss );				// end-of-array separator
		return oss.str();
	}

	std::tstring CFilterStore::MakeFilters( const UINT positions[], size_t count ) const
	{
		std::tostringstream oss;

		StreamFilters( oss, positions, count );
		StreamAllFiles( oss );
		StreamArrayEnd( oss );				// end-of-array separator
		return oss.str();
	}


	// CFilterJoiner implementation

	const TCHAR CFilterJoiner::s_regSection[] = _T("Settings\\FileFilters");

	std::tstring CFilterJoiner::MakeFilters( shell::BrowseMode browseMode ) const
	{
		std::tostringstream oss;
		for ( std::vector<std::tstring>::const_iterator itClassTag = m_classTags.begin(); itClassTag != m_classTags.end(); ++itClassTag )
			if ( CFilterStore* pFilterStore = CFilterRepository::Instance().Lookup( *itClassTag, browseMode ) )
			{
				pFilterStore->StreamFilters( oss );

				if ( browseMode != shell::FileSaveAs )		// combined filers are useful only when loading (we need targeted file types)
					pFilterStore->StreamClassFilter( oss );
			}

		if ( m_classTags.size() > 1 )					// "All File Types" makes sense only for multiple joiner; otherwise we already have the class filter
			CFilterStore::StreamFilter( oss, TFilterPair( _T("All File Types"), MakeSpecs( browseMode ) ) );

		CFilterStore::StreamAllFiles( oss );
		CFilterStore::StreamArrayEnd( oss );				// end-of-array separator
		return oss.str();
	}

	std::tstring CFilterJoiner::MakeSpecs( shell::BrowseMode browseMode ) const
	{
		std::tstring allKnownSpecs;
		for ( std::vector<std::tstring>::const_iterator itClassTag = m_classTags.begin(); itClassTag != m_classTags.end(); ++itClassTag )
			if ( CFilterStore* pFilterStore = CFilterRepository::Instance().Lookup( *itClassTag, browseMode ) )
				stream::Tag( allKnownSpecs, pFilterStore->GetClassSpecs(), CFilterStore::s_specSep );

		return allKnownSpecs;
	}

	bool CFilterJoiner::BrowseFile( fs::CPath& rFilePath, shell::BrowseMode browseMode,
									DWORD flags /*= 0*/, const TCHAR* pDefaultExt /*= nullptr*/,
									CWnd* pParentWnd /*= nullptr*/, const TCHAR* pTitle /*= nullptr*/ ) const
	{
		CShellFileDialog dlg( browseMode, rFilePath.GetPtr(), this, flags, pDefaultExt, pParentWnd, pTitle );
		return dlg.RunModal( &rFilePath );
	}

	const std::tstring& CFilterJoiner::GetKey( void ) const
	{
		if ( m_key.empty() && !m_classTags.empty() )
			m_key = str::Join( m_classTags, CFilterStore::s_filterSep );
		return m_key;
	}

	UINT CFilterJoiner::HashKey( const std::tstring& key )
	{
		return static_cast<UINT>( utl::GetHashValue( key ) );
	}

	std::tstring CFilterJoiner::FormatHashKey( const std::tstring& key )
	{
		return str::Format( _T("filter_%08x"), HashKey( key ) );
	}

	std::tstring CFilterJoiner::RetrieveSelFilterSpec( void ) const
	{
		reg::CKey key( AfxGetApp()->GetSectionKey( s_regSection ) );
		if ( key.IsOpen() )
		{
			std::tstring valueName = FormatHashKey( GetKey() );
			if ( key.HasValue( valueName.c_str() ) )				// was persisted?
				return key.ReadStringValue( valueName.c_str() );
		}
		return std::tstring();
	}

	void CFilterJoiner::StoreSelFilterSpec( const TCHAR* pSelFilterSpec ) const
	{
		// spec is more reliable for persisting current filter, because the tag may differ between Open and Save (e.g. Decoder/Encoder)
		AfxGetApp()->WriteProfileString( s_regSection, FormatHashKey( GetKey() ).c_str(), pSelFilterSpec );
	}


	// CFilterRepository implementation

	CFilterStore* CFilterRepository::Lookup( const std::tstring& classTag, shell::BrowseMode browseMode ) const
	{
		std::unordered_map<std::tstring, TOpenSavePair>::const_iterator itFound = m_stores.find( classTag );
		ASSERT( itFound != m_stores.end() );

		if ( shell::FileSaveAs == browseMode && itFound->second.second != nullptr )
			return itFound->second.second;
		return itFound->second.first;
	}

	CFilterRepository& CFilterRepository::Instance( void )
	{
		static CFilterRepository s_repository;
		return s_repository;
	}

	void CFilterRepository::Register( CFilterStore* pFilterStore, BrowseFlags browseFlags )
	{
		TOpenSavePair& rStorePair = m_stores[ pFilterStore->GetClassTag() ];
		switch ( browseFlags )
		{
			case BrowseOpen:
				ASSERT_NULL( rStorePair.first );			// no <class_tag, browseFlags> collision
				rStorePair.first = pFilterStore;
				break;
			case BrowseSave:
				ASSERT_NULL( rStorePair.second );
				rStorePair.second = pFilterStore;
				break;
			case BrowseBoth:
				ASSERT_NULL( rStorePair.first );
				ASSERT_NULL( rStorePair.second );
				rStorePair.first = rStorePair.second = pFilterStore;
				break;
		}
	}

	void CFilterRepository::Unregister( CFilterStore* pFilterStore, BrowseFlags browseFlags )
	{
		std::unordered_map<std::tstring, TOpenSavePair>::iterator itFound = m_stores.find( pFilterStore->GetClassTag() );
		ASSERT( itFound != m_stores.end() );
		TOpenSavePair& rStorePair = itFound->second;
		switch ( browseFlags )
		{
			case BrowseOpen:	rStorePair.first = nullptr; break;
			case BrowseSave:	rStorePair.second = nullptr; break;
			case BrowseBoth:	rStorePair.first = rStorePair.second = nullptr; break;
		}

		static const TOpenSavePair nullPair( nullptr, nullptr );
		if ( nullPair == itFound->second )
			m_stores.erase( itFound );
	}

} //namespace fs

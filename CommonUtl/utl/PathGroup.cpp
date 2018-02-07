
#include "stdafx.h"
#include "PathGroup.h"
#include "ContainerUtilities.h"
#include "StringUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace fs
{
	const TCHAR CPathGroup::s_sep[] = _T(";");

	CPathGroup::CPathGroup( const TCHAR environVar[] )
	{
		enum { BufferSize = 8192 };
		std::vector< TCHAR > value( BufferSize );

		if ( ::GetEnvironmentVariable( environVar, &value.front(), BufferSize ) != 0 )
			Split( &value.front() );
	}

	void CPathGroup::Clear( void )
	{
		m_specs.clear();
		m_paths.clear();
	}

	bool CPathGroup::Contains( const fs::CPath& path ) const
	{
		return utl::Contains( m_paths, path );
	}

	bool CPathGroup::AnyIsPartOf( const fs::CPath& path ) const
	{
		if ( !path.IsEmpty() )
			for ( std::vector< fs::CPath >::const_iterator itSubPath = m_paths.begin(); itSubPath != m_paths.end(); ++itSubPath )
				if ( path::Contains( path.GetPtr(), itSubPath->GetPtr() ) )
					return true;
				else if ( path::ContainsWildcards( itSubPath->GetPtr() ) )
					if ( path::MatchWildcard( path.GetPtr(), itSubPath->GetPtr() ) )
						return true;

		return false;
	}

	std::tstring CPathGroup::Join( const TCHAR sep[] /*= s_sep*/ ) const
	{
		return str::Join( m_specs.begin(), m_specs.end(), sep );
	}

	void CPathGroup::Split( const TCHAR flat[], const TCHAR delims[] /*= s_sep*/ )
	{
		Clear();
		str::Tokenize( m_specs, flat, delims );			// discard empty entries

		std::for_each( m_specs.begin(), m_specs.end(), path::Normalize );
		utl::RemoveDuplicates< pred::EquivalentPathString >( m_specs );
		ExpandPaths();
	}

	void CPathGroup::Append( const CPathGroup& right )
	{
		std::vector< std::tstring > allSpecs;
		allSpecs.reserve( m_specs.size() + right.m_specs.size() );
		allSpecs = m_specs;
		allSpecs.insert( allSpecs.end(), right.m_specs.begin(), right.m_specs.end() );

		Split( str::Join( m_specs.begin(), m_specs.end(), s_sep ).c_str(), s_sep );
	}

	void CPathGroup::ExpandPaths( void )
	{
		m_paths.clear();
		m_paths.reserve( m_specs.size() );

		for ( std::vector< std::tstring >::const_iterator itSpec = m_specs.begin(); itSpec != m_specs.end(); ++itSpec )
			m_paths.push_back( str::ExpandEnvironmentStrings( itSpec->c_str() ) );
	}

	void CPathGroup::Load( const TCHAR section[], const TCHAR entry[] )
	{
		Split( AfxGetApp()->GetProfileString( section, entry, Join( s_sep ).c_str() ).GetString(), s_sep );
	}

	void CPathGroup::Save( const TCHAR section[], const TCHAR entry[] ) const
	{
		AfxGetApp()->WriteProfileString( section, entry, Join( s_sep ).c_str() );
	}
}

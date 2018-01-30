
#include "stdafx.h"
#include "PathGroup.h"
#include "utl/ContainerUtilities.h"
#include "utl/EnumTags.h"
#include "utl/StringUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace inc
{
	const CEnumTags& GetTags_Location( void )
	{
		static const CEnumTags tags( _T("Include Path|Local Path|Additional Path|Absolute Path|Source Path|Library Path|Binary Path") );
		ASSERT( tags.GetUiTags().size() == ( BinaryPath + 1 ) );
		return tags;
	}
}


const TCHAR CPathGroup::s_sep[] = _T(";");

CPathGroup::CPathGroup( const TCHAR environVar[] )
{
	enum { BufferSize = 8192 };
	std::vector< TCHAR > value( BufferSize );

	if ( ::GetEnvironmentVariable( environVar, &value.front(), BufferSize ) != 0 )
		Store( &value.front() );
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

void CPathGroup::Store( const TCHAR flat[], const TCHAR delims[] /*= s_sep*/ )
{
	str::Tokenize( m_specs, flat, delims );			// discard empty entries

	std::for_each( m_specs.begin(), m_specs.end(), path::Normalize );
	utl::RemoveDuplicates< pred::EquivalentPathString >( m_specs );
	ExpandPaths();
}

void CPathGroup::Add( const TCHAR flat[], const TCHAR delims[] /*= s_sep*/ )
{
	std::vector< std::tstring > newSpecs;
	str::Tokenize( newSpecs, flat, delims );			// discard empty entries
	if ( !newSpecs.empty() )
	{
		std::for_each( newSpecs.begin(), newSpecs.end(), path::Normalize );

		m_specs.insert( m_specs.end(), newSpecs.begin(), newSpecs.end() );
		utl::RemoveDuplicates< pred::EquivalentPathString >( m_specs );
		ExpandPaths();
	}
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
	Store( AfxGetApp()->GetProfileString( section, entry, Join( s_sep ).c_str() ).GetString(), s_sep );
}

void CPathGroup::Save( const TCHAR section[], const TCHAR entry[] ) const
{
	AfxGetApp()->WriteProfileString( section, entry, Join( s_sep ).c_str() );
}

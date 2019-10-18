
#include "stdafx.h"
#include "FileAssoc.h"
#include "utl/FileSystem.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


enum CircularIndex
{
	CI_Invalid = -1,

	CI_H,
	CI_HXX,
	CI_INL,
	CI_HPP,
	CI_CXX,
	CI_TMPL,
	CI_T,
	CI_C,
	CI_CPP,
	CI_LCC,
	CI_OPP,
	CI_PPC,

	CI_SQL,
	CI_TAB,
	CI_PK,
	CI_PKG,
	CI_PKB,
	CI_PKS,
	CI_PAC,
	CI_OT,
	CI_OTB
};


const TCHAR* CFileAssoc::s_circularExt[] =
{
	_T(".h"),		// CI_H
	_T(".hxx"),		// CI_HXX
	_T(".inl"),		// CI_INL
	_T(".hpp"),		// CI_HPP
	_T(".cxx"),		// CI_CXX
	_T(".tmpl"),	// CI_TMPL
	_T(".t"),		// CI_T
	_T(".c"),		// CI_C
	_T(".cpp"),		// CI_CPP
	_T(".lcc"),		// CI_LCC
	_T(".opp"),		// CI_OPP
	_T(".ppc"),		// CI_PPC

	_T(".sql"),		// CI_SQL
	_T(".tab"),		// CI_TAB
	_T(".pk"),		// CI_PK
	_T(".pkg"),		// CI_PKG
	_T(".pkb"),		// CI_PKB
	_T(".pks"),		// CI_PKS
	_T(".pac"),		// CI_PAC
	_T(".ot"),		// CI_OT
	_T(".otb")		// CI_OTB
};


const TCHAR* CFileAssoc::s_relDirs[] =
{
	_T(""),
	_T("..\\include\\"),
	_T("..\\source\\")
};

const fs::CPath CFileAssoc::s_emptyPath;


// CFileAssoc implementation

void CFileAssoc::Clear( void )
{
	m_parts.Clear();
	m_fileType = ft::Unknown;
	m_circularIndex = CI_Invalid;
	m_hasSpecialAssociations = false;
}

bool CFileAssoc::SetPath( const fs::CPath& path )
{
	Clear();
	if ( !path::IsAbsolute( path.GetPtr() ) || !path.FileExist() )
		return false;

	m_parts.SplitPath( path.Get() );
	m_fileType = ft::FindTypeOfExtension( m_parts.m_ext.c_str() );

	// search for known circular index
	m_hasSpecialAssociations = IsResourceHeaderFile( m_parts ) ||
		ft::RC == m_fileType ||
		ft::RES == m_fileType;

	for ( unsigned int i = 0; i != COUNT_OF( s_circularExt ); ++i )
		if ( path::EqualsPtr( m_parts.m_ext.c_str(), s_circularExt[ i ] ) )
		{
			m_circularIndex = static_cast< CircularIndex >( i );
			break;
		}

	return m_circularIndex != CI_Invalid;
}

bool CFileAssoc::IsValidKnownAssoc( void ) const
{
	return IsValid() && ( m_circularIndex != CI_Invalid || m_hasSpecialAssociations );
}

// true if:
//		"resource.h"
//		"XyzRes.h" or "XyzRes.rh" (with complementary "Xyz.rc")
bool CFileAssoc::IsResourceHeaderFile( const fs::CPathParts& parts )
{
	if ( path::Equals( parts.GetNameExt(), _T("resource.h") ) )
		return true;

	if ( path::EqualsPtr( parts.m_ext.c_str(), _T(".h") ) || path::EqualsPtr( parts.m_ext.c_str(), _T(".rh") ) )
	{
		static const TCHAR prefix[] = _T("Res");
		const size_t prefixLen = str::GetLength( prefix );

		if ( str::EqualsIN( parts.m_fname.c_str(), prefix, prefixLen ) )
		{
			fs::CPathParts rcParts = parts;
			rcParts.m_fname = parts.m_fname.substr( 0, parts.m_fname.length() - prefixLen );
			rcParts.m_ext = _T(".rc");

			return fs::IsValidFile( rcParts.MakePath().GetPtr() );
		}
	}

	return false;
}

fs::CPath CFileAssoc::FindAssociation( const TCHAR* pExt ) const
{
	std::vector< fs::CPath > alternateDirs;

	for ( unsigned int i = 0; i != COUNT_OF( s_relDirs ); ++i )
		alternateDirs.push_back( fs::CPath( m_parts.m_dir ) / fs::CPath( s_relDirs[ i ] ) );

	QueryComplementaryParentDirs( alternateDirs, m_parts.m_dir );		// also add complementary parent directories, such as SOURCE/INCLUDE

	for ( std::vector< fs::CPath >::const_iterator itDir = alternateDirs.begin(); itDir != alternateDirs.end(); ++itDir )
	{
		fs::CPath assocFullPath = fs::CPathParts::MakeFullPath( m_parts.m_drive.c_str(), itDir->GetPtr(), m_parts.m_fname.c_str(), pExt );
		if ( assocFullPath.FileExist() )
			return assocFullPath;
	}

	return s_emptyPath;
}

void CFileAssoc::QueryComplementaryParentDirs( std::vector< fs::CPath >& rComplementaryDirs, const fs::CPath& dir )
{
	// dir and rComplementaryDirs should look like "\WINNT\system32\"

	std::vector< std::tstring > dirTokens;
	str::Split( dirTokens, dir.GetPtr(), _T("\\") );

	// reverse iteration
	for ( int i = (int)dirTokens.size(); --i >= 0; )
		if ( path::Equals( dirTokens[ i ], _T("include") ) || path::Equals( dirTokens[ i ], _T("inc") ) )
		{
			std::tstring orgToken = dirTokens[ i ];

			dirTokens[ i ] = _T("source");
			rComplementaryDirs.push_back( str::Join( dirTokens, _T("\\") ) );

			dirTokens[ i ] = _T("src");
			rComplementaryDirs.push_back( str::Join( dirTokens, _T("\\") ) );

			dirTokens[ i ] = orgToken;
		}
		else if ( path::Equals( dirTokens[ i ], _T("source") ) || path::Equals( dirTokens[ i ], _T("src") ) )
		{
			std::tstring orgToken = dirTokens[ i ];

			dirTokens[ i ] = _T("include");
			rComplementaryDirs.push_back( str::Join( dirTokens, _T("\\") ) );

			dirTokens[ i ] = _T("inc");
			rComplementaryDirs.push_back( str::Join( dirTokens, _T("\\") ) );

			dirTokens[ i ] = orgToken;
		}
}

int CFileAssoc::GetNextIndex( int& index, bool forward /*= true*/ ) const
{
	ASSERT( index >= 0 );

	index += ( forward ? 1 : -1 );

	if ( index >= (int)COUNT_OF( s_circularExt ) )
		index = 0;
	else if ( index < 0 )
		index = (int)COUNT_OF( s_circularExt ) - 1;

	return index;
}

void CFileAssoc::GetComplIndex( std::vector< CircularIndex >& rIndexes ) const
{
	rIndexes.clear();
	switch ( m_circularIndex )
	{
		case CI_H:
		case CI_HPP:
			rIndexes.push_back( CI_CPP );
			rIndexes.push_back( CI_C );
			rIndexes.push_back( CI_LCC );
			rIndexes.push_back( CI_OPP );
			rIndexes.push_back( CI_PPC );
			break;
		case CI_HXX:
		case CI_INL:
		case CI_CXX:
		case CI_TMPL:
		case CI_T:
		case CI_C:
		case CI_CPP:
		case CI_LCC:
		case CI_OPP:
		case CI_PPC:
			rIndexes.push_back( CI_H );
			rIndexes.push_back( CI_HPP );
			break;
		case CI_SQL:
			rIndexes.push_back( CI_TAB );
			break;
		case CI_TAB:
			rIndexes.push_back( CI_SQL );
			break;
		case CI_PK:
		case CI_PKG:
		case CI_PKS:
		case CI_PAC:
			rIndexes.push_back( CI_PKB );
			break;
		case CI_PKB:
			rIndexes.push_back( CI_PK );
			rIndexes.push_back( CI_PKG );
			rIndexes.push_back( CI_PKS );
			rIndexes.push_back( CI_PAC );
			break;
		case CI_OT:
			rIndexes.push_back( CI_OTB );
			break;
		case CI_OTB:
			rIndexes.push_back( CI_OT );
			break;
	}
}

fs::CPath CFileAssoc::GetNextAssoc( bool forward /*= true*/ )
{
	int extIndex = m_circularIndex;

	if ( IsValidKnownAssoc() )
	{
		fs::CPath associatedFullPath;

		if ( HasSpecialAssociations() )
		{
			associatedFullPath = GetSpecialComplementaryAssoc();
			if ( !associatedFullPath.IsEmpty() )
				return associatedFullPath;
		}

		while ( GetNextIndex( extIndex, forward ) != m_circularIndex )
		{
			associatedFullPath = FindAssociation( s_circularExt[ extIndex ] );
			if ( !associatedFullPath.IsEmpty() )
				return associatedFullPath;
		}
	}

	return s_emptyPath;
}

fs::CPath CFileAssoc::GetComplementaryAssoc( void )
{
	if ( IsValidKnownAssoc() )
	{
		fs::CPath associatedFullPath;

		if ( HasSpecialAssociations() )
		{
			associatedFullPath = GetSpecialComplementaryAssoc();
			if ( !associatedFullPath.IsEmpty() )
				return associatedFullPath;
		}

		std::vector< CircularIndex > indexes;
		GetComplIndex( indexes );
		for ( std::vector< CircularIndex >::const_iterator itIndex = indexes.begin(); itIndex != indexes.end(); ++itIndex )
		{
			associatedFullPath = FindAssociation( s_circularExt[ *itIndex ] );
			if ( !associatedFullPath.IsEmpty() )
				return associatedFullPath;
		}
	}

	return s_emptyPath;
}

fs::CPath CFileAssoc::LookupSpecialFullPath( const fs::CPath& wildPattern )
{
	_tfinddata_t findInfo;
	long hFile = _tfindfirst( wildPattern.GetPtr(), &findInfo );
	std::tstring specFullPath;

	if ( hFile != -1L )
	{
		specFullPath = findInfo.name;
		_findclose( hFile );
	}
	return specFullPath;
}

fs::CPath CFileAssoc::GetSpecialComplementaryAssoc( void ) const
{
	fs::CPath assocFullPath;
	fs::CPath dirPath = m_parts.GetDirPath();

	ASSERT( HasSpecialAssociations() );

	if ( IsResourceHeaderFile( m_parts ) )
	{
		assocFullPath = LookupSpecialFullPath( dirPath / fs::CPath( _T("*.rc") ) );
		if ( !assocFullPath.IsEmpty() )
			assocFullPath = dirPath / assocFullPath;
	}
	else if ( ft::RC == m_fileType || ft::RES == m_fileType )
	{
		assocFullPath = LookupSpecialFullPath( dirPath / fs::CPath( _T("resource.*h") ) );
		if ( assocFullPath.IsEmpty() )
			assocFullPath = LookupSpecialFullPath( dirPath / fs::CPath( m_parts.m_fname + _T("Res.*h") ) );

		if ( !assocFullPath.IsEmpty() )
			assocFullPath = dirPath / assocFullPath;
	}

	if ( !assocFullPath.IsEmpty() && assocFullPath.FileExist() )
		return assocFullPath;

	return s_emptyPath;
}

void CFileAssoc::FindVariationsOf( std::vector< fs::CPath >& rVariations, int& rThisIdx, const fs::CPath& pattern )
{
	CFileFind findVariations;
	for( BOOL found = findVariations.FindFile( pattern.GetPtr() ); found; )
	{
		found = findVariations.FindNextFile();

		std::tstring foundFileName = (LPCTSTR)findVariations.GetFileName();

		if ( path::Equals( foundFileName, m_parts.GetNameExt() ) )
		{
			ASSERT( rThisIdx == -1 );
			rThisIdx = (int)rVariations.size();
		}
		rVariations.push_back( foundFileName );
	}
}

fs::CPath CFileAssoc::GetNextFileNameVariation( bool forward /*= true*/ )
{
	std::vector< fs::CPath > variations;
	int thisVariationIdx = -1;
	std::tstring nameBase;
	size_t lastUnderscorePos = m_parts.m_fname.rfind( _T('_') );

	if ( std::tstring::npos == lastUnderscorePos )
	{
		size_t firstBlankPos = m_parts.m_fname.find( _T(' ') );

		if ( std::tstring::npos == firstBlankPos )
			nameBase = m_parts.m_fname;
		else
			nameBase = m_parts.m_fname.substr( 0, firstBlankPos );
	}
	else
		nameBase = m_parts.m_fname.substr( 0, lastUnderscorePos );

	fs::CPathParts altParts = m_parts;

	altParts.m_fname = nameBase;
	FindVariationsOf( variations, thisVariationIdx, altParts.MakePath() );
	altParts.m_fname = nameBase + _T("_*");
	FindVariationsOf( variations, thisVariationIdx, altParts.MakePath() );
	altParts.m_fname = nameBase + _T(" *");
	FindVariationsOf( variations, thisVariationIdx, altParts.MakePath() );

	const int size = (int)variations.size();
	int nextVariationIdx;

	nextVariationIdx = ( forward ? thisVariationIdx + 1 : thisVariationIdx - 1 );
	if ( nextVariationIdx < 0 )
		nextVariationIdx = size - 1;
	else if ( nextVariationIdx >= size )
		nextVariationIdx = 0;

	return m_parts.GetDirPath() / variations[ nextVariationIdx ];
}

fs::CPath CFileAssoc::GetNextFileNameVariationEx( bool forward /*= true*/ )
{
	int extIndex = m_circularIndex, origCIndex = m_circularIndex;
	fs::CPath variation = GetNextFileNameVariation( forward );

	while ( variation.IsEmpty() && GetNextIndex( extIndex, forward ) != origCIndex )
	{
		m_circularIndex = (CircularIndex)extIndex;
		variation = GetNextFileNameVariation( forward );
	}

	return variation;
}

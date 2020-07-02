
#include "stdafx.h"
#include "PathUniqueMaker.h"
#include "StringUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CPathUniqueMaker::CPathUniqueMaker( const TCHAR fmtNumSuffix[] /*= path::StdFormatNumSuffix()*/ )
	: m_pFmtNumSuffix( fmtNumSuffix )
{
	REQUIRE( !str::IsEmpty( m_pFmtNumSuffix ) );
	SetupSuffixPattern();
}

void CPathUniqueMaker::SetupSuffixPattern( void )
{
	static const TCHAR s_numericFieldChars[] = _T("diouxX");

	const TCHAR* pNumStart = _tcschr( m_pFmtNumSuffix, _T('%') );
	const TCHAR* pNumEnd = pNumStart != NULL ? pNumEnd = str::FindTokenEnd( pNumStart, s_numericFieldChars ) : NULL;

	if ( !str::IsEmpty( pNumEnd ) )
	{
		++pNumEnd;			// skip the numeric format field
		m_fnSuffixPattern = std::tstring( m_pFmtNumSuffix, pNumStart ) + _T('*') + pNumEnd;
	}
	else
		ASSERT( false );	// fishy numeric suffix format '%s' - it does not produce unique filenames
}

fs::CPath CPathUniqueMaker::MakeUniqueFilename( const fs::CPath& filePath ) const
{
	ASSERT( !filePath.IsEmpty() );

	if ( IsUniquePath( filePath ) )
		return filePath;

	fs::CPath newFilePath;
	std::tstring fnameBase = filePath.GetFname();

	for ( UINT seqCount = std::max( 2u, QueryExistingSequenceCount( filePath ) + 1 ); ; ++seqCount )
	{
		std::tstring fname = fnameBase + str::Format( m_pFmtNumSuffix, seqCount );

		newFilePath = filePath;
		newFilePath.ReplaceFname( fname.c_str() );

		if ( IsUniquePath( newFilePath ) )
			break;
	}
	return newFilePath;
}

UINT CPathUniqueMaker::QueryExistingSequenceCount( const fs::CPath& filePath ) const
{
	UINT seqCount = 0;
	fs::CPath parentPath = filePath.GetParentPath();
	std::tstring fnameBase = filePath.GetFname();
	std::tstring filenamePattern = fnameBase + m_fnSuffixPattern + filePath.GetExt();

	for ( stdext::hash_set< fs::CPath >::const_iterator itUniquePath = m_uniquePathsIndex.begin(); itUniquePath != m_uniquePathsIndex.end(); ++itUniquePath )
		if ( filePath == *itUniquePath )							// direct collision?
			seqCount = std::max( 1u, seqCount );
		else if ( parentPath == itUniquePath->GetParentPath() )		// same parent path?
			if ( path::MatchWildcard( itUniquePath->GetNameExt(), filenamePattern.c_str() ) )	// check filename collision
			{
				std::tstring uniqueFnameBase = itUniquePath->GetFname();
				UINT number;

				if ( ParseNumericSuffix( number, uniqueFnameBase.c_str() + fnameBase.length() ) )	// skip past original fname to search for digits
					seqCount = std::max( number, seqCount );
			}

	return seqCount;
}

bool CPathUniqueMaker::ParseNumericSuffix( UINT& rNumber, const TCHAR* pText )
{
	while ( *pText != _T('\0') && !str::CharTraits::IsDigit( *pText ) )
		++pText;

	return *pText != _T('\0') && num::ParseNumber( rNumber, pText );
}

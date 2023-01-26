
#include "stdafx.h"
#include "Table.h"
#include "utl/Algorithms.h"
#include "utl/ContainerOwnership.h"
#include "utl/TextFileIo.h"
#include <hash_set>
#include <deque>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#include "utl/TextFileIo.hxx"


namespace hlp
{
	void PushFront( std::deque< TCHAR >& rOut, const std::tstring& text )
	{
		for ( std::tstring::const_reverse_iterator itChar = text.rbegin(); itChar != text.rend(); ++itChar )
			rOut.push_front( *itChar );
	}
}


// CTextCell implementation

TCHAR CTextCell::s_columnSep[] = _T("\t");

CTextCell::CTextCell( const CTextCell* pParent, const std::tstring& name )
	: m_name( name )
	, m_pParent( pParent )
{
}

CTextCell::~CTextCell()
{
	utl::ClearOwningContainer( m_children );
}

CTextCell* CTextCell::AddChild( const std::tstring& name )
{
	CTextCell* pChild = FindCell( name );

	if ( NULL == pChild )
		m_children.push_back( pChild = new CTextCell( this, name ) );

	ENSURE( pChild != NULL );
	return pChild;
}

CTextCell* CTextCell::FindCell( const std::tstring& name ) const
{
	for ( std::vector< CTextCell* >::const_iterator itChild = m_children.begin(); itChild != m_children.end(); ++itChild )
		if ( name == (*itChild)->GetName() )
			return *itChild;

	return NULL;
}

std::tstring CTextCell::MakePath( const CTextCell* pRoot /*= NULL*/ ) const
{
	std::deque< TCHAR > path( m_name.begin(), m_name.end() );
	static std::tstring s_strColumnSep = s_columnSep;

	for ( const CTextCell* pParent = m_pParent; pParent != pRoot; pParent = pParent->GetParent() )
	{
		hlp::PushFront( path, s_strColumnSep );
		hlp::PushFront( path, pParent->GetName() );
	}

	return std::tstring( path.begin(), path.end() );
}

CTextCell* CTextCell::DeepFindCell( const TCHAR* pCellPath ) const
{
	std::vector< std::tstring > cellNames;
	str::Split( cellNames, pCellPath, s_columnSep );

	CTextCell* pCell = const_cast<CTextCell*>( this );

	for ( std::vector< std::tstring >::const_iterator itCellName = cellNames.begin(); itCellName != cellNames.end() && pCell != NULL; ++itCellName )
		pCell = pCell->FindCell( *itCellName );

	return pCell;
}

void CTextCell::QuerySubFolders( std::vector< CTextCell* >& rSubFolders ) const
{
	utl::QueryThat( rSubFolders, m_children, std::mem_fun( &CTextCell::IsFolder ) );
}

void CTextCell::QueryLeafs( std::vector< CTextCell* >& rLeafs ) const
{
	utl::QueryThat( rLeafs, m_children, std::mem_fun( &CTextCell::IsLeaf ) );
}


// CTable implementation

CTable::CTable( void )
	: m_root( NULL, _T("<root>") )
{
}

CTable::~CTable()
{
}

fs::Encoding CTable::ParseTextFile( const fs::CPath& textFilePath, bool sortRows ) throws_( CRuntimeException )
{
	std::vector< std::tstring > rows;
	fs::Encoding encoding = io::ReadLinesFromFile( rows, textFilePath );

	ParseRows( rows, sortRows );
	return encoding;
}

void CTable::ParseRows( std::vector< std::tstring >& rRows, bool sortRows )
{
	if ( sortRows )
		std::sort( rRows.begin(), rRows.end(), pred::TLess_StringyIntuitive() );

	stdext::hash_set< std::tstring > uniqueRows;

	for ( std::vector< std::tstring >::const_iterator itRow = rRows.begin(); itRow != rRows.end(); ++itRow )
		if ( uniqueRows.insert( *itRow ).second )		// is row unique? - filter duplicate rows
			ParseColumns( *itRow );
}

void CTable::ParseColumns( const std::tstring& row )
{
	if ( row.empty() )
		return;				// ignore empty rows

	std::vector< std::tstring > columns;
	str::Split( columns, row.c_str(), CTextCell::s_columnSep );

	CTextCell* pPathCell = &m_root;

	for ( std::vector< std::tstring >::const_iterator itColumn = columns.begin(); itColumn != columns.end(); ++itColumn )
		if ( !itColumn->empty() )
			pPathCell = pPathCell->AddChild( *itColumn );
		else
			break;			// break at empty column, and stop parsing
}

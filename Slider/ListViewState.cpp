
#include "stdafx.h"
#include "ListViewState.h"
#include "utl/ContainerUtilities.h"
#include "utl/Serialization.h"
#include "utl/SerializeStdTypes.h"
#include "utl/UI/ReportListControl.h"
#include "utl/UI/Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


class CListStringItems
{
public:
	CListStringItems( const CListBox* pListBox = NULL )
	{
		if ( pListBox != NULL )
			StoreItems( pListBox );
	}

	void StoreItems( const CListBox* pListBox )
	{
		ui::ReadListBoxItems( m_items, *pListBox );
	}

	int Find( const std::tstring& item ) const { return static_cast< int >( utl::FindPos( m_items, item ) ); }

	void QueryIndexes( std::vector< int >& rIndexes, const std::vector< std::tstring >& items ) const
	{
		rIndexes.clear();
		rIndexes.reserve( items.size() );
		for ( std::vector< std::tstring >::const_iterator itItem = items.begin(); itItem != items.end(); ++itItem )
		{
			int pos = Find( *itItem );
			if ( pos != -1 )
				rIndexes.push_back( pos );
		}
	}

	std::tstring GetAt( size_t pos ) const
	{
		ASSERT( pos < m_items.size() );
		return m_items[ pos ];
	}

	void QueryItems( std::vector< std::tstring >& rItems, std::vector< int >& indexes ) const
	{
		rItems.clear();
		rItems.reserve( indexes.size() );
		for ( std::vector< int >::const_iterator itIndex = indexes.begin(); itIndex != indexes.end(); ++itIndex )
		{
			if ( (size_t)*itIndex < m_items.size() )
				rItems.push_back( m_items[ *itIndex ] );
		}
	}
private:
	std::vector< std::tstring > m_items;
};


// CListViewState implementation

CListViewState::CListViewState( StoreMode storeBy )
{
	switch ( storeBy )
	{
		case StoreByIndex: m_pIndexImpl.reset( new CImpl< int > ); break;
		case StoreByString: m_pStringImpl.reset( new CImpl< std::tstring > ); break;
	}
}

CListViewState::CListViewState( std::vector< std::tstring >& rSelStrings )
	: m_pStringImpl( new CImpl< std::tstring > )
{
	m_pStringImpl->m_selItems.swap( rSelStrings );
}

CListViewState::CListViewState( const std::vector< int >& selIndexes )
	: m_pIndexImpl( new CImpl< int > )
{
	m_pIndexImpl->m_selItems = selIndexes;
}

CListViewState::~CListViewState()
{
	Reset();
}

CListViewState& CListViewState::operator=( const CListViewState& src )
{
	if ( &src != this )
	{
		Reset();
		if ( src.UseIndexes() )
			m_pIndexImpl.reset( new CImpl< int >( *src.m_pIndexImpl ) );
		else if ( src.UseStrings() )
			m_pStringImpl.reset( new CImpl< std::tstring >( *src.m_pStringImpl ) );
	}
	return *this;
}

void CListViewState::Stream( CArchive& archive )
{
	if ( archive.IsStoring() )
		archive << (int)( UseIndexes() ? StoreByIndex : StoreByString );
	else
	{
		StoreMode storeBy;
		archive >> (int&)storeBy;

		Reset();
		switch ( storeBy )
		{
			case StoreByIndex: m_pIndexImpl.reset( new CImpl< int > ); break;
			case StoreByString: m_pStringImpl.reset( new CImpl< std::tstring > ); break;
		}
	}

	if ( UseIndexes() )
		m_pIndexImpl->Stream( archive );
	else
		m_pStringImpl->Stream( archive );
}

int CListViewState::GetSelCount( void ) const
{
	if ( UseIndexes() )
		return (int)m_pIndexImpl->m_selItems.size();
	else if ( UseStrings() )
		return (int)m_pStringImpl->m_selItems.size();
	return 0;
}

namespace hlp
{
	template< typename Type >
	bool SetCaretOnSel( CListViewState::CImpl< Type >& rDataSet, bool firstSel )
	{
		if ( rDataSet.m_selItems.empty() )
			return false;

		rDataSet.m_caret = firstSel ? rDataSet.m_selItems.front() : utl::Back( rDataSet.m_selItems );
		return true;
	}
}

bool CListViewState::SetCaretOnSel( bool firstSel /*= true*/ )
{
	if ( UseIndexes() )
		return hlp::SetCaretOnSel( *m_pIndexImpl, firstSel );
	else if ( UseStrings() )
		return hlp::SetCaretOnSel( *m_pStringImpl, firstSel );
	return 0;
}


template< typename Type >
std::tostream& operator<<( std::tostream& oss, const CListViewState::CImpl< Type >& impl )
{
	return oss
		<< _T(" SEL=") << str::FormatSet( impl.m_selItems )
		<< _T(" CARET=") << impl.m_caret
		<< _T(" TOP=") << impl.m_top;
}

std::tstring CListViewState::dbgFormat( void )
{
	std::tostringstream oss;
#ifdef _DEBUG
	oss << UseIndexes() ? _T("INDEXES") : _T("STRINGS");
	if ( m_pIndexImpl.get() != NULL )
		oss << *m_pIndexImpl;
	else if ( m_pStringImpl.get() != NULL )
		oss << *m_pStringImpl;
#else // !_DEBUG

#endif // _DEBUG
	return oss.str();
}

void CListViewState::FromListBox( const CListBox* pListBox )
{
	ASSERT_PTR( pListBox->GetSafeHwnd() );
	Clear();

	if ( !UseIndexes() )
	{
		CListViewState indexState( StoreByIndex );
		indexState.FromListBox( pListBox );

		CListStringItems items( pListBox );
		items.QueryItems( m_pStringImpl->m_selItems, indexState.m_pIndexImpl->m_selItems );
		m_pStringImpl->m_caret = items.GetAt( indexState.m_pIndexImpl->m_caret );
		m_pStringImpl->m_top = items.GetAt( indexState.m_pIndexImpl->m_top );
		return;
	}

	m_pIndexImpl->m_top = pListBox->GetTopIndex();

	if ( HasFlag( pListBox->GetStyle(), LBS_EXTENDEDSEL | LBS_MULTIPLESEL ) )
	{
		m_pIndexImpl->m_caret = pListBox->GetCaretIndex();

		if ( int selCount = pListBox->GetSelCount() )
		{
			m_pIndexImpl->m_selItems.resize( selCount );
			pListBox->GetSelItems( selCount, &m_pIndexImpl->m_selItems.front() );
		}
	}
	else
	{
		int selIndex = pListBox->GetCurSel();
		if ( selIndex != LB_ERR )
			m_pIndexImpl->m_selItems.push_back( selIndex );
	}
}

void CListViewState::ToListBox( CListBox* pListBox ) const
{
	ASSERT_PTR( pListBox->GetSafeHwnd() );

	if ( !UseIndexes() )
	{
		CListStringItems items( pListBox );

		CListViewState indexState( StoreByIndex );
		items.QueryIndexes( indexState.m_pIndexImpl->m_selItems, m_pStringImpl->m_selItems );
		indexState.m_pIndexImpl->m_caret = items.Find( m_pStringImpl->m_caret );
		indexState.m_pIndexImpl->m_top = items.Find( m_pStringImpl->m_top );
		indexState.ToListBox( pListBox );
		return;
	}

	if ( m_pIndexImpl->m_top != LB_ERR )
		pListBox->SetTopIndex( m_pIndexImpl->m_top );

	if ( HasFlag( pListBox->GetStyle(), LBS_EXTENDEDSEL | LBS_MULTIPLESEL ) )
	{
		pListBox->SelItemRange( false, 0, pListBox->GetCount() - 1 );		// clear selection

		for ( size_t i = m_pIndexImpl->m_selItems.size(); i-- != 0; )
			pListBox->SetSel( m_pIndexImpl->m_selItems[ i ] );

		if ( m_pIndexImpl->m_caret != LB_ERR )
			pListBox->SetCaretIndex( m_pIndexImpl->m_caret, LB_ERR == m_pIndexImpl->m_top );
	}
	else
		pListBox->SetCurSel( 1 == m_pIndexImpl->m_selItems.size() ? m_pIndexImpl->m_selItems.front() : LB_ERR );		// single selection list
}

void CListViewState::FromListCtrl( const CReportListControl* pListCtrl )
{
	ASSERT_PTR( pListCtrl->GetSafeHwnd() );
	ASSERT( UseIndexes() );
	Clear();

	for ( POSITION pos = pListCtrl->GetFirstSelectedItemPosition(); pos != NULL; )
		m_pIndexImpl->m_selItems.push_back( pListCtrl->GetNextSelectedItem( pos ) );

	m_pIndexImpl->m_caret = pListCtrl->GetNextItem( -1, LVNI_ALL | LVNI_FOCUSED );
	m_pIndexImpl->m_top = pListCtrl->GetTopIndex();
}

void CListViewState::ToListCtrl( CReportListControl* pListCtrl ) const
{
	ASSERT_PTR( pListCtrl->GetSafeHwnd() );
	ASSERT( UseIndexes() );

	CScopedInternalChange change( pListCtrl );

	pListCtrl->ClearSelection();

	// select new items
	for ( size_t i = 0; i != m_pIndexImpl->m_selItems.size(); ++i )
		pListCtrl->SetItemState( m_pIndexImpl->m_selItems[ i ], LVNI_SELECTED, LVNI_SELECTED );

	if ( m_pIndexImpl->m_caret != -1 )
	{
		pListCtrl->SetItemState( m_pIndexImpl->m_caret, LVNI_FOCUSED, LVNI_FOCUSED );
		pListCtrl->SetSelectionMark( m_pIndexImpl->m_caret );
		pListCtrl->EnsureVisible( m_pIndexImpl->m_caret, false );
	}
}

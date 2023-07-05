#ifndef ListViewState_h
#define ListViewState_h
#pragma once

#include <vector>


class CReportListControl;

enum StoreMode { StoreByIndex, StoreByString };


// stores the visual state (selection, caret and top items) of list view (list-box or list-control)

struct CListViewState
{
public:
	CListViewState( void ) {}
	CListViewState( StoreMode storeBy );
	CListViewState( const std::vector<int>& selIndexes );
	CListViewState( std::vector<std::tstring>& rSelStrings );
	CListViewState( const CListViewState& src );
	~CListViewState();

	CListViewState& operator=( const CListViewState& src );

	void Clear( void ) { ASSERT( IsConsistent() ); UseIndexes() ? m_pIndexImpl->m_selItems.clear() : m_pStringImpl->m_selItems.clear(); }
	void Stream( CArchive& archive );

	bool IsConsistent( void ) const { return ( NULL == m_pStringImpl.get() ) != ( NULL == m_pIndexImpl.get() ); }
	bool UseIndexes( void ) const { return m_pIndexImpl.get() != NULL; }
	bool UseStrings( void ) const { return m_pStringImpl.get() != NULL; }

	bool IsEmpty( void ) const { return 0 == GetSelCount(); }
	int GetSelCount( void ) const;

	int GetCaretIndex( void ) const { return UseIndexes() ? m_pIndexImpl->m_caret : -1; }
	void SetCaretIndex( int caretIndex ) { ASSERT( UseIndexes() ); m_pIndexImpl->m_caret = caretIndex; }
	bool SetCaretOnSel( bool firstSel = true );

	std::tstring dbgFormat( void );

	// control interface
	void FromListBox( const CListBox* pListBox );
	void ToListBox( CListBox* pListBox ) const;

	void FromListCtrl( const CReportListControl* pListCtrl );
	void ToListCtrl( CReportListControl* pListCtrl ) const;
private:
	void Reset( void ) { m_pIndexImpl.reset(); m_pStringImpl.reset(); }

	template< typename Type > static void SetDefaultValue( Type& rValue ) { rValue = Type(); }
	template<> static void SetDefaultValue<int>( int& rValue ) { rValue = -1; }
public:
	template< typename Type >
	struct CImpl
	{
		CImpl( void ) { Clear(); }

		void Clear( void )
		{
			m_selItems.clear();
			SetDefaultValue( m_caret );
			SetDefaultValue( m_top );
		}

		void Stream( CArchive& archive )
		{
			serial::SerializeValues( archive, m_selItems );
			archive & m_caret & m_top;
		}
	public:
		persist std::vector<Type> m_selItems;
		persist Type m_caret;
		persist Type m_top;
	};
public:
	persist std::auto_ptr< CImpl<int> > m_pIndexImpl;
	persist std::auto_ptr< CImpl<std::tstring> > m_pStringImpl;
};


#endif // ListViewState_h

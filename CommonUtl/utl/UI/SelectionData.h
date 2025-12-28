#ifndef SelectionData_h
#define SelectionData_h
#pragma once


class CReportListControl;
class CTextListEditor;


namespace ui
{
	template< typename ObjectT >
	struct CSelectionData
	{
		CSelectionData( bool useTopItem = true ) : m_useTopItem( useTopItem ), m_pCaretItem( nullptr ), m_pTopVisibleItem( nullptr ) {}

		bool IsEmpty( void ) const { return nullptr == m_pCaretItem && m_selItems.empty(); }
		void Clear( void ) { m_pTopVisibleItem = nullptr; m_pCaretItem = nullptr; m_selItems.clear(); }

		bool operator==( const CSelectionData& right ) const
		{
			return m_pCaretItem == right.m_pCaretItem
				&& m_selItems == right.m_selItems
				&& ( !m_useTopItem || m_pTopVisibleItem == right.m_pTopVisibleItem );
		}
		bool operator!=( const CSelectionData& right ) const { return !operator==( right ); }

		const std::vector<ObjectT*>& GetSelItems( void ) const { return m_selItems; }
		bool SetSelItems( const std::vector<ObjectT*>& selItems ) { return utl::ModifyValue( m_selItems, selItems ); }

		bool IsSelectionMultiple( void ) const { return m_selItems.size() > 1; }
		bool IsCaretSelected( void ) const { return m_pCaretItem != nullptr && std::find( m_selItems.begin(), m_selItems.end(), m_pCaretItem ) != m_selItems.end(); }

		ObjectT* GetCaretItem( void ) const { return m_pCaretItem; }
		bool SetCaretItem( const ObjectT* pCaretItem ) { return utl::ModifyPtr( m_pCaretItem, pCaretItem ); }

		ObjectT* GetTopVisibleItem( void ) const { return m_pTopVisibleItem; }
		bool SetTopVisibleItem( const ObjectT* pTopVisibleItem ) { return utl::ModifyPtr( m_pTopVisibleItem, pTopVisibleItem ); }

		// edit-like selection (caret is the selection):
		ObjectT* GetCurSelItem( void ) const;
		bool SetCurSelItem( ObjectT* pCurSelItem, bool exclusive = true );
	public:
		bool ReadList( const CReportListControl* pListCtrl );
		bool UpdateList( CReportListControl* pListCtrl ) const;

		bool ReadEdit( const CTextListEditor* pEdit );
		bool UpdateEdit( CTextListEditor* pEdit ) const;
	private:
		bool SortListSelItems( const CReportListControl* pListCtrl );
	protected:
		bool m_useTopItem;
		std::vector<ObjectT*> m_selItems;
		ObjectT* m_pCaretItem;
		ObjectT* m_pTopVisibleItem;
	};
}


#endif // SelectionData_h

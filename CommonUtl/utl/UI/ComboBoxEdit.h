#ifndef ComboBoxEdit_h
#define ComboBoxEdit_h
#pragma once


class CComboDropList;


// a regular combo-box, that prevents partial string matches in the drop down list - useful with CBS_DROPDOWN style (with edit-box)

class CComboBoxEdit : public CComboBox
{
public:
	CComboBoxEdit( void );
	virtual ~CComboBoxEdit();

	void SetUseExactMatch( bool useExactMatch = true ) { ASSERT_NULL( m_pDropList.get() ); m_useExactMatch = useExactMatch; }
protected:
	virtual void SubclassDetails( const COMBOBOXINFO& cbInfo );
private:
	bool m_useExactMatch;			// if true: prevents selecting items in the list based on partial matches (uses LB_FINDSTRINGEXACT instead of LB_FINDSTRING)
	std::auto_ptr<CComboDropList> m_pDropList;

	// generated stuff
public:
	virtual void PreSubclassWindow( void );
	virtual BOOL PreTranslateMessage( MSG* pMsg );
};


class CComboDropList : public CWnd	// can't inherit from CListBox
{
public:
	CComboDropList( CComboBox* pParentCombo, bool useExactMatch );

	static CComboDropList* MakeSubclass( CComboBox* pParentCombo, bool useExactMatch, bool autoDelete = true );
private:
	CComboBox* m_pParentCombo;
	bool m_autoDelete;				// useful when not mamaged in a data-member
	bool m_useExactMatch;			// if true: prevents selecting items in the list based on partial matches (uses LB_FINDSTRINGEXACT instead of LB_FINDSTRING)
	bool m_trackingMenu;			// prevent closing up the drop-down list while tracking the context menu

	// generated stuff
protected:
	virtual void PostNcDestroy( void ) overrides(CWnd);
private:
	afx_msg void OnContextMenu( CWnd* pWnd, CPoint point );
	afx_msg void OnCaptureChanged( CWnd* pWnd );
	afx_msg LRESULT OnLBFindString( WPARAM wParam, LPARAM lParam );

	DECLARE_MESSAGE_MAP()
};


#endif // ComboBoxEdit_h

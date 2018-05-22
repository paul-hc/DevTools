#ifndef DateTimeControl_h
#define DateTimeControl_h
#pragma once

#include <afxdtctl.h>
#include "InternalChange.h"
#include "Range.h"


class CAccelTable;


class CDateTimeControl : public CDateTimeCtrl
					   , public CInternalChange
{
public:
	CDateTimeControl( const TCHAR* pValidFormat = s_dateTimeFormat, const TCHAR* pNullFormat = s_nullFormat );
	virtual ~CDateTimeControl();

	void SetValidFormat( const TCHAR* pValidFormat );
	void SetNullFormat( const TCHAR* pNullFormat );

	Range< CTime > GetDateRange( void ) const;
	bool SetDateRange( const Range< CTime >& dateTimeRange );

	CTime GetDateTime( void ) const;
	bool SetDateTime( CTime dateTime );

	bool IsNullDateTime( void ) const;
	bool SetNullDateTime( void ) { return SetDateTime( CTime() ); }

	static bool IsNullDateTime( const CTime& dateTime ) { return 0 == dateTime.GetTime(); }
private:
	bool FlipFormat( bool isValid );

	static CAccelTable& GetAccelTable( void );
private:
	const TCHAR* m_pValidFormat;
	const TCHAR* m_pNullFormat;
	const TCHAR* m_pLastFormat;					// keeps track of the null state
public:
	static const TCHAR s_dateTimeFormat[];		// "d-MM-yyy  hh:m:s"
	static const TCHAR s_nullFormat[];			// " "

public:
	// generated stuff
	public:
	virtual void PreSubclassWindow( void );
	virtual BOOL PreTranslateMessage( MSG* pMsg );
protected:
	afx_msg void OnContextMenu( CWnd* pWnd, CPoint screenPos );
	afx_msg void OnInitMenuPopup( CMenu* pPopupMenu, UINT index, BOOL isSysMenu );
	virtual BOOL OnDtnDateTimeChange_Reflect( NMHDR* pNmHdr, LRESULT* pResult );
	virtual BOOL OnDtnCloseup_Reflect( NMHDR* pNmHdr, LRESULT* pResult );
	afx_msg void OnCut( void );
	afx_msg void OnCopy( void );
	afx_msg void OnPaste( void );
	afx_msg void OnUpdatePaste( CCmdUI* pCmdUI );
	afx_msg void OnSetNow( void );
	afx_msg void OnClear( void );
	afx_msg void OnUpdateValid( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


#endif // DateTimeControl_h

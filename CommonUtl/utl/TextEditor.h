#ifndef TextEditor_h
#define TextEditor_h
#pragma once

#include "Range.h"
#include "TextEdit.h"


class CTextEditor : public CTextEdit
{
public:
	CTextEditor( bool useFixedFont = false );
	virtual ~CTextEditor();

	enum SelType { SelEmpty, SelSubText, SelAllText };

	SelType GetSelType( void ) const;

	bool HasSelEmpty( void ) const { return SelEmpty == GetSelType(); }
	bool HasSelSubText( void ) const { return SelSubText == GetSelType(); }
	bool HasSelAllText( void ) const { return SelAllText == GetSelType(); }
	bool HasSel( void ) const;

	template< typename IntType >
	Range< IntType > GetSelRange( void ) const
	{
		int startPos, endPos;
		GetSel( startPos, endPos );
		ASSERT( startPos <= endPos && startPos >= 0 );
		return Range< IntType >( static_cast< IntType >( startPos ), static_cast< IntType >( endPos ) );
	}

	template< typename IntType >
	void SetSelRange( const Range< IntType >& sel ) { SetSel( static_cast< int >( sel.m_start ), static_cast< int >( sel.m_end ) ); }
private:
	CAccelTable m_editorAccel;

	// generated stuff
	public:
	virtual BOOL PreTranslateMessage( MSG* pMsg );
protected:
	afx_msg void OnContextMenu( CWnd* pWnd, CPoint point );
	afx_msg void OnLButtonDblClk( UINT flags, CPoint point );
	afx_msg void OnWordSelection( UINT cmdId );
	afx_msg void OnChangeCase( UINT cmdId );
	afx_msg void OnChangeNumber( UINT cmdId );

	DECLARE_MESSAGE_MAP()
};


#endif // TextEditor_h

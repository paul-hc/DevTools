#ifndef BaseItemTooltipsCtrl_h
#define BaseItemTooltipsCtrl_h
#pragma once

#include "ISubject.h"


// Abstract base for list-like controls that displays item tooltips.

template< typename BaseCtrl >
abstract class CBaseItemTooltipsCtrl
	: public BaseCtrl
{
	typedef BaseCtrl TBaseClass;
protected:
	CBaseItemTooltipsCtrl( void );
public:
	// item interface
	virtual utl::ISubject* GetItemSubjectAt( int index ) const = 0;
	virtual CRect GetItemRectAt( int index ) const = 0;
	virtual int GetItemFromPoint( const CPoint& clientPos ) const = 0;

	virtual std::tstring GetItemTooltipTextAt( int index ) const
	{
		return utl::GetSafeDisplayCode( GetItemSubjectAt( index ) );
	}

	template< typename ObjectT >
	ObjectT* GetItemObjectAt( int index ) const { return checked_static_cast<ObjectT*>( GetItemSubjectAt( index ) ); }
private:

	// generated stuff
public:
	virtual void PreSubclassWindow( void );
	virtual INT_PTR OnToolHitTest( CPoint point, TOOLINFO* pToolInfo ) const;
protected:
	afx_msg BOOL OnTtnNeedText( UINT cmdId, NMHDR* pNmHdr, LRESULT* pResult );

	DECLARE_MESSAGE_MAP()
};


// provides a default contructor for CCtrlView base

abstract class CBaseCtrlView
	: public CCtrlView
{
protected:
	CBaseCtrlView( void ) : CCtrlView( _T(""), 0 ) {}

	void ConstructView( const TCHAR* pCtrlClass, DWORD dwDefaultStyle )
	{
		m_strClass = pCtrlClass;
		m_dwDefaultStyle = dwDefaultStyle;
	}
};


#endif // BaseItemTooltipsCtrl_h

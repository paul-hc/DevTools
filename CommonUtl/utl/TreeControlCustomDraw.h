#ifndef TreeControlCustomDraw_h
#define TreeControlCustomDraw_h
#pragma once

#include "ListLikeCtrlBase.h"


class CTreeControl;


class CTreeControlCustomDraw
	: public CListLikeCustomDrawBase
{
public:
	CTreeControlCustomDraw( NMTVCUSTOMDRAW* pDraw, CTreeControl* pTree );

	// cell text effects
	bool ApplyItemTextEffect( void );
	COLORREF GetRealizedBkColor( void ) const;
private:
	// cell text effects
	ui::CTextEffect MakeItemEffect( void ) const;
	bool ApplyEffect( const ui::CTextEffect& textEffect );			// to m_pDraw

	bool SelectTextEffect( const ui::CTextEffect& textEffect );		// to m_pDC

	bool IsSelItemContrast( void ) const;							// item is blue backgound with white text?
private:
	NMTVCUSTOMDRAW* m_pDraw;
	CTreeControl* m_pTree;
public:
	const HTREEITEM m_hItem;
	const utl::ISubject* m_pObject;
};


#endif // TreeControlCustomDraw_h

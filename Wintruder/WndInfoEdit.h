#ifndef WndInfoEdit_h
#define WndInfoEdit_h
#pragma once

#include "utl/UI/ImageEdit.h"


class CWndInfoEdit : public CImageEdit
{
public:
	CWndInfoEdit( void );
	virtual ~CWndInfoEdit();

	HWND GetCurrentWnd( void ) const { return m_hCurrentWnd; }
	void SetCurrentWnd( HWND hCurrentWnd );
protected:
	// base overrides
	virtual void DrawImage( CDC* pDC, const CRect& imageRect );
private:
	HWND m_hCurrentWnd;
};


#endif // WndInfoEdit_h

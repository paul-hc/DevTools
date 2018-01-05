#ifndef DesktopDC_h
#define DesktopDC_h
#pragma once

#include "UtilitiesEx.h"


class CDesktopDC : public CDC
{
public:
	CDesktopDC( bool clipTopLevelWindows = true, bool useUpdateLocking = false );
	virtual ~CDesktopDC();
private:
	HDC InitUpdateLocking( bool clipTopLevelWindows );
	HDC InitNormal( bool clipTopLevelWindows );
	void Release( void );
	void MakeTopLevelRegion( CRgn& rRegion ) const;
private:
	bool m_updateLocked;
	HWND m_hDesktopWnd;
	CScopedDisableBeep m_disableBeep;		// beeps (async or sync) can interfere with the desktop DC (freezes drawing)
};


#endif // DesktopDC_h

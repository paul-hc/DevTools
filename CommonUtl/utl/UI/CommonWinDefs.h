#ifndef CommonWinDefs_h
#define CommonWinDefs_h
#pragma once


// MFC forward declarations
void AFXAPI AfxSetWindowText( HWND hWndCtrl, LPCTSTR lpszNew );									// <afxpriv.h>
void AFXAPI AfxCancelModes( HWND hWndRcvr );													// <src/mfc/afximpl.h>
BOOL AFXAPI AfxFullPath( _Pre_notnull_ _Post_z_ LPTSTR lpszPathOut, LPCTSTR lpszFileIn );		// <src/mfc/afximpl.h>


// headers to pre-compile
#include "StdColors.h"


// new theme parts & states (missing in VC9 tmschema.h)
namespace vt
{
	#include "UI/vsstyle.h"


	// missing part & state definitions

	enum GLOBALS_PARTS { GP_BORDER = 1, GP_LINEHORZ, GP_LINEVERT };

	enum BORDER_STATES { BSS_FLAT = 1, BSS_RAISED, BSS_SUNKEN };
	enum LINEHORZ_STATES { LHS_FLAT = 1, LHS_RAISED, LHS_SUNKEN };
	enum LINEVERT_STATES { LVS_FLAT = 1, LVS_RAISED, LVS_SUNKEN };

	enum SEARCHEDITBOX_PARTS { SEBP_SEARCHEDITBOXTEXT = 1 };
	enum SEARCHEDITBOXTEXT_STATES { SEBTS_FORMATTED = 1 };
}


#endif // CommonWinDefs_h

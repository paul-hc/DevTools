#ifndef FlagsEdit_h
#define FlagsEdit_h
#pragma once

#include "BaseFlagsCtrl.h"
#include "utl/InternalChange.h"


class CFlagsEdit : public CEdit
				 , public CBaseFlagsCtrl
				 , public CInternalChange
{
public:
	CFlagsEdit( void );
	virtual ~CFlagsEdit();
protected:
	// base overrides
	virtual void OutputFlags( void );
	virtual void InitControl( void );

	DWORD InputFlags( bool* pValid = NULL ) const;
	bool HandleTextChange( void );
public:
	public:
	virtual void PreSubclassWindow( void );
	virtual BOOL PreTranslateMessage( MSG* pMsg );
protected:
	afx_msg BOOL OnEnChange_Reflect( void );
	afx_msg LRESULT OnWmCopy( WPARAM wParam, LPARAM lParam );
	afx_msg void OnCopy( void );
	afx_msg void OnUpdateCopy( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


#endif // FlagsEdit_h

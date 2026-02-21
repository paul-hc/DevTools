#ifndef HotKeyCtrlEx_h
#define HotKeyCtrlEx_h
#pragma once

#include <afxcmn.h>
#include "InternalChange.h"
#include "MultiValueBase.h"


namespace ui
{
	struct CHotKey
	{
		CHotKey( WORD hotKey = 0 ) : m_virtualKeyCode( LOBYTE( hotKey ) ), m_modifiers( HIBYTE( hotKey ) ) {}
		CHotKey( WORD virtualKeyCode, WORD modifiers ) : m_virtualKeyCode( virtualKeyCode ), m_modifiers( modifiers ) {}

		WORD AsWord( void ) const { return MAKEWORD( m_virtualKeyCode, m_modifiers ); }				// LOBYTE: virtualKeyCode, HIBYTE: modifiers - compatible with IShellLink::GetHotkey
		WORD AsDWord( void ) const { return (DWORD)MAKELONG( m_virtualKeyCode, m_modifiers ); }		// LOWORD: virtualKeyCode, HIWORD: modifiers

		bool operator==( const CHotKey& right ) const { return m_virtualKeyCode == right.m_virtualKeyCode && m_modifiers == right.m_modifiers; }
		bool operator!=( const CHotKey& right ) const { return !operator==( right ); }
	public:
		WORD m_virtualKeyCode;
		WORD m_modifiers;
	};
}


// Enhances the MFC CHotKeyCtrl class:
//	- inhibits EN_CHANGE notifications on internal changes;
//	- maintains an isModified state, resets the modify flag when setting text programatically.

class CHotKeyCtrlEx : public CHotKeyCtrl
	, public CInternalChange
	, public ui::CMultiValueBase
{
	// hidden base methods
	using CHotKeyCtrl::SetHotKey;
	using CHotKeyCtrl::GetHotKey;
public:
	CHotKeyCtrlEx( void );
	virtual ~CHotKeyCtrlEx();

	bool IsModified( void ) const { return m_isModified; }
	void SetModified( bool isModified = true ) { m_isModified = isModified; }

	// hot key combination for the hot key control
	ui::CHotKey GetHotKey( void ) const;
	bool SetHotKey( const ui::CHotKey& hotKey );
	bool SetHotKey( WORD virtualKeyCode, WORD modifiers ) { return SetHotKey( ui::CHotKey( virtualKeyCode, modifiers ) ); }

	// ui::CMultiValueBase implementation
	virtual bool InMultiValuesMode( void ) const implement;
	bool SetMultiValuesMode( bool multiValuesMode = true );		// switch to MultiValues contents mode
private:
	bool m_isModified;					// modified by user
	bool m_multiValuesMode;

	// generated stuff
protected:
	afx_msg void OnPaint( void );
	afx_msg BOOL OnEnChange_Reflect( void );

	DECLARE_MESSAGE_MAP()
};


#endif // HotKeyCtrlEx_h

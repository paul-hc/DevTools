
#include "pch.h"
#include "HotKeyCtrlEx.h"
#include "WndUtils.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


CHotKeyCtrlEx::CHotKeyCtrlEx( void )
	: CHotKeyCtrl()
	, m_isModified( false )
	, m_multiValuesMode( false )
{
}

CHotKeyCtrlEx::~CHotKeyCtrlEx()
{
}

ui::CHotKey CHotKeyCtrlEx::GetHotKey( void ) const
{
	ui::CHotKey hotKey;
	__super::GetHotKey( hotKey.m_virtualKeyCode, hotKey.m_modifiers );
	return hotKey;
}

bool CHotKeyCtrlEx::SetHotKey( const ui::CHotKey& hotKey )
{
	CScopedInternalChange internalChange( this );

	SetModified( false );

	bool changed = SetMultiValuesMode( false );

	if ( hotKey == GetHotKey() )
		return changed;

	__super::SetHotKey( hotKey.m_virtualKeyCode, hotKey.m_modifiers );
	return true;
}

bool CHotKeyCtrlEx::InMultiValuesMode( void ) const implement
{
	return m_multiValuesMode;
}

bool CHotKeyCtrlEx::SetMultiValuesMode( bool multiValuesMode /*= true*/ )
{
	if ( !utl::ModifyValue( m_multiValuesMode, multiValuesMode ) )
		return false;

	Invalidate();
	return true;
}


// message handlers

BEGIN_MESSAGE_MAP( CHotKeyCtrlEx, CHotKeyCtrl )
	ON_WM_PAINT()
	ON_CONTROL_REFLECT_EX( EN_CHANGE, OnEnChange_Reflect )
END_MESSAGE_MAP()

void CHotKeyCtrlEx::OnPaint( void )
{
	if ( InMultiValuesMode() )
	{
		CPaintDC dc( this );

		DrawMultiValueText( &dc, this, ::GetSysColorBrush( COLOR_WINDOW ) );
	}
	else
		__super::OnPaint();
}

BOOL CHotKeyCtrlEx::OnEnChange_Reflect( void )
{
	if ( IsInternalChange() )		// don't propagate internal changes
		return TRUE;				// skip parent routing

	SetModified();
	SetMultiValuesMode( false );
	return FALSE;					// continue routing
}

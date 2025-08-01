#ifndef CtrlInterfaces_h
#define CtrlInterfaces_h
#pragma once


class CResizeGripBar;


namespace ui
{
	// implemented by controls that act as a layout frame, having custom internal layout rules
	//
	interface ILayoutFrame
	{
		virtual CWnd* GetControl( void ) const = 0;
		virtual CWnd* GetDialog( void ) const = 0;
		virtual void OnControlResized( void ) = 0;
		virtual bool ShowPane( bool show ) = 0;

		// optional layout pane methods:
		virtual CResizeGripBar* GetSplitterGripBar( void ) const { return nullptr; }
		virtual void SetSplitterGripBar( CResizeGripBar* pResizeGripBar ) { pResizeGripBar; }
	};


	// implemented by "editor frames" command targets, that manage controls with internal command routing, and require parent command routing when handlers exist
	//
	interface ICommandFrame
	{
		virtual CCmdTarget* GetCmdTarget( void ) = 0;
		virtual bool HandleTranslateMessage( MSG* pMsg ) = 0;		// called by the dialog on PreTranslateMessage
		virtual bool HandleCtrlCmdMsg( UINT id, int code, void* pExtra, AFX_CMDHANDLERINFO* pHandlerInfo ) = 0;
	};
}


#endif // CtrlInterfaces_h

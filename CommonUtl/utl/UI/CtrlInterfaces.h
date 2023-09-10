#ifndef CtrlInterfaces_h
#define CtrlInterfaces_h
#pragma once


namespace ui
{
	// implemented by controls that act as a layout frame, having custom internal layout rules
	//
	interface ILayoutFrame
	{
		virtual void OnControlResized( void ) = 0;
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

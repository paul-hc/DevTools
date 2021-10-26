#ifndef Dialog_fwd_h
#define Dialog_fwd_h
#pragma once


enum { DialogOutput, DialogSaveChanges };

class CLayoutPropertyPage;


namespace ui
{
	// implemented to customize command information (tooltips, message strings) by dialogs, frames, etc
	//
	interface ICustomCmdInfo
	{
		virtual void QueryTooltipText( std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const = 0;
	};


	// implemented by the parent of an embedded child page; routes deep control notifications to the parent
	//
	interface IEmbeddedPageCallback
	{
		virtual void OnChildPageNotify( CLayoutPropertyPage* pEmbeddedPage, CWnd* pCtrl, int notifCode ) = 0;
	};
}


#endif // Dialog_fwd_h

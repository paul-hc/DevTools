#ifndef Application_fwd_h
#define Application_fwd_h
#pragma once


namespace svc { interface ICommandService; }


namespace app
{
	enum CustomColors { ColorErrorBk = color::PastelPink };

	svc::ICommandService* GetCmdSvc( void );
}


namespace popup
{
	enum PopupBar { FormatPicker, MoreRenameActions, TextTools, TouchList, DuplicatesList };
}


#endif // Application_fwd_h

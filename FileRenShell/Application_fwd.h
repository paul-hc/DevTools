#ifndef Application_fwd_h
#define Application_fwd_h
#pragma once


namespace app
{
	enum MenuCommand
	{
		Cmd_SendToCliboard, Cmd_RenameFiles, Cmd_TouchFiles, Cmd_Undo,
		Cmd_RunUnitTests,
			_CmdCount
	};


	enum CustomColors { ColorErrorBk = color::PastelPink };
}


namespace popup
{
	enum PopupBar { FormatPicker, MoreRenameActions, TextTools, TouchList };
}


#endif // Application_fwd_h

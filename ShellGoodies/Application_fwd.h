#ifndef Application_fwd_h
#define Application_fwd_h
#pragma once


class CEnumTags;
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


namespace ren
{
	enum SortBy { SrcPathDirsFirst = -1, SrcPath, SrcSize, SrcDateModify, DestPath };

	typedef std::pair< SortBy, bool > TSortingPair;
}


#endif // Application_fwd_h

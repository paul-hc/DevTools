#ifndef Application_fwd_h
#define Application_fwd_h
#pragma once


class CEnumTags;
interface IFileEditor;
namespace svc { interface ICommandService; }


namespace app
{
	enum CustomColors { ColorErrorBk = color::PastelPink, ColorWarningText = color::SolidOrange };

	svc::ICommandService* GetCmdSvc( void );


	enum TargetScope { TargetAllItems, TargetSelectedItems };

	const CEnumTags& GetTags_TargetScope( void );

	void FlashCtrlFrame( CWnd* pCtrl );


	INT_PTR SafeExecuteDialog( IFileEditor* pFileEditor );
}


namespace popup
{
	enum PopupBar { FormatPicker, MoreRenameActions, TextTools, TouchList, DuplicatesList };
}


namespace ren
{
	enum SortBy { RecordDefault = -1, SrcPath, SrcSize, SrcDateModify, DestPath, SrcPathDirsFirst = RecordDefault };

	typedef std::pair<SortBy, bool> TSortingPair;
}


namespace reg
{
	extern const TCHAR section_Settings[];		// "Settings"
}


#endif // Application_fwd_h

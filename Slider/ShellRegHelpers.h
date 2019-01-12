#ifndef ShellRegHelpers_h
#define ShellRegHelpers_h
#pragma once


namespace shell_reg
{
	// shell registration helpers for HKEY_CLASSES_ROOT subkeys

	enum { ShellCommand, ShellDdeExec, ShellCmdCount = 2 };			// count of shell commands associated to any image extension (SLIDER_MENU_NAME_...)

//	static const TCHAR s_fmtValueCommand[] = _T("\"%s\"");

#define DIR_VERB_SLIDE_VIEW _T("Slide &View")

	// shell Directory verbs
	static const TCHAR s_dirVerb_SlideView[] = DIR_VERB_SLIDE_VIEW;

	static const TCHAR* s_dirHandler[ ShellCmdCount ] =
	{
		_T("Directory\\shell\\") DIR_VERB_SLIDE_VIEW _T("\\command"),
		_T("Directory\\shell\\") DIR_VERB_SLIDE_VIEW _T("\\ddeexec")
	};

#undef DIR_VERB_SLIDE_VIEW


#define IMG_VERB_OPEN_WITH_SLIDER _T("Open with &Slider")
#define IMG_VERB_QUEUE_IN_SLIDER  _T("Queue in &Slider")

	// image verbs
	static const TCHAR* s_fmtValueDdeExec[ ShellCmdCount ] = { _T("[open(\"%1\")]"), _T("[queue(\"%1\")]") };

	static const TCHAR s_imgVerb_OpenWithSlider[] = IMG_VERB_OPEN_WITH_SLIDER;
	static const TCHAR s_imgVerb_QueueInSlider[] = IMG_VERB_QUEUE_IN_SLIDER;

	static const TCHAR* s_fmtShlHandlerCommand[ ShellCmdCount ] =
	{
		_T("%s\\shell\\") IMG_VERB_OPEN_WITH_SLIDER _T("\\command"),
		_T("%s\\shell\\") IMG_VERB_QUEUE_IN_SLIDER _T("\\command")
	};
	static const TCHAR* s_fmtShlHandlerDdeExec[ ShellCmdCount ] =
	{
		_T("%s\\shell\\") IMG_VERB_OPEN_WITH_SLIDER _T("\\ddeexec"),
		_T("%s\\shell\\") IMG_VERB_QUEUE_IN_SLIDER _T("\\ddeexec")
	};

#undef IMG_VERB_OPEN_WITH_SLIDER
#undef IMG_VERB_QUEUE_IN_SLIDER
}


#endif // ShellRegHelpers_h

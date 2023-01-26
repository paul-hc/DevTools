
#include "stdafx.h"
#include "FlagRepository.h"
#include "utl/ContainerOwnership.h"
#include <unordered_map>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// definitions not included in latest version of header <commctrl.h>

// Header Control
#define HDS_VERT		0x00000001		// HDS_HORZ to get sense

// Toolbar extended styles
#ifndef TBSTYLE_EX_VERTICAL
#	define TBSTYLE_EX_VERTICAL                 0x00000004
#endif


#define FLAG( value, mask ) { value, mask, CFlagInfo::Editable, _T(#value) }
#define FLAG_READONLY( value, mask ) { value, mask, CFlagInfo::ReadOnly, _T(#value) }
#define FLAG_NAME( value, mask, name ) { value, mask, CFlagInfo::Editable, name }
#define FLAG_ALIASES( value, mask, aliases ) { value, mask, CFlagInfo::Editable, _T(#value)_T("/")_T(#aliases) }
#define GROUP_SEPARATOR( pName ) { 0, 0, CFlagInfo::Separator, pName }


#define BAR_COMMCTRL_FLAGS\
	GROUP_SEPARATOR( _T("Align (Common Control)") ),\
	FLAG( CCS_TOP, 0 ),\
	FLAG( CCS_NOMOVEY, 0 ),\
	FLAG( CCS_BOTTOM, 0 ),\
	FLAG( CCS_NORESIZE, 0 ),\
	GROUP_SEPARATOR( _T("Common Control") ),\
	FLAG( CCS_VERT, 0 ),\
	FLAG( CCS_NOPARENTALIGN, 0 ),\
	FLAG( CCS_ADJUSTABLE, 0 ),\
	FLAG( CCS_NODIVIDER, 0 )\


// STYLE DEFINITIONS

namespace style
{
	namespace general
	{
		CFlagInfo s_topLevelFlags[] =
		{
			GROUP_SEPARATOR( _T("Top Level") ),
			FLAG( WS_OVERLAPPED, WS_POPUP ),
			FLAG( WS_POPUP, WS_POPUP ),
			GROUP_SEPARATOR( _T("General") ),
			FLAG( WS_VISIBLE, 0 ),
			FLAG( WS_DISABLED, 0 ),
			FLAG( WS_CLIPSIBLINGS, 0 ),
			FLAG( WS_CLIPCHILDREN, 0 ),
			FLAG( WS_BORDER, 0 ),
			FLAG( WS_DLGFRAME, 0 ),
			FLAG( WS_THICKFRAME, 0 ),
			FLAG( WS_SYSMENU, 0 ),
			FLAG( WS_VSCROLL, 0 ),
			FLAG( WS_HSCROLL, 0 ),
			FLAG( WS_MINIMIZE, 0 ),
			FLAG( WS_MAXIMIZE, 0 ),
			FLAG( WS_MINIMIZEBOX, 0 ),
			FLAG( WS_MAXIMIZEBOX, 0 )
		};

		CFlagInfo s_childFlags[] =
		{
			GROUP_SEPARATOR( _T("Child") ),
			FLAG_READONLY( WS_CHILD, 0 ),
			GROUP_SEPARATOR( _T("General") ),
			FLAG( WS_VISIBLE, 0 ),
			FLAG( WS_DISABLED, 0 ),
			FLAG( WS_GROUP, 0 ),
			FLAG( WS_TABSTOP, 0 ),
			FLAG( WS_BORDER, 0 ),
			FLAG( WS_DLGFRAME, 0 ),
			FLAG( WS_THICKFRAME, 0 ),
			FLAG( WS_CLIPSIBLINGS, 0 ),
			FLAG( WS_CLIPCHILDREN, 0 ),
			FLAG( WS_VSCROLL, 0 ),
			FLAG( WS_HSCROLL, 0 ),
			FLAG( WS_SYSMENU, 0 ),
			FLAG( WS_MINIMIZE, 0 ),
			FLAG( WS_MAXIMIZE, 0 )
		};
	} // namespace general


	namespace specific
	{
		CFlagInfo s_dialogFlags[] =
		{
			FLAG( DS_3DLOOK, 0 ),
			FLAG( DS_SETFONT, 0 ),
			FLAG( DS_FIXEDSYS, 0 ),
			FLAG( DS_MODALFRAME, 0 ),
			GROUP_SEPARATOR( NULL ),
			FLAG( DS_CONTROL, 0 ),
			FLAG_READONLY( DS_SYSMODAL, 0 ),
			GROUP_SEPARATOR( _T("Alignment") ),
			FLAG( DS_ABSALIGN, 0 ),
			FLAG( DS_CENTER, 0 ),
			FLAG( DS_CENTERMOUSE, 0 ),
			GROUP_SEPARATOR( _T("More") ),
			FLAG( DS_LOCALEDIT, 0 ),
			FLAG( DS_NOIDLEMSG, 0 ),
			FLAG( DS_SETFOREGROUND, 0 ),
			FLAG( DS_NOFAILCREATE, 0 ),
			FLAG( DS_CONTEXTHELP, 0 )
		};

		CFlagInfo s_popupMenuFlags[] =
		{
			{ 0, 1, CFlagInfo::ReadOnly, _T("<<n/a>>") }
		};

		CFlagInfo s_staticFlags[] =
		{
			GROUP_SEPARATOR( _T("Static Type") ),
			FLAG( SS_LEFT, SS_TYPEMASK ),
			FLAG( SS_CENTER, SS_TYPEMASK ),
			FLAG( SS_RIGHT, SS_TYPEMASK ),
			FLAG( SS_ICON, SS_TYPEMASK ),
			FLAG( SS_BLACKRECT, SS_TYPEMASK ),
			FLAG( SS_GRAYRECT, SS_TYPEMASK ),
			FLAG( SS_WHITERECT, SS_TYPEMASK ),
			FLAG( SS_BLACKFRAME, SS_TYPEMASK ),
			FLAG( SS_GRAYFRAME, SS_TYPEMASK ),
			FLAG( SS_WHITEFRAME, SS_TYPEMASK ),
			FLAG( SS_USERITEM, SS_TYPEMASK ),
			FLAG( SS_SIMPLE, SS_TYPEMASK ),
			FLAG( SS_LEFTNOWORDWRAP, SS_TYPEMASK ),
			FLAG( SS_OWNERDRAW, SS_TYPEMASK ),
			FLAG( SS_BITMAP, SS_TYPEMASK ),
			FLAG( SS_ENHMETAFILE, SS_TYPEMASK ),
			FLAG( SS_ETCHEDHORZ, SS_TYPEMASK ),
			FLAG( SS_ETCHEDVERT, SS_TYPEMASK ),
			FLAG( SS_ETCHEDFRAME, SS_TYPEMASK ),
			FLAG( SS_TYPEMASK, SS_TYPEMASK ),
			GROUP_SEPARATOR( _T("Options") ),
			FLAG( SS_NOPREFIX, 0 ),
			FLAG( SS_NOTIFY, 0 ),
			FLAG( SS_CENTERIMAGE, 0 ),
			FLAG( SS_RIGHTJUST, 0 ),
			FLAG( SS_REALSIZEIMAGE, 0 ),
			FLAG( SS_SUNKEN, 0 ),
			FLAG( SS_ENDELLIPSIS, 0 ),
			FLAG( SS_PATHELLIPSIS, 0 )
		};

		CFlagInfo s_editFlags[] =
		{
			GROUP_SEPARATOR( _T("Align") ),
			FLAG( ES_LEFT, ES_CENTER | ES_RIGHT ),
			FLAG( ES_CENTER, ES_CENTER | ES_RIGHT ),
			FLAG( ES_RIGHT, ES_CENTER | ES_RIGHT ),
			GROUP_SEPARATOR( NULL ),
			FLAG( ES_MULTILINE, 0 ),
			FLAG( ES_NUMBER, 0 ),
			FLAG( ES_READONLY, 0 ),
			GROUP_SEPARATOR( NULL ),
			FLAG( ES_AUTOVSCROLL, 0 ),
			FLAG( ES_AUTOHSCROLL, 0 ),
			FLAG( ES_NOHIDESEL, 0 ),
			FLAG( ES_OEMCONVERT, 0 ),
			FLAG( ES_WANTRETURN, 0 ),
			FLAG( ES_UPPERCASE, 0 ),
			FLAG( ES_LOWERCASE, 0 ),
			FLAG( ES_PASSWORD, 0 )
		};

		CFlagInfo s_richEditFlags[] =
		{
			GROUP_SEPARATOR( _T("Align") ),
			FLAG( ES_LEFT, ES_CENTER | ES_RIGHT ),
			FLAG( ES_CENTER, ES_CENTER | ES_RIGHT ),
			FLAG( ES_RIGHT, ES_CENTER | ES_RIGHT ),
			GROUP_SEPARATOR( NULL ),
			FLAG( ES_MULTILINE, 0 ),
			FLAG( ES_READONLY, 0 ),
			GROUP_SEPARATOR( NULL ),
			FLAG( ES_AUTOVSCROLL, 0 ),
			FLAG( ES_AUTOHSCROLL, 0 ),
			FLAG( ES_NOHIDESEL, 0 ),
			FLAG( ES_OEMCONVERT, 0 ),
			FLAG( ES_WANTRETURN, 0 ),
			FLAG( ES_LOWERCASE, 0 ),
			FLAG( ES_PASSWORD, 0 ),
			GROUP_SEPARATOR( _T("Rich Edit") ),
			// rich edit specific
			FLAG( ES_SAVESEL, 0 ),
			FLAG( ES_SUNKEN, 0 ),
			FLAG( ES_DISABLENOSCROLL, 0 ),		// overrides ES_NUMBER
			FLAG( ES_SELECTIONBAR, 0 ),			// overrides WS_MAXIMIZE
			FLAG( ES_NOOLEDRAGDROP, 0 )			// overrides ES_UPPERCASE
		};

		#define BS_ALIGN_H_MASK	( BS_LEFT | BS_RIGHT | BS_CENTER )
		#define BS_ALIGN_V_MASK	( BS_TOP | BS_BOTTOM | BS_VCENTER )

		CFlagInfo s_buttonFlags[] =
		{
			GROUP_SEPARATOR( _T("Button Type") ),
			FLAG( BS_PUSHBUTTON, BS_TYPEMASK ),
			FLAG( BS_DEFPUSHBUTTON, BS_TYPEMASK ),
			FLAG( BS_CHECKBOX, BS_TYPEMASK ),
			FLAG( BS_AUTOCHECKBOX, BS_TYPEMASK ),
			FLAG( BS_RADIOBUTTON, BS_TYPEMASK ),
			FLAG( BS_3STATE, BS_TYPEMASK ),
			FLAG( BS_AUTO3STATE, BS_TYPEMASK ),
			FLAG( BS_GROUPBOX, BS_TYPEMASK ),
			FLAG( BS_USERBUTTON, BS_TYPEMASK ),
			FLAG( BS_AUTORADIOBUTTON, BS_TYPEMASK ),
			FLAG( BS_OWNERDRAW, BS_TYPEMASK ),
			FLAG( BS_SPLITBUTTON, BS_TYPEMASK ),		// CommCtrl.h
			FLAG( BS_DEFSPLITBUTTON, BS_TYPEMASK ),
			FLAG( BS_COMMANDLINK, BS_TYPEMASK ),
			FLAG( BS_DEFCOMMANDLINK, BS_TYPEMASK ),
			FLAG( BS_LEFTTEXT, 0 ),
			GROUP_SEPARATOR( _T("Content") ),
			FLAG( BS_TEXT, BS_ICON | BS_BITMAP ),
			FLAG( BS_ICON, BS_ICON | BS_BITMAP ),
			FLAG( BS_BITMAP, BS_ICON | BS_BITMAP ),
			GROUP_SEPARATOR( _T("Horizontal Align") ),
			FLAG( BS_LEFT, BS_ALIGN_H_MASK ),
			FLAG( BS_RIGHT, BS_ALIGN_H_MASK ),
			FLAG( BS_CENTER, BS_ALIGN_H_MASK ),
			GROUP_SEPARATOR( _T("Vertical Align") ),
			FLAG( BS_TOP, BS_ALIGN_V_MASK ),
			FLAG( BS_BOTTOM, BS_ALIGN_V_MASK ),
			FLAG( BS_VCENTER, BS_ALIGN_V_MASK ),
			GROUP_SEPARATOR( _T("More") ),
			FLAG( BS_PUSHLIKE, 0 ),
			FLAG( BS_MULTILINE, 0 ),
			FLAG( BS_NOTIFY, 0 ),
			FLAG( BS_FLAT, 0 )
		};

		CFlagInfo s_comboBoxFlags[] =
		{
			GROUP_SEPARATOR( _T("Combo Type") ),
			FLAG( CBS_SIMPLE, CBS_SIMPLE | CBS_DROPDOWN | CBS_DROPDOWNLIST ),
			FLAG( CBS_DROPDOWN, CBS_SIMPLE | CBS_DROPDOWN | CBS_DROPDOWNLIST ),
			FLAG( CBS_DROPDOWNLIST, CBS_SIMPLE | CBS_DROPDOWN | CBS_DROPDOWNLIST ),
			GROUP_SEPARATOR( _T("Owner-draw") ),
			FLAG_NAME( 0, CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE, _T("[default]") ),
			FLAG( CBS_OWNERDRAWFIXED, CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE ),
			FLAG( CBS_OWNERDRAWVARIABLE, CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE ),
			GROUP_SEPARATOR( _T("Options") ),
			FLAG( CBS_AUTOHSCROLL, 0 ),
			FLAG( CBS_OEMCONVERT, 0 ),
			FLAG( CBS_SORT, 0 ),
			FLAG( CBS_HASSTRINGS, 0 ),
			FLAG( CBS_NOINTEGRALHEIGHT, 0 ),
			FLAG( CBS_DISABLENOSCROLL, 0 ),
			FLAG( CBS_UPPERCASE, 0 ),
			FLAG( CBS_LOWERCASE, 0 )
		};

		CFlagInfo s_listBoxFlags[] =
		{
			GROUP_SEPARATOR( _T("Behavior") ),
			FLAG( LBS_MULTIPLESEL, 0 ),
			FLAG( LBS_EXTENDEDSEL, 0 ),
			FLAG( LBS_NOSEL, 0 ),
			GROUP_SEPARATOR( NULL ),
			FLAG( LBS_NOTIFY, 0 ),
			FLAG( LBS_SORT, 0 ),
			FLAG( LBS_NOREDRAW, 0 ),
			GROUP_SEPARATOR( _T("Owner-draw") ),
			FLAG_NAME( 0, LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE, _T("[default]") ),
			FLAG( LBS_OWNERDRAWFIXED, LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE ),
			FLAG( LBS_OWNERDRAWVARIABLE, LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE ),
			GROUP_SEPARATOR( _T("Options") ),
			FLAG( LBS_HASSTRINGS, 0 ),
			FLAG( LBS_NOINTEGRALHEIGHT, 0 ),
			FLAG( LBS_USETABSTOPS, 0 ),
			FLAG( LBS_MULTICOLUMN, 0 ),
			FLAG( LBS_WANTKEYBOARDINPUT, 0 ),
			FLAG( LBS_DISABLENOSCROLL, 0 ),
			FLAG( LBS_NODATA, 0 )
		};

		CFlagInfo s_scrollBarFlags[] =
		{
			GROUP_SEPARATOR( _T("Orientation") ),
			FLAG( SBS_HORZ, SBS_HORZ | SBS_VERT ),
			FLAG( SBS_VERT, SBS_HORZ | SBS_VERT ),
			GROUP_SEPARATOR( _T("Align") ),
			FLAG( SBS_TOPALIGN, 0 ),
			FLAG( SBS_BOTTOMALIGN, 0 ),
			GROUP_SEPARATOR( _T("Options") ),
			FLAG( SBS_SIZEBOX, 0 ),
			FLAG( SBS_SIZEGRIP, 0 )
		};

		CFlagInfo s_spinButtonFlags[] =
		{	// no groups
			GROUP_SEPARATOR( _T("Align") ),
			FLAG_NAME( 0, UDS_ALIGNRIGHT | UDS_ALIGNLEFT, _T("[not aligned]") ),
			FLAG( UDS_ALIGNRIGHT, UDS_ALIGNRIGHT | UDS_ALIGNLEFT ),
			FLAG( UDS_ALIGNLEFT, UDS_ALIGNRIGHT | UDS_ALIGNLEFT ),
			GROUP_SEPARATOR( _T("Options") ),
			FLAG_READONLY( UDS_AUTOBUDDY, 0 ),
			FLAG( UDS_SETBUDDYINT, 0 ),
			FLAG( UDS_HORZ, 0 ),
			FLAG( UDS_ARROWKEYS, 0 ),
			FLAG( UDS_NOTHOUSANDS, 0 ),
			FLAG( UDS_HOTTRACK, 0 ),
			FLAG( UDS_WRAP, 0 )
		};

		CFlagInfo s_sliderFlags[] =
		{
			FLAG( TBS_AUTOTICKS, 0 ),
			FLAG( TBS_NOTICKS, 0 ),
			FLAG( TBS_ENABLESELRANGE, 0 ),
			FLAG( TBS_FIXEDLENGTH, 0 ),
			FLAG( TBS_NOTHUMB, 0 ),
			FLAG( TBS_TOOLTIPS, 0 ),
			GROUP_SEPARATOR( _T("Orientation") ),
			FLAG( TBS_VERT, 0 ),
			FLAG( TBS_TOP, 0 ),
			FLAG( TBS_BOTH, 0 )
		};

		CFlagInfo s_hotKeyFlags[] =
		{
			BAR_COMMCTRL_FLAGS
		};

		CFlagInfo s_headerCtrlFlags[] =
		{
			FLAG_READONLY( HDS_HORZ, HDS_VERT ),
			FLAG_READONLY( HDS_VERT, HDS_VERT ),
			FLAG( HDS_BUTTONS, 0 ),
			FLAG( HDS_HOTTRACK, 0 ),
			FLAG( HDS_HIDDEN, 0 ),
			FLAG( HDS_DRAGDROP, 0 ),
			FLAG( HDS_FULLDRAG, 0 ),
			BAR_COMMCTRL_FLAGS
		};

		CFlagInfo s_listCtrlFlags[] =
		{
			GROUP_SEPARATOR( _T("List View Mode") ),
			FLAG( LVS_ICON, LVS_TYPEMASK ),
			FLAG( LVS_REPORT, LVS_TYPEMASK ),
			FLAG( LVS_SMALLICON, LVS_TYPEMASK ),
			FLAG( LVS_LIST, LVS_TYPEMASK ),
			GROUP_SEPARATOR( _T("Sorting") ),
			FLAG_NAME( 0, LVS_SORTASCENDING | LVS_SORTDESCENDING, _T("[not sorted]") ),
			FLAG( LVS_SORTASCENDING, LVS_SORTASCENDING | LVS_SORTDESCENDING ),
			FLAG( LVS_SORTDESCENDING, LVS_SORTASCENDING | LVS_SORTDESCENDING ),
			GROUP_SEPARATOR( _T("Align") ),
			FLAG( LVS_ALIGNTOP, LVS_ALIGNMASK ),
			FLAG( LVS_ALIGNLEFT, LVS_ALIGNMASK ),
			GROUP_SEPARATOR( NULL ),
			FLAG( LVS_SINGLESEL, 0 ),
			FLAG( LVS_SHOWSELALWAYS, 0 ),
			FLAG( LVS_SHAREIMAGELISTS, 0 ),
			FLAG( LVS_NOLABELWRAP, 0 ),
			FLAG( LVS_AUTOARRANGE, 0 ),
			FLAG( LVS_EDITLABELS, 0 ),
			FLAG( LVS_OWNERDATA, 0 ),
			FLAG( LVS_NOSCROLL, 0 ),
			FLAG( LVS_OWNERDRAWFIXED, 0 ),
			FLAG( LVS_NOCOLUMNHEADER, 0 ),
			FLAG( LVS_NOSORTHEADER, 0 )
		};

		CFlagInfo s_treeCtrlFlags[] =
		{
			FLAG( TVS_HASBUTTONS, 0 ),
			FLAG( TVS_HASLINES, 0 ),
			FLAG( TVS_LINESATROOT, 0 ),
			FLAG( TVS_EDITLABELS, 0 ),
			FLAG( TVS_DISABLEDRAGDROP, 0 ),
			FLAG( TVS_SHOWSELALWAYS, 0 ),
			FLAG( TVS_RTLREADING, 0 ),
			FLAG( TVS_NOTOOLTIPS, 0 ),
			FLAG( TVS_CHECKBOXES, 0 ),
			FLAG( TVS_TRACKSELECT, 0 ),
			GROUP_SEPARATOR( _T("Internet Explorer 4+") ),
			FLAG( TVS_SINGLEEXPAND, 0 ),
			FLAG( TVS_INFOTIP, 0 ),
			FLAG( TVS_FULLROWSELECT, 0 ),
			FLAG( TVS_NOSCROLL, 0 ),
			FLAG( TVS_NONEVENHEIGHT, 0 ),
			GROUP_SEPARATOR( _T("Internet Explorer 5+") ),
			FLAG( TVS_NOHSCROLL, 0 )
		};

		CFlagInfo s_tabCtrlFlags[] =
		{
			FLAG_NAME( 0, TCS_FOCUSONBUTTONDOWN | TCS_FOCUSNEVER, _T("[default]") ),
			FLAG( TCS_FOCUSONBUTTONDOWN, TCS_FOCUSONBUTTONDOWN | TCS_FOCUSNEVER ),
			FLAG( TCS_FOCUSNEVER, TCS_FOCUSONBUTTONDOWN | TCS_FOCUSNEVER ),
			GROUP_SEPARATOR( _T("Style") ),
			FLAG( TCS_TABS, TCS_BUTTONS ),
			FLAG( TCS_BUTTONS, TCS_BUTTONS ),
			GROUP_SEPARATOR( NULL ),
			FLAG( TCS_SCROLLOPPOSITE, 0 ),
			FLAG_ALIASES( TCS_RIGHT, 0, TCS_BOTTOM ),
			FLAG( TCS_MULTISELECT, 0 ),
			FLAG( TCS_FORCEICONLEFT, 0 ),
			FLAG( TCS_FORCELABELLEFT, 0 ),
			FLAG( TCS_HOTTRACK, 0 ),
			FLAG( TCS_VERTICAL, 0 ),
			FLAG( TCS_MULTILINE, 0 ),
			FLAG( TCS_FIXEDWIDTH, 0 ),
			FLAG( TCS_RAGGEDRIGHT, 0 ),
			GROUP_SEPARATOR( NULL ),
			FLAG( TCS_OWNERDRAWFIXED, 0 ),
			FLAG( TCS_TOOLTIPS, 0 ),
			GROUP_SEPARATOR( _T("Internet Explorer 4+") ),
			FLAG( TCS_FLATBUTTONS, 0 )
		};

		CFlagInfo s_monthCalendarFlags[] =
		{
			FLAG( MCS_DAYSTATE, 0 ),
			FLAG( MCS_MULTISELECT, 0 ),
			FLAG( MCS_WEEKNUMBERS, 0 ),
			FLAG( MCS_NOTODAYCIRCLE, 0 ),
			FLAG( MCS_NOTODAY, 0 )
		};

		CFlagInfo s_dateTimeCtrlFlags[] =
		{
			// [PC]: Strange DTS_UPDOWN and DTS_TIMEFORMAT styles overlap
			FLAG( DTS_UPDOWN, DTS_UPDOWN | DTS_TIMEFORMAT ),
			FLAG( DTS_TIMEFORMAT, DTS_UPDOWN | DTS_TIMEFORMAT ),
			GROUP_SEPARATOR( NULL ),
			FLAG( DTS_SHOWNONE, 0 ),
			FLAG( DTS_SHORTDATEFORMAT, 0 ),
			FLAG( DTS_LONGDATEFORMAT, 0 ),
			FLAG( DTS_APPCANPARSE, 0 ),
			FLAG( DTS_RIGHTALIGN, 0 ),
			GROUP_SEPARATOR( _T("Internet Explorer 5+") ),
			FLAG( DTS_SHORTDATECENTURYFORMAT, 0 )
		};

		CFlagInfo s_progressBarFlags[] =
		{
			FLAG( PBS_SMOOTH, 0 ),
			FLAG( PBS_VERTICAL, 0 )
		};

		CFlagInfo s_animateCtrlFlags[] =
		{
			FLAG( ACS_CENTER, 0 ),
			FLAG( ACS_TRANSPARENT, 0 ),
			FLAG( ACS_AUTOPLAY, 0 ),
			FLAG( ACS_TIMER, 0 )
		};

		CFlagInfo s_tooltipCtrlFlags[] =
		{
			FLAG( TTS_ALWAYSTIP, 0 ),
			FLAG( TTS_NOPREFIX, 0 ),
			FLAG( TTS_NOANIMATE, 0 ),
			FLAG( TTS_NOFADE, 0 ),
			FLAG( TTS_BALLOON, 0 ),
			FLAG( TTS_CLOSE, 0 ),
			GROUP_SEPARATOR( _T("Windows Vista+") ),
			FLAG( TTS_USEVISUALSTYLE, 0 )
		};

		CFlagInfo s_toolBarFlags[] =
		{
			FLAG( TBSTYLE_TOOLTIPS, 0 ),
			FLAG( TBSTYLE_WRAPABLE, 0 ),
			FLAG( TBSTYLE_ALTDRAG, 0 ),
			FLAG( TBSTYLE_FLAT, 0 ),
			FLAG( TBSTYLE_LIST, 0 ),
			FLAG( TBSTYLE_CUSTOMERASE, 0 ),
			FLAG( TBSTYLE_REGISTERDROP, 0 ),
			FLAG( TBSTYLE_TRANSPARENT, 0 ),
			BAR_COMMCTRL_FLAGS
		};

		CFlagInfo s_reBarFlags[] =
		{
			FLAG( RBS_TOOLTIPS, 0 ),
			FLAG( RBS_VARHEIGHT, 0 ),
			FLAG( RBS_BANDBORDERS, 0 ),
			FLAG( RBS_FIXEDORDER, 0 ),
			FLAG( RBS_REGISTERDROP, 0 ),
			FLAG( RBS_AUTOSIZE, 0 ),
			FLAG( RBS_VERTICALGRIPPER, 0 ),
			FLAG( RBS_DBLCLKTOGGLE, 0 ),
			GROUP_SEPARATOR( NULL ),
			FLAG( RBS_TOOLTIPS, 0 ),
			FLAG( RBS_VARHEIGHT, 0 ),
			FLAG( RBS_BANDBORDERS, 0 ),
			FLAG( RBS_FIXEDORDER, 0 ),
			BAR_COMMCTRL_FLAGS
		};

		CFlagInfo s_statusBarFlags[] =
		{
			FLAG( SBARS_SIZEGRIP, 0 ),
			GROUP_SEPARATOR( _T("Internet Explorer 5+") ),
			FLAG( SBARS_TOOLTIPS, 0 ),
			BAR_COMMCTRL_FLAGS
		};

	} // namespace specific

} //namespace style


// STYLE-EX DEFINITIONS

namespace style_ex
{
	CFlagInfo s_generalExFlags[] =
	{
		FLAG( WS_EX_WINDOWEDGE, 0 ),
		FLAG( WS_EX_STATICEDGE, 0 ),
		FLAG( WS_EX_CLIENTEDGE, 0 ),
		FLAG( WS_EX_DLGMODALFRAME, 0 ),
		GROUP_SEPARATOR( NULL ),
		FLAG( WS_EX_TOPMOST, 0 ),
		FLAG( WS_EX_APPWINDOW, 0 ),
		FLAG( WS_EX_ACCEPTFILES, 0 ),
		FLAG( WS_EX_TRANSPARENT, 0 ),
		FLAG( WS_EX_NOPARENTNOTIFY, 0 ),
		GROUP_SEPARATOR( NULL ),
		FLAG( WS_EX_CONTROLPARENT, 0 ),
		FLAG( WS_EX_MDICHILD, 0 ),
		FLAG( WS_EX_TOOLWINDOW, 0 ),
		FLAG( WS_EX_CONTEXTHELP, 0 ),
		GROUP_SEPARATOR( NULL ),
		FLAG( WS_EX_LEFT, WS_EX_RIGHT ),
		FLAG( WS_EX_RIGHT, WS_EX_RIGHT ),
		GROUP_SEPARATOR( NULL ),
		FLAG( WS_EX_LTRREADING, WS_EX_RTLREADING ),
		FLAG( WS_EX_RTLREADING, WS_EX_RTLREADING ),
		GROUP_SEPARATOR( NULL ),
		FLAG( WS_EX_RIGHTSCROLLBAR, WS_EX_LEFTSCROLLBAR ),
		FLAG( WS_EX_LEFTSCROLLBAR, WS_EX_LEFTSCROLLBAR )
	};


	CFlagInfo s_comboBoxExFlags[] =
	{
		FLAG( CBES_EX_NOEDITIMAGE, 0 ),
		FLAG( CBES_EX_NOEDITIMAGEINDENT, 0 ),
		FLAG( CBES_EX_PATHWORDBREAKPROC, 0 )
	};


	CFlagInfo s_listCtrlExFlags[] =
	{
		FLAG( LVS_EX_GRIDLINES, 0 ),
		FLAG( LVS_EX_SUBITEMIMAGES, 0 ),
		FLAG( LVS_EX_CHECKBOXES, 0 ),
		FLAG( LVS_EX_TRACKSELECT, 0 ),
		FLAG( LVS_EX_HEADERDRAGDROP, 0 ),
		FLAG( LVS_EX_FULLROWSELECT, 0 ),
		FLAG( LVS_EX_ONECLICKACTIVATE, 0 ),
		FLAG( LVS_EX_TWOCLICKACTIVATE, 0 ),
		GROUP_SEPARATOR( NULL ),
		FLAG_READONLY( LVS_EX_FLATSB, 0 ),
		FLAG( LVS_EX_REGIONAL, 0 ),
		FLAG( LVS_EX_INFOTIP, 0 ),
		FLAG( LVS_EX_UNDERLINEHOT, 0 ),
		FLAG( LVS_EX_UNDERLINECOLD, 0 ),
		FLAG( LVS_EX_MULTIWORKAREAS, 0 ),
		GROUP_SEPARATOR( _T("Internet Explorer 5+") ),
		FLAG( LVS_EX_LABELTIP, 0 ),
		FLAG( LVS_EX_BORDERSELECT, 0 ),
		GROUP_SEPARATOR( _T("Windows XP+") ),
		FLAG( LVS_EX_DOUBLEBUFFER, 0 ),
		FLAG( LVS_EX_HIDELABELS, 0 ),
		FLAG( LVS_EX_SINGLEROW, 0 ),
		FLAG( LVS_EX_SNAPTOGRID, 0 ),
		FLAG( LVS_EX_SIMPLESELECT, 0 ),
		GROUP_SEPARATOR( _T("Windows Vista+") ),
		FLAG( LVS_EX_JUSTIFYCOLUMNS, 0 ),
		FLAG( LVS_EX_TRANSPARENTBKGND, 0 ),
		FLAG( LVS_EX_TRANSPARENTSHADOWTEXT, 0 ),
		FLAG( LVS_EX_AUTOAUTOARRANGE, 0 ),
		FLAG( LVS_EX_HEADERINALLVIEWS, 0 ),
		FLAG( LVS_EX_AUTOCHECKSELECT, 0 ),
		FLAG( LVS_EX_AUTOSIZECOLUMNS, 0 ),
		FLAG( LVS_EX_COLUMNSNAPPOINTS, 0 ),
		FLAG( LVS_EX_COLUMNOVERFLOW, 0 )
	};


	CFlagInfo s_treeCtrlExFlags[] =
	{
		GROUP_SEPARATOR( _T("Windows Vista+") ),
		FLAG( TVS_EX_MULTISELECT, 0 ),
		FLAG( TVS_EX_DOUBLEBUFFER, 0 ),
		FLAG( TVS_EX_NOINDENTSTATE, 0 ),
		FLAG( TVS_EX_RICHTOOLTIP, 0 ),
		FLAG( TVS_EX_AUTOHSCROLL, 0 ),
		FLAG( TVS_EX_FADEINOUTEXPANDOS, 0 ),
		FLAG( TVS_EX_PARTIALCHECKBOXES, 0 ),
		FLAG( TVS_EX_EXCLUSIONCHECKBOXES, 0 ),
		FLAG( TVS_EX_DIMMEDCHECKBOXES, 0 ),
		FLAG( TVS_EX_DRAWIMAGEASYNC, 0 )
	};


	CFlagInfo s_tabCtrlExFlags[] =
	{
		GROUP_SEPARATOR( _T("Internet Explorer 4+") ),
		FLAG( TCS_EX_FLATSEPARATORS, 0 ),
		FLAG( TCS_EX_REGISTERDROP, 0 )
	};


	CFlagInfo s_toolbarExFlags[] =
	{
		FLAG( TBSTYLE_EX_DRAWDDARROWS, 0 ),
		FLAG_ALIASES( TBSTYLE_WRAPABLE, 0, TBSTYLE_EX_MULTICOLUMN ),
		FLAG( TBSTYLE_EX_VERTICAL, 0 ),
		GROUP_SEPARATOR( NULL ),
		FLAG( TBSTYLE_EX_MIXEDBUTTONS, 0 ),
		FLAG( TBSTYLE_EX_HIDECLIPPEDBUTTONS, 0 ),
		GROUP_SEPARATOR( NULL ),
		FLAG( TBSTYLE_EX_DOUBLEBUFFER, 0 )
	};

} //namespace style


// CStyleRepository implementation

CStyleRepository::CStyleRepository( void )
	: m_generalTopLevelStore( _T(""), ARRAY_PAIR( style::general::s_topLevelFlags ), &GetWindowStyle, &SetWindowStyle )
	, m_generalChildStore( _T(""), ARRAY_PAIR( style::general::s_childFlags ), &GetWindowStyle, &SetWindowStyle )
{
	using namespace style;

	AddFlagStore( _T("#32768"), ARRAY_PAIR( specific::s_popupMenuFlags ) );
	AddFlagStore( _T("#32770"), ARRAY_PAIR( specific::s_dialogFlags ) );
	AddFlagStore( _T("Static"), ARRAY_PAIR( specific::s_staticFlags ) );
	AddFlagStore( _T("Edit"), ARRAY_PAIR( specific::s_editFlags ) );
	AddFlagStore( _T("RichEdit|RichEdit20A|RichEdit20W"), ARRAY_PAIR( specific::s_richEditFlags ) );
	AddFlagStore( _T("Button"), ARRAY_PAIR( specific::s_buttonFlags ) );
	AddFlagStore( _T("ComboBox|ComboBoxEx32"), ARRAY_PAIR( specific::s_comboBoxFlags ) );
	AddFlagStore( _T("ListBox|ComboLBox"), ARRAY_PAIR( specific::s_listBoxFlags ) );
	AddFlagStore( _T("ScrollBar"), ARRAY_PAIR( specific::s_scrollBarFlags ) );
	AddFlagStore( _T("msctls_UpDown32|msctls_UpDown"), ARRAY_PAIR( specific::s_spinButtonFlags ) );
	AddFlagStore( _T("msctls_TrackBar32|msctls_TrackBar"), ARRAY_PAIR( specific::s_sliderFlags ) );
	AddFlagStore( _T("msctls_HotKey32|msctls_HotKey"), ARRAY_PAIR( specific::s_hotKeyFlags ) );
	AddFlagStore( _T("SysHeader32|SysHeader"), ARRAY_PAIR( specific::s_headerCtrlFlags ) );
	AddFlagStore( _T("SysListView32|SysListView"), ARRAY_PAIR( specific::s_listCtrlFlags ) );
	AddFlagStore( _T("SysTreeView32|SysTreeView"), ARRAY_PAIR( specific::s_treeCtrlFlags ) );
	AddFlagStore( _T("SysTabControl32|SysTabControl"), ARRAY_PAIR( specific::s_tabCtrlFlags ) );
	AddFlagStore( _T("SysMonthCal32"), ARRAY_PAIR( specific::s_monthCalendarFlags ) );
	AddFlagStore( _T("SysDateTimePick32"), ARRAY_PAIR( specific::s_dateTimeCtrlFlags ) );
	AddFlagStore( _T("msctls_progress32|msctls_progress"), ARRAY_PAIR( specific::s_progressBarFlags ) );
	AddFlagStore( _T("SysAnimate32"), ARRAY_PAIR( specific::s_animateCtrlFlags ) );
	AddFlagStore( _T("tooltips_class32|tooltips_class"), ARRAY_PAIR( specific::s_tooltipCtrlFlags ) );
	AddFlagStore( _T("ToolbarWindow32|ToolbarWindow"), ARRAY_PAIR( specific::s_toolBarFlags ) );
	AddFlagStore( _T("ReBarWindow32|ReBarWindow"), ARRAY_PAIR( specific::s_reBarFlags ) );
	AddFlagStore( _T("msctls_statusbar32|msctls_statusbar"), ARRAY_PAIR( specific::s_statusBarFlags ) );
}

CStyleRepository::~CStyleRepository()
{
	utl::ClearOwningContainer( m_specificStores );
}

CStyleRepository& CStyleRepository::Instance( void )
{
	static CStyleRepository styleRepository;
	return styleRepository;
}

void CStyleRepository::AddFlagStore( const TCHAR* pWndClassAliases, CFlagInfo pFlagInfos[], unsigned int count )
{
	m_specificStores.push_back( new CFlagStore( pWndClassAliases, pFlagInfos, count, &GetWindowStyle, &SetWindowStyle ) );
}

const CFlagStore* CStyleRepository::FindSpecificStore( const std::tstring& wndClass ) const
{
	static std::unordered_map< std::tstring, CFlagStore* > classToStoreMap;
	if ( classToStoreMap.empty() )		// init map once
		for ( std::vector< CFlagStore* >::const_iterator itStore = m_specificStores.begin(); itStore != m_specificStores.end(); ++itStore )
			for ( std::vector< std::tstring >::const_iterator itWndClass = ( *itStore )->m_wndClasses.begin(); itWndClass != ( *itStore )->m_wndClasses.end(); ++itWndClass )
				classToStoreMap[ str::MakeUpper( *itWndClass ) ] = *itStore;		// class name key in upper case

	std::unordered_map< std::tstring, CFlagStore* >::const_iterator itFound = classToStoreMap.find( str::MakeUpper( wndClass ) );

	return itFound != classToStoreMap.end() ? itFound->second : NULL;
}

std::tstring& CStyleRepository::StreamStyle( std::tstring& rOutput, DWORD style, const std::tstring& wndClass, const TCHAR* pSep /*= stream::flagSep*/ ) const
{
	GetGeneralStore( HasFlag( style, WS_CHILD ) )->StreamFormatFlags( rOutput, style, pSep );

	if ( !wndClass.empty() )
		if ( const CFlagStore* pFlagStore = FindSpecificStore( wndClass ) )
			pFlagStore->StreamFormatFlags( rOutput, style, pSep );

	return rOutput;
}

DWORD CStyleRepository::GetWindowStyle( HWND hSrc )
{
	return ::GetWindowLong( hSrc, GWL_STYLE );
}

DWORD CStyleRepository::SetWindowStyle( HWND hDest, DWORD style )
{
	return ::SetWindowLong( hDest, GWL_STYLE, style );
}


// CStyleExRepository implementation

CStyleExRepository::CStyleExRepository( void )
{
	m_stores.push_back( new CFlagStore( _T(""), ARRAY_PAIR( style_ex::s_generalExFlags ), &GetWindowStyleEx, &SetWindowStyleEx ) );
	m_stores.push_back( new CFlagStore( _T("SysListView|SysListView32"), ARRAY_PAIR( style_ex::s_listCtrlExFlags ), &GetListViewStyleEx, &SetListViewStyleEx ) );
	m_stores.push_back( new CFlagStore( _T("SysTreeView|SysTreeView32"), ARRAY_PAIR( style_ex::s_treeCtrlExFlags ), &GetTreeViewStyleEx, &SetTreeViewStyleEx ) );
	m_stores.push_back( new CFlagStore( _T("SysTabControl|SysTabControl32"), ARRAY_PAIR( style_ex::s_tabCtrlExFlags ), &GetTabCtrlStyleEx, &SetTabCtrlStyleEx ) );
	m_stores.push_back( new CFlagStore( _T("ComboBoxEx32"), ARRAY_PAIR( style_ex::s_comboBoxExFlags ), &GetComboExStyleEx, &SetComboExStyleEx ) );
	m_stores.push_back( new CFlagStore( _T("ToolbarWindow|ToolbarWindow32"), ARRAY_PAIR( style_ex::s_toolbarExFlags ), &GetToolbarStyleEx, &SetToolbarStyleEx ) );
}

CStyleExRepository::~CStyleExRepository()
{
	utl::ClearOwningContainer( m_stores );
}

CStyleExRepository& CStyleExRepository::Instance( void )
{
	static CStyleExRepository styleExRepository;
	return styleExRepository;
}

const CFlagStore* CStyleExRepository::FindStore( const std::tstring& wndClass /*= std::tstring()*/ ) const
{
	static std::unordered_map< std::tstring, CFlagStore* > classToStoreMap;
	if ( classToStoreMap.empty() )		// init map once
		for ( std::vector< CFlagStore* >::const_iterator itStore = m_stores.begin(); itStore != m_stores.end(); ++itStore )
			for ( std::vector< std::tstring >::const_iterator itWndClass = ( *itStore )->m_wndClasses.begin(); itWndClass != ( *itStore )->m_wndClasses.end(); ++itWndClass )
				classToStoreMap[ str::MakeUpper( *itWndClass ) ] = *itStore;		// class name key in upper case

	std::unordered_map< std::tstring, CFlagStore* >::const_iterator itFound = classToStoreMap.find( str::MakeUpper( wndClass ) );

	return itFound != classToStoreMap.end() ? itFound->second : NULL;
}

std::tstring& CStyleExRepository::StreamStyleEx( std::tstring& rOutput, DWORD styleEx, const std::tstring& wndClass /*= std::tstring()*/, const TCHAR* pSep /*= stream::flagSep*/ ) const
{
	if ( const CFlagStore* pFlagStore = FindStore( wndClass ) )
		pFlagStore->StreamFormatFlags( rOutput, styleEx, pSep );

	return rOutput;
}

DWORD CStyleExRepository::GetWindowStyleEx( HWND hSrc )
{
	return ::GetWindowLong( hSrc, GWL_EXSTYLE );
}

DWORD CStyleExRepository::SetWindowStyleEx( HWND hDest, DWORD styleEx )
{
	DWORD orgStyleEx = GetWindowStyleEx( hDest );

	if ( ( styleEx ^ orgStyleEx ) & WS_EX_TOPMOST )
	{	// special handling if WS_EX_TOPMOST has changed
		// modify extended styles excepting WS_EX_TOPMOST
		::SetWindowLong( hDest, GWL_EXSTYLE, ( styleEx & ~WS_EX_TOPMOST ) | ( orgStyleEx & WS_EX_TOPMOST ) );

		ui::SetTopMost( hDest, HasFlag( styleEx, WS_EX_TOPMOST ) );		// finally modify WS_EX_TOPMOST
	}
	else
		::SetWindowLong( hDest, GWL_EXSTYLE, styleEx );

	return orgStyleEx;
}

DWORD CStyleExRepository::GetListViewStyleEx( HWND hSrc )
{
	return ListView_GetExtendedListViewStyle( hSrc );
}

DWORD CStyleExRepository::SetListViewStyleEx( HWND hDest, DWORD styleExClass )
{
	return ListView_SetExtendedListViewStyle( hDest, styleExClass );
}

DWORD CStyleExRepository::GetTreeViewStyleEx( HWND hSrc )
{
	return TreeView_GetExtendedStyle( hSrc );
}

DWORD CStyleExRepository::SetTreeViewStyleEx( HWND hDest, DWORD styleExClass )
{
	return TreeView_SetExtendedStyle( hDest, styleExClass, UINT_MAX );
}

DWORD CStyleExRepository::GetTabCtrlStyleEx( HWND hSrc )
{
	return TabCtrl_GetExtendedStyle( hSrc );
}

DWORD CStyleExRepository::SetTabCtrlStyleEx( HWND hDest, DWORD styleExClass )
{
	return TabCtrl_SetExtendedStyle( hDest, styleExClass );
}

DWORD CStyleExRepository::GetComboExStyleEx( HWND hSrc )
{
	return (DWORD)::SendMessage( hSrc, CBEM_GETEXTENDEDSTYLE, 0, 0 );
}

DWORD CStyleExRepository::SetComboExStyleEx( HWND hDest, DWORD styleExClass )
{
	return (DWORD)::SendMessage( hDest, CBEM_SETEXTENDEDSTYLE, 0, styleExClass );
}

DWORD CStyleExRepository::GetToolbarStyleEx( HWND hSrc )
{
	return (DWORD)::SendMessage( hSrc, TB_GETEXTENDEDSTYLE, 0, 0 );
}

DWORD CStyleExRepository::SetToolbarStyleEx( HWND hDest, DWORD styleExClass )
{
	return (DWORD)::SendMessage( hDest, TB_SETEXTENDEDSTYLE, styleExClass, 0 );
}

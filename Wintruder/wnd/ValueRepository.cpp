
#include "pch.h"
#include "ValueRepository.h"
#include "utl/ContainerOwnership.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


#define VALUE_INFO( value ) { static_cast<short>( value ), _T(#value), nullptr }
#define MFC_VALUE_INFO( value ) { (short)static_cast<unsigned short>( value ), _T(#value), nullptr }


// IDENTIFIER DEFINITIONS

namespace ident
{
	CValueInfo windows[] =
	{
		VALUE_INFO( IDC_STATIC ),
		VALUE_INFO( ID_SEPARATOR ),
		VALUE_INFO( IDOK ),
		VALUE_INFO( IDCANCEL ),
		VALUE_INFO( IDOK ),
		VALUE_INFO( IDCANCEL ),
		VALUE_INFO( IDABORT ),
		VALUE_INFO( IDRETRY ),
		VALUE_INFO( IDIGNORE ),
		VALUE_INFO( IDYES ),
		VALUE_INFO( IDNO ),
		VALUE_INFO( IDCLOSE ),
		VALUE_INFO( IDHELP ),
		VALUE_INFO( IDTRYAGAIN ),
		VALUE_INFO( IDCONTINUE ),
		// property sheet
		VALUE_INFO( ID_APPLY_NOW ),
		VALUE_INFO( ID_WIZBACK ),
		VALUE_INFO( ID_WIZNEXT ),
		VALUE_INFO( ID_WIZFINISH ),
		VALUE_INFO( AFX_IDC_TAB_CONTROL )
	};


	namespace mfc
	{
		CValueInfo frameAndView[] =
		{
			MFC_VALUE_INFO( AFX_IDW_PANE_FIRST ),
			MFC_VALUE_INFO( AFX_IDW_PANE_LAST ),
			MFC_VALUE_INFO( AFX_IDW_HSCROLL_FIRST ),
			MFC_VALUE_INFO( AFX_IDW_VSCROLL_FIRST ),
			MFC_VALUE_INFO( AFX_IDW_SIZE_BOX ),
			MFC_VALUE_INFO( AFX_IDW_PANE_SAVE )
		};

		CValueInfo misc[] =
		{
			MFC_VALUE_INFO( AFX_IDS_APP_TITLE ),
			MFC_VALUE_INFO( AFX_IDS_IDLEMESSAGE ),
			MFC_VALUE_INFO( AFX_IDS_HELPMODEMESSAGE ),
			MFC_VALUE_INFO( AFX_IDS_APP_TITLE_EMBEDDING ),
			MFC_VALUE_INFO( AFX_IDS_COMPANY_NAME ),
			MFC_VALUE_INFO( AFX_IDS_OBJ_TITLE_INPLACE ),
			MFC_VALUE_INFO( AFX_IDM_FIRST_MDICHILD )
		};

		CValueInfo controlBar[] =
		{
			MFC_VALUE_INFO( AFX_IDW_CONTROLBAR_FIRST ),
			MFC_VALUE_INFO( AFX_IDW_CONTROLBAR_LAST ),
			MFC_VALUE_INFO( AFX_IDW_TOOLBAR ),
			MFC_VALUE_INFO( AFX_IDW_STATUS_BAR ),
			MFC_VALUE_INFO( AFX_IDW_PREVIEW_BAR ),
			MFC_VALUE_INFO( AFX_IDW_RESIZE_BAR ),
			MFC_VALUE_INFO( AFX_IDW_REBAR ),
			MFC_VALUE_INFO( AFX_IDW_DIALOGBAR ),
			MFC_VALUE_INFO( AFX_IDW_DOCKBAR_TOP ),
			MFC_VALUE_INFO( AFX_IDW_DOCKBAR_LEFT ),
			MFC_VALUE_INFO( AFX_IDW_DOCKBAR_RIGHT ),
			MFC_VALUE_INFO( AFX_IDW_DOCKBAR_BOTTOM ),
			MFC_VALUE_INFO( AFX_IDW_DOCKBAR_FLOAT )
		};

		CValueInfo statusBar[] =
		{
			MFC_VALUE_INFO( ID_INDICATOR_EXT ),
			MFC_VALUE_INFO( ID_INDICATOR_CAPS ),
			MFC_VALUE_INFO( ID_INDICATOR_NUM ),
			MFC_VALUE_INFO( ID_INDICATOR_SCRL ),
			MFC_VALUE_INFO( ID_INDICATOR_OVR ),
			MFC_VALUE_INFO( ID_INDICATOR_REC ),
			MFC_VALUE_INFO( ID_INDICATOR_KANA )
		};

		CValueInfo menu[] =
		{
			MFC_VALUE_INFO( ID_FILE_NEW ),
			MFC_VALUE_INFO( ID_FILE_OPEN ),
			MFC_VALUE_INFO( ID_FILE_CLOSE ),
			MFC_VALUE_INFO( ID_FILE_SAVE ),
			MFC_VALUE_INFO( ID_FILE_SAVE_AS ),
			MFC_VALUE_INFO( ID_FILE_PAGE_SETUP ),
			MFC_VALUE_INFO( ID_FILE_PRINT_SETUP ),
			MFC_VALUE_INFO( ID_FILE_PRINT ),
			MFC_VALUE_INFO( ID_FILE_PRINT_DIRECT ),
			MFC_VALUE_INFO( ID_FILE_PRINT_PREVIEW ),
			MFC_VALUE_INFO( ID_FILE_UPDATE ),
			MFC_VALUE_INFO( ID_FILE_SAVE_COPY_AS ),
			MFC_VALUE_INFO( ID_FILE_SEND_MAIL ),
			MFC_VALUE_INFO( ID_FILE_NEW_FRAME ),
			MFC_VALUE_INFO( ID_FILE_MRU_FILE1 ),
			MFC_VALUE_INFO( ID_FILE_MRU_FILE2 ),
			MFC_VALUE_INFO( ID_FILE_MRU_FILE3 ),
			MFC_VALUE_INFO( ID_FILE_MRU_FILE4 ),
			MFC_VALUE_INFO( ID_FILE_MRU_FILE5 ),
			MFC_VALUE_INFO( ID_FILE_MRU_FILE6 ),
			MFC_VALUE_INFO( ID_FILE_MRU_FILE7 ),
			MFC_VALUE_INFO( ID_FILE_MRU_FILE8 ),
			MFC_VALUE_INFO( ID_FILE_MRU_FILE9 ),
			MFC_VALUE_INFO( ID_FILE_MRU_FILE10 ),
			MFC_VALUE_INFO( ID_FILE_MRU_FILE11 ),
			MFC_VALUE_INFO( ID_FILE_MRU_FILE12 ),
			MFC_VALUE_INFO( ID_FILE_MRU_FILE13 ),
			MFC_VALUE_INFO( ID_FILE_MRU_FILE14 ),
			MFC_VALUE_INFO( ID_FILE_MRU_FILE15 ),
			MFC_VALUE_INFO( ID_FILE_MRU_FILE16 ),
			MFC_VALUE_INFO( ID_EDIT_CLEAR ),
			MFC_VALUE_INFO( ID_EDIT_CLEAR_ALL ),
			MFC_VALUE_INFO( ID_EDIT_COPY ),
			MFC_VALUE_INFO( ID_EDIT_CUT ),
			MFC_VALUE_INFO( ID_EDIT_FIND ),
			MFC_VALUE_INFO( ID_EDIT_PASTE ),
			MFC_VALUE_INFO( ID_EDIT_PASTE_LINK ),
			MFC_VALUE_INFO( ID_EDIT_PASTE_SPECIAL ),
			MFC_VALUE_INFO( ID_EDIT_REPEAT ),
			MFC_VALUE_INFO( ID_EDIT_REPLACE ),
			MFC_VALUE_INFO( ID_EDIT_SELECT_ALL ),
			MFC_VALUE_INFO( ID_EDIT_UNDO ),
			MFC_VALUE_INFO( ID_EDIT_REDO ),
			MFC_VALUE_INFO( ID_WINDOW_NEW ),
			MFC_VALUE_INFO( ID_WINDOW_ARRANGE ),
			MFC_VALUE_INFO( ID_WINDOW_CASCADE ),
			MFC_VALUE_INFO( ID_WINDOW_TILE_HORZ ),
			MFC_VALUE_INFO( ID_WINDOW_TILE_VERT ),
			MFC_VALUE_INFO( ID_WINDOW_SPLIT ),
			MFC_VALUE_INFO( ID_VIEW_TOOLBAR ),
			MFC_VALUE_INFO( ID_VIEW_STATUS_BAR ),
			MFC_VALUE_INFO( ID_VIEW_REBAR ),
			MFC_VALUE_INFO( ID_VIEW_AUTOARRANGE ),
			MFC_VALUE_INFO( ID_VIEW_SMALLICON ),
			MFC_VALUE_INFO( ID_VIEW_LARGEICON ),
			MFC_VALUE_INFO( ID_VIEW_LIST ),
			MFC_VALUE_INFO( ID_VIEW_DETAILS ),
			MFC_VALUE_INFO( ID_VIEW_LINEUP ),
			MFC_VALUE_INFO( ID_VIEW_BYNAME ),
			MFC_VALUE_INFO( ID_APP_ABOUT ),
			MFC_VALUE_INFO( ID_APP_EXIT ),
			MFC_VALUE_INFO( ID_HELP_INDEX ),
			MFC_VALUE_INFO( ID_HELP_FINDER ),
			MFC_VALUE_INFO( ID_HELP_USING ),
			MFC_VALUE_INFO( ID_CONTEXT_HELP ),
			MFC_VALUE_INFO( ID_HELP ),
			MFC_VALUE_INFO( ID_DEFAULT_HELP ),
			MFC_VALUE_INFO( ID_RECORD_FIRST ),
			MFC_VALUE_INFO( ID_RECORD_LAST ),
			MFC_VALUE_INFO( ID_RECORD_NEXT ),
			MFC_VALUE_INFO( ID_RECORD_PREV ),
			MFC_VALUE_INFO( ID_NEXT_PANE ),
			MFC_VALUE_INFO( ID_PREV_PANE ),
			MFC_VALUE_INFO( ID_FORMAT_FONT )
		};

		CValueInfo ole[] =
		{
			MFC_VALUE_INFO( ID_OLE_INSERT_NEW ),
			MFC_VALUE_INFO( ID_OLE_EDIT_LINKS ),
			MFC_VALUE_INFO( ID_OLE_EDIT_CONVERT ),
			MFC_VALUE_INFO( ID_OLE_EDIT_CHANGE_ICON ),
			MFC_VALUE_INFO( ID_OLE_EDIT_PROPERTIES ),
			MFC_VALUE_INFO( ID_OLE_VERB_FIRST ),
			MFC_VALUE_INFO( ID_OLE_VERB_LAST )
		};

		CValueInfo printPreview[] =
		{
			MFC_VALUE_INFO( AFX_ID_PREVIEW_CLOSE ),
			MFC_VALUE_INFO( AFX_ID_PREVIEW_NUMPAGE ),
			MFC_VALUE_INFO( AFX_ID_PREVIEW_NEXT ),
			MFC_VALUE_INFO( AFX_ID_PREVIEW_PREV ),
			MFC_VALUE_INFO( AFX_ID_PREVIEW_PRINT ),
			MFC_VALUE_INFO( AFX_ID_PREVIEW_ZOOMIN ),
			MFC_VALUE_INFO( AFX_ID_PREVIEW_ZOOMOUT )
		};

		CValueInfo resource[] =
		{
			// standard cursors
			MFC_VALUE_INFO( AFX_IDC_CONTEXTHELP ),
			MFC_VALUE_INFO( AFX_IDC_MAGNIFY ),
			MFC_VALUE_INFO( AFX_IDC_SMALLARROWS ),
			MFC_VALUE_INFO( AFX_IDC_HSPLITBAR ),
			MFC_VALUE_INFO( AFX_IDC_VSPLITBAR ),
			MFC_VALUE_INFO( AFX_IDC_NODROPCRSR ),
			MFC_VALUE_INFO( AFX_IDC_TRACKNWSE ),
			MFC_VALUE_INFO( AFX_IDC_TRACKNESW ),
			MFC_VALUE_INFO( AFX_IDC_TRACKNS ),
			MFC_VALUE_INFO( AFX_IDC_TRACKWE ),
			MFC_VALUE_INFO( AFX_IDC_TRACK4WAY ),
			MFC_VALUE_INFO( AFX_IDC_MOVE4WAY ),

			// wheel mouse cursors
			MFC_VALUE_INFO( AFX_IDC_MOUSE_PAN_NW ),
			MFC_VALUE_INFO( AFX_IDC_MOUSE_PAN_N ),
			MFC_VALUE_INFO( AFX_IDC_MOUSE_PAN_NE ),
			MFC_VALUE_INFO( AFX_IDC_MOUSE_PAN_W ),
			MFC_VALUE_INFO( AFX_IDC_MOUSE_PAN_HV ),
			MFC_VALUE_INFO( AFX_IDC_MOUSE_PAN_E ),
			MFC_VALUE_INFO( AFX_IDC_MOUSE_PAN_SW ),
			MFC_VALUE_INFO( AFX_IDC_MOUSE_PAN_S ),
			MFC_VALUE_INFO( AFX_IDC_MOUSE_PAN_SE ),
			MFC_VALUE_INFO( AFX_IDC_MOUSE_PAN_HORZ ),
			MFC_VALUE_INFO( AFX_IDC_MOUSE_PAN_VERT ),

			// wheel mouse bitmaps
			MFC_VALUE_INFO( AFX_IDC_MOUSE_ORG_HORZ ),
			MFC_VALUE_INFO( AFX_IDC_MOUSE_ORG_VERT ),
			MFC_VALUE_INFO( AFX_IDC_MOUSE_ORG_HV ),
			MFC_VALUE_INFO( AFX_IDC_MOUSE_MASK ),

			MFC_VALUE_INFO( AFX_IDB_MINIFRAME_MENU ),		// mini frame window bitmap
			MFC_VALUE_INFO( AFX_IDB_CHECKLISTBOX_95 ),		// CheckListBox checks bitmap
			MFC_VALUE_INFO( AFX_IDR_PREVIEW_ACCEL ),		// AFX standard accelerator resources

			// AFX standard ICON IDs
			MFC_VALUE_INFO( AFX_IDI_STD_MDIFRAME ),
			MFC_VALUE_INFO( AFX_IDI_STD_FRAME )
		};

		CValueInfo message[] =
		{
			MFC_VALUE_INFO( AFX_IDS_SCSIZE ),
			MFC_VALUE_INFO( AFX_IDS_SCMOVE ),
			MFC_VALUE_INFO( AFX_IDS_SCMINIMIZE ),
			MFC_VALUE_INFO( AFX_IDS_SCMAXIMIZE ),
			MFC_VALUE_INFO( AFX_IDS_SCNEXTWINDOW ),
			MFC_VALUE_INFO( AFX_IDS_SCPREVWINDOW ),
			MFC_VALUE_INFO( AFX_IDS_SCCLOSE ),
			MFC_VALUE_INFO( AFX_IDS_SCRESTORE ),
			MFC_VALUE_INFO( AFX_IDS_SCTASKLIST ),
			MFC_VALUE_INFO( AFX_IDS_MDICHILD ),
			MFC_VALUE_INFO( AFX_IDS_DESKACCESSORY ),
			MFC_VALUE_INFO( AFX_IDS_OPENFILE ),
			MFC_VALUE_INFO( AFX_IDS_SAVEFILE ),
			MFC_VALUE_INFO( AFX_IDS_ALLFILTER ),
			MFC_VALUE_INFO( AFX_IDS_UNTITLED ),
			MFC_VALUE_INFO( AFX_IDS_SAVEFILECOPY ),
			MFC_VALUE_INFO( AFX_IDS_PREVIEW_CLOSE ),
			MFC_VALUE_INFO( AFX_IDS_UNNAMED_FILE ),
			MFC_VALUE_INFO( AFX_IDS_HIDE ),
			MFC_VALUE_INFO( AFX_IDP_NO_ERROR_AVAILABLE ),
			MFC_VALUE_INFO( AFX_IDS_NOT_SUPPORTED_EXCEPTION ),
			MFC_VALUE_INFO( AFX_IDS_RESOURCE_EXCEPTION ),
			MFC_VALUE_INFO( AFX_IDS_MEMORY_EXCEPTION ),
			MFC_VALUE_INFO( AFX_IDS_USER_EXCEPTION ),
			MFC_VALUE_INFO( AFX_IDS_INVALID_ARG_EXCEPTION ),
			MFC_VALUE_INFO( AFX_IDS_PRINTONPORT ),
			MFC_VALUE_INFO( AFX_IDS_ONEPAGE ),
			MFC_VALUE_INFO( AFX_IDS_TWOPAGE ),
			MFC_VALUE_INFO( AFX_IDS_PRINTPAGENUM ),
			MFC_VALUE_INFO( AFX_IDS_PREVIEWPAGEDESC ),
			MFC_VALUE_INFO( AFX_IDS_PRINTDEFAULTEXT ),
			MFC_VALUE_INFO( AFX_IDS_PRINTDEFAULT ),
			MFC_VALUE_INFO( AFX_IDS_PRINTFILTER ),
			MFC_VALUE_INFO( AFX_IDS_PRINTCAPTION ),
			MFC_VALUE_INFO( AFX_IDS_PRINTTOFILE ),
			MFC_VALUE_INFO( AFX_IDS_OBJECT_MENUITEM ),
			MFC_VALUE_INFO( AFX_IDS_EDIT_VERB ),
			MFC_VALUE_INFO( AFX_IDS_ACTIVATE_VERB ),
			MFC_VALUE_INFO( AFX_IDS_CHANGE_LINK ),
			MFC_VALUE_INFO( AFX_IDS_AUTO ),
			MFC_VALUE_INFO( AFX_IDS_MANUAL ),
			MFC_VALUE_INFO( AFX_IDS_FROZEN ),
			MFC_VALUE_INFO( AFX_IDS_ALL_FILES ),
			MFC_VALUE_INFO( AFX_IDS_SAVE_MENU ),
			MFC_VALUE_INFO( AFX_IDS_UPDATE_MENU ),
			MFC_VALUE_INFO( AFX_IDS_SAVE_AS_MENU ),
			MFC_VALUE_INFO( AFX_IDS_SAVE_COPY_AS_MENU ),
			MFC_VALUE_INFO( AFX_IDS_EXIT_MENU ),
			MFC_VALUE_INFO( AFX_IDS_UPDATING_ITEMS ),
			MFC_VALUE_INFO( AFX_IDS_METAFILE_FORMAT ),
			MFC_VALUE_INFO( AFX_IDS_DIB_FORMAT ),
			MFC_VALUE_INFO( AFX_IDS_BITMAP_FORMAT ),
			MFC_VALUE_INFO( AFX_IDS_LINKSOURCE_FORMAT ),
			MFC_VALUE_INFO( AFX_IDS_EMBED_FORMAT ),
			MFC_VALUE_INFO( AFX_IDS_PASTELINKEDTYPE ),
			MFC_VALUE_INFO( AFX_IDS_UNKNOWNTYPE ),
			MFC_VALUE_INFO( AFX_IDS_RTF_FORMAT ),
			MFC_VALUE_INFO( AFX_IDS_TEXT_FORMAT ),
			MFC_VALUE_INFO( AFX_IDS_INVALID_CURRENCY ),
			MFC_VALUE_INFO( AFX_IDS_INVALID_DATETIME ),
			MFC_VALUE_INFO( AFX_IDS_INVALID_DATETIMESPAN ),
			MFC_VALUE_INFO( AFX_IDP_INVALID_FILENAME ),
			MFC_VALUE_INFO( AFX_IDP_FAILED_TO_OPEN_DOC ),
			MFC_VALUE_INFO( AFX_IDP_FAILED_TO_SAVE_DOC ),
			MFC_VALUE_INFO( AFX_IDP_ASK_TO_SAVE ),
			MFC_VALUE_INFO( AFX_IDP_FAILED_TO_CREATE_DOC ),
			MFC_VALUE_INFO( AFX_IDP_FILE_TOO_LARGE ),
			MFC_VALUE_INFO( AFX_IDP_FAILED_TO_START_PRINT ),
			MFC_VALUE_INFO( AFX_IDP_FAILED_TO_LAUNCH_HELP ),
			MFC_VALUE_INFO( AFX_IDP_INTERNAL_FAILURE ),
			MFC_VALUE_INFO( AFX_IDP_COMMAND_FAILURE ),
			MFC_VALUE_INFO( AFX_IDP_FAILED_MEMORY_ALLOC ),
			MFC_VALUE_INFO( AFX_IDP_UNREG_DONE ),
			MFC_VALUE_INFO( AFX_IDP_UNREG_FAILURE ),
			MFC_VALUE_INFO( AFX_IDP_DLL_LOAD_FAILED ),
			MFC_VALUE_INFO( AFX_IDP_DLL_BAD_VERSION ),
			MFC_VALUE_INFO( AFX_IDP_PARSE_INT ),
			MFC_VALUE_INFO( AFX_IDP_PARSE_REAL ),
			MFC_VALUE_INFO( AFX_IDP_PARSE_INT_RANGE ),
			MFC_VALUE_INFO( AFX_IDP_PARSE_REAL_RANGE ),
			MFC_VALUE_INFO( AFX_IDP_PARSE_STRING_SIZE ),
			MFC_VALUE_INFO( AFX_IDP_PARSE_RADIO_BUTTON ),
			MFC_VALUE_INFO( AFX_IDP_PARSE_BYTE ),
			MFC_VALUE_INFO( AFX_IDP_PARSE_UINT ),
			MFC_VALUE_INFO( AFX_IDP_PARSE_DATETIME ),
			MFC_VALUE_INFO( AFX_IDP_PARSE_CURRENCY ),
			MFC_VALUE_INFO( AFX_IDP_PARSE_GUID ),
			MFC_VALUE_INFO( AFX_IDP_PARSE_TIME ),
			MFC_VALUE_INFO( AFX_IDP_PARSE_DATE ),
			MFC_VALUE_INFO( AFX_IDP_FAILED_INVALID_FORMAT ),
			MFC_VALUE_INFO( AFX_IDP_FAILED_INVALID_PATH ),
			MFC_VALUE_INFO( AFX_IDP_FAILED_DISK_FULL ),
			MFC_VALUE_INFO( AFX_IDP_FAILED_ACCESS_READ ),
			MFC_VALUE_INFO( AFX_IDP_FAILED_ACCESS_WRITE ),
			MFC_VALUE_INFO( AFX_IDP_FAILED_IO_ERROR_READ ),
			MFC_VALUE_INFO( AFX_IDP_FAILED_IO_ERROR_WRITE ),
			MFC_VALUE_INFO( AFX_IDP_SCRIPT_ERROR ),
			MFC_VALUE_INFO( AFX_IDP_SCRIPT_DISPATCH_EXCEPTION ),
			MFC_VALUE_INFO( AFX_IDP_STATIC_OBJECT ),
			MFC_VALUE_INFO( AFX_IDP_FAILED_TO_CONNECT ),
			MFC_VALUE_INFO( AFX_IDP_SERVER_BUSY ),
			MFC_VALUE_INFO( AFX_IDP_BAD_VERB ),
			MFC_VALUE_INFO( AFX_IDS_NOT_DOCOBJECT ),
			MFC_VALUE_INFO( AFX_IDP_FAILED_TO_NOTIFY ),
			MFC_VALUE_INFO( AFX_IDP_FAILED_TO_LAUNCH ),
			MFC_VALUE_INFO( AFX_IDP_ASK_TO_UPDATE ),
			MFC_VALUE_INFO( AFX_IDP_FAILED_TO_UPDATE ),
			MFC_VALUE_INFO( AFX_IDP_FAILED_TO_REGISTER ),
			MFC_VALUE_INFO( AFX_IDP_FAILED_TO_AUTO_REGISTER ),
			MFC_VALUE_INFO( AFX_IDP_FAILED_TO_CONVERT ),
			MFC_VALUE_INFO( AFX_IDP_GET_NOT_SUPPORTED ),
			MFC_VALUE_INFO( AFX_IDP_SET_NOT_SUPPORTED ),
			MFC_VALUE_INFO( AFX_IDP_ASK_TO_DISCARD ),
			MFC_VALUE_INFO( AFX_IDP_FAILED_TO_CREATE ),
			MFC_VALUE_INFO( AFX_IDP_FAILED_MAPI_LOAD ),
			MFC_VALUE_INFO( AFX_IDP_INVALID_MAPI_DLL ),
			MFC_VALUE_INFO( AFX_IDP_FAILED_MAPI_SEND ),
			MFC_VALUE_INFO( AFX_IDP_FILE_NONE ),
			MFC_VALUE_INFO( AFX_IDP_FILE_GENERIC ),
			MFC_VALUE_INFO( AFX_IDP_FILE_NOT_FOUND ),
			MFC_VALUE_INFO( AFX_IDP_FILE_BAD_PATH ),
			MFC_VALUE_INFO( AFX_IDP_FILE_TOO_MANY_OPEN ),
			MFC_VALUE_INFO( AFX_IDP_FILE_ACCESS_DENIED ),
			MFC_VALUE_INFO( AFX_IDP_FILE_INVALID_FILE ),
			MFC_VALUE_INFO( AFX_IDP_FILE_REMOVE_CURRENT ),
			MFC_VALUE_INFO( AFX_IDP_FILE_DIR_FULL ),
			MFC_VALUE_INFO( AFX_IDP_FILE_BAD_SEEK ),
			MFC_VALUE_INFO( AFX_IDP_FILE_HARD_IO ),
			MFC_VALUE_INFO( AFX_IDP_FILE_SHARING ),
			MFC_VALUE_INFO( AFX_IDP_FILE_LOCKING ),
			MFC_VALUE_INFO( AFX_IDP_FILE_DISKFULL ),
			MFC_VALUE_INFO( AFX_IDP_FILE_EOF ),
			MFC_VALUE_INFO( AFX_IDP_ARCH_NONE ),
			MFC_VALUE_INFO( AFX_IDP_ARCH_GENERIC ),
			MFC_VALUE_INFO( AFX_IDP_ARCH_READONLY ),
			MFC_VALUE_INFO( AFX_IDP_ARCH_ENDOFFILE ),
			MFC_VALUE_INFO( AFX_IDP_ARCH_WRITEONLY ),
			MFC_VALUE_INFO( AFX_IDP_ARCH_BADINDEX ),
			MFC_VALUE_INFO( AFX_IDP_ARCH_BADCLASS ),
			MFC_VALUE_INFO( AFX_IDP_ARCH_BADSCHEMA ),
			MFC_VALUE_INFO( AFX_IDS_OCC_SCALEUNITS_PIXELS ),
			MFC_VALUE_INFO( AFX_IDS_STATUS_FONT ),
			MFC_VALUE_INFO( AFX_IDS_TOOLTIP_FONT ),
			MFC_VALUE_INFO( AFX_IDS_UNICODE_FONT ),
			MFC_VALUE_INFO( AFX_IDS_MINI_FONT ),
			MFC_VALUE_INFO( AFX_IDP_SQL_FIRST ),
			MFC_VALUE_INFO( AFX_IDP_SQL_CONNECT_FAIL ),
			MFC_VALUE_INFO( AFX_IDP_SQL_RECORDSET_FORWARD_ONLY ),
			MFC_VALUE_INFO( AFX_IDP_SQL_EMPTY_COLUMN_LIST ),
			MFC_VALUE_INFO( AFX_IDP_SQL_FIELD_SCHEMA_MISMATCH ),
			MFC_VALUE_INFO( AFX_IDP_SQL_ILLEGAL_MODE ),
			MFC_VALUE_INFO( AFX_IDP_SQL_MULTIPLE_ROWS_AFFECTED ),
			MFC_VALUE_INFO( AFX_IDP_SQL_NO_CURRENT_RECORD ),
			MFC_VALUE_INFO( AFX_IDP_SQL_NO_ROWS_AFFECTED ),
			MFC_VALUE_INFO( AFX_IDP_SQL_RECORDSET_READONLY ),
			MFC_VALUE_INFO( AFX_IDP_SQL_SQL_NO_TOTAL ),
			MFC_VALUE_INFO( AFX_IDP_SQL_ODBC_LOAD_FAILED ),
			MFC_VALUE_INFO( AFX_IDP_SQL_DYNASET_NOT_SUPPORTED ),
			MFC_VALUE_INFO( AFX_IDP_SQL_SNAPSHOT_NOT_SUPPORTED ),
			MFC_VALUE_INFO( AFX_IDP_SQL_API_CONFORMANCE ),
			MFC_VALUE_INFO( AFX_IDP_SQL_SQL_CONFORMANCE ),
			MFC_VALUE_INFO( AFX_IDP_SQL_NO_DATA_FOUND ),
			MFC_VALUE_INFO( AFX_IDP_SQL_ROW_UPDATE_NOT_SUPPORTED ),
			MFC_VALUE_INFO( AFX_IDP_SQL_ODBC_V2_REQUIRED ),
			MFC_VALUE_INFO( AFX_IDP_SQL_NO_POSITIONED_UPDATES ),
			MFC_VALUE_INFO( AFX_IDP_SQL_LOCK_MODE_NOT_SUPPORTED ),
			MFC_VALUE_INFO( AFX_IDP_SQL_DATA_TRUNCATED ),
			MFC_VALUE_INFO( AFX_IDP_SQL_ROW_FETCH ),
			MFC_VALUE_INFO( AFX_IDP_SQL_INCORRECT_ODBC ),
			MFC_VALUE_INFO( AFX_IDP_SQL_UPDATE_DELETE_FAILED ),
			MFC_VALUE_INFO( AFX_IDP_SQL_DYNAMIC_CURSOR_NOT_SUPPORTED ),
			MFC_VALUE_INFO( AFX_IDP_SQL_FIELD_NOT_FOUND ),
			MFC_VALUE_INFO( AFX_IDP_SQL_BOOKMARKS_NOT_SUPPORTED ),
			MFC_VALUE_INFO( AFX_IDP_SQL_BOOKMARKS_NOT_ENABLED ),
			MFC_VALUE_INFO( AFX_IDS_DELETED ),
			MFC_VALUE_INFO( AFX_IDP_DAO_FIRST ),
			MFC_VALUE_INFO( AFX_IDP_DAO_ENGINE_INITIALIZATION ),
			MFC_VALUE_INFO( AFX_IDP_DAO_DFX_BIND ),
			MFC_VALUE_INFO( AFX_IDP_DAO_OBJECT_NOT_OPEN ),
			MFC_VALUE_INFO( AFX_IDP_DAO_ROWTOOSHORT ),
			MFC_VALUE_INFO( AFX_IDP_DAO_BADBINDINFO ),
			MFC_VALUE_INFO( AFX_IDP_DAO_COLUMNUNAVAILABLE ),
			MFC_VALUE_INFO( AFX_IDS_HTTP_TITLE ),
			MFC_VALUE_INFO( AFX_IDS_HTTP_NO_TEXT ),
			MFC_VALUE_INFO( AFX_IDS_HTTP_BAD_REQUEST ),
			MFC_VALUE_INFO( AFX_IDS_HTTP_AUTH_REQUIRED ),
			MFC_VALUE_INFO( AFX_IDS_HTTP_FORBIDDEN ),
			MFC_VALUE_INFO( AFX_IDS_HTTP_NOT_FOUND ),
			MFC_VALUE_INFO( AFX_IDS_HTTP_SERVER_ERROR ),
			MFC_VALUE_INFO( AFX_IDS_HTTP_NOT_IMPLEMENTED ),
			MFC_VALUE_INFO( AFX_IDS_CHECKLISTBOX_UNCHECK ),
			MFC_VALUE_INFO( AFX_IDS_CHECKLISTBOX_CHECK ),
			MFC_VALUE_INFO( AFX_IDS_CHECKLISTBOX_MIXED )
		};

		CValueInfo oleMessage[] =
		{
			MFC_VALUE_INFO( AFX_IDS_PROPPAGE_UNKNOWN ),
			MFC_VALUE_INFO( AFX_IDS_COLOR_DESKTOP ),
			MFC_VALUE_INFO( AFX_IDS_COLOR_APPWORKSPACE ),
			MFC_VALUE_INFO( AFX_IDS_COLOR_WNDBACKGND ),
			MFC_VALUE_INFO( AFX_IDS_COLOR_WNDTEXT ),
			MFC_VALUE_INFO( AFX_IDS_COLOR_MENUBAR ),
			MFC_VALUE_INFO( AFX_IDS_COLOR_MENUTEXT ),
			MFC_VALUE_INFO( AFX_IDS_COLOR_ACTIVEBAR ),
			MFC_VALUE_INFO( AFX_IDS_COLOR_INACTIVEBAR ),
			MFC_VALUE_INFO( AFX_IDS_COLOR_ACTIVETEXT ),
			MFC_VALUE_INFO( AFX_IDS_COLOR_INACTIVETEXT ),
			MFC_VALUE_INFO( AFX_IDS_COLOR_ACTIVEBORDER ),
			MFC_VALUE_INFO( AFX_IDS_COLOR_INACTIVEBORDER ),
			MFC_VALUE_INFO( AFX_IDS_COLOR_WNDFRAME ),
			MFC_VALUE_INFO( AFX_IDS_COLOR_SCROLLBARS ),
			MFC_VALUE_INFO( AFX_IDS_COLOR_BTNFACE ),
			MFC_VALUE_INFO( AFX_IDS_COLOR_BTNSHADOW ),
			MFC_VALUE_INFO( AFX_IDS_COLOR_BTNTEXT ),
			MFC_VALUE_INFO( AFX_IDS_COLOR_BTNHIGHLIGHT ),
			MFC_VALUE_INFO( AFX_IDS_COLOR_DISABLEDTEXT ),
			MFC_VALUE_INFO( AFX_IDS_COLOR_HIGHLIGHT ),
			MFC_VALUE_INFO( AFX_IDS_COLOR_HIGHLIGHTTEXT ),
			MFC_VALUE_INFO( AFX_IDS_REGULAR ),
			MFC_VALUE_INFO( AFX_IDS_BOLD ),
			MFC_VALUE_INFO( AFX_IDS_ITALIC ),
			MFC_VALUE_INFO( AFX_IDS_BOLDITALIC ),
			MFC_VALUE_INFO( AFX_IDS_SAMPLETEXT ),
			MFC_VALUE_INFO( AFX_IDS_DISPLAYSTRING_FONT ),
			MFC_VALUE_INFO( AFX_IDS_DISPLAYSTRING_COLOR ),
			MFC_VALUE_INFO( AFX_IDS_DISPLAYSTRING_PICTURE ),
			MFC_VALUE_INFO( AFX_IDS_PICTUREFILTER ),
			MFC_VALUE_INFO( AFX_IDS_PICTYPE_UNKNOWN ),
			MFC_VALUE_INFO( AFX_IDS_PICTYPE_NONE ),
			MFC_VALUE_INFO( AFX_IDS_PICTYPE_BITMAP ),
			MFC_VALUE_INFO( AFX_IDS_PICTYPE_METAFILE ),
			MFC_VALUE_INFO( AFX_IDS_PICTYPE_ICON ),
			MFC_VALUE_INFO( AFX_IDS_COLOR_PPG ),
			MFC_VALUE_INFO( AFX_IDS_COLOR_PPG_CAPTION ),
			MFC_VALUE_INFO( AFX_IDS_FONT_PPG ),
			MFC_VALUE_INFO( AFX_IDS_FONT_PPG_CAPTION ),
			MFC_VALUE_INFO( AFX_IDS_PICTURE_PPG ),
			MFC_VALUE_INFO( AFX_IDS_PICTURE_PPG_CAPTION ),
			MFC_VALUE_INFO( AFX_IDS_PICTUREBROWSETITLE ),
			MFC_VALUE_INFO( AFX_IDS_BORDERSTYLE_0 ),
			MFC_VALUE_INFO( AFX_IDS_BORDERSTYLE_1 ),
			MFC_VALUE_INFO( AFX_IDS_VERB_EDIT ),
			MFC_VALUE_INFO( AFX_IDS_VERB_PROPERTIES ),
			MFC_VALUE_INFO( AFX_IDP_PICTURECANTOPEN ),
			MFC_VALUE_INFO( AFX_IDP_PICTURECANTLOAD ),
			MFC_VALUE_INFO( AFX_IDP_PICTURETOOLARGE ),
			MFC_VALUE_INFO( AFX_IDP_PICTUREREADFAILED ),
			MFC_VALUE_INFO( AFX_IDP_E_ILLEGALFUNCTIONCALL ),
			MFC_VALUE_INFO( AFX_IDP_E_OVERFLOW ),
			MFC_VALUE_INFO( AFX_IDP_E_OUTOFMEMORY ),
			MFC_VALUE_INFO( AFX_IDP_E_DIVISIONBYZERO ),
			MFC_VALUE_INFO( AFX_IDP_E_OUTOFSTRINGSPACE ),
			MFC_VALUE_INFO( AFX_IDP_E_OUTOFSTACKSPACE ),
			MFC_VALUE_INFO( AFX_IDP_E_BADFILENAMEORNUMBER ),
			MFC_VALUE_INFO( AFX_IDP_E_FILENOTFOUND ),
			MFC_VALUE_INFO( AFX_IDP_E_BADFILEMODE ),
			MFC_VALUE_INFO( AFX_IDP_E_FILEALREADYOPEN ),
			MFC_VALUE_INFO( AFX_IDP_E_DEVICEIOERROR ),
			MFC_VALUE_INFO( AFX_IDP_E_FILEALREADYEXISTS ),
			MFC_VALUE_INFO( AFX_IDP_E_BADRECORDLENGTH ),
			MFC_VALUE_INFO( AFX_IDP_E_DISKFULL ),
			MFC_VALUE_INFO( AFX_IDP_E_BADRECORDNUMBER ),
			MFC_VALUE_INFO( AFX_IDP_E_BADFILENAME ),
			MFC_VALUE_INFO( AFX_IDP_E_TOOMANYFILES ),
			MFC_VALUE_INFO( AFX_IDP_E_DEVICEUNAVAILABLE ),
			MFC_VALUE_INFO( AFX_IDP_E_PERMISSIONDENIED ),
			MFC_VALUE_INFO( AFX_IDP_E_DISKNOTREADY ),
			MFC_VALUE_INFO( AFX_IDP_E_PATHFILEACCESSERROR ),
			MFC_VALUE_INFO( AFX_IDP_E_PATHNOTFOUND ),
			MFC_VALUE_INFO( AFX_IDP_E_INVALIDPATTERNSTRING ),
			MFC_VALUE_INFO( AFX_IDP_E_INVALIDUSEOFNULL ),
			MFC_VALUE_INFO( AFX_IDP_E_INVALIDFILEFORMAT ),
			MFC_VALUE_INFO( AFX_IDP_E_INVALIDPROPERTYVALUE ),
			MFC_VALUE_INFO( AFX_IDP_E_INVALIDPROPERTYARRAYINDEX ),
			MFC_VALUE_INFO( AFX_IDP_E_SETNOTSUPPORTEDATRUNTIME ),
			MFC_VALUE_INFO( AFX_IDP_E_SETNOTSUPPORTED ),
			MFC_VALUE_INFO( AFX_IDP_E_NEEDPROPERTYARRAYINDEX ),
			MFC_VALUE_INFO( AFX_IDP_E_SETNOTPERMITTED ),
			MFC_VALUE_INFO( AFX_IDP_E_GETNOTSUPPORTEDATRUNTIME ),
			MFC_VALUE_INFO( AFX_IDP_E_GETNOTSUPPORTED ),
			MFC_VALUE_INFO( AFX_IDP_E_PROPERTYNOTFOUND ),
			MFC_VALUE_INFO( AFX_IDP_E_INVALIDCLIPBOARDFORMAT ),
			MFC_VALUE_INFO( AFX_IDP_E_INVALIDPICTURE ),
			MFC_VALUE_INFO( AFX_IDP_E_PRINTERERROR ),
			MFC_VALUE_INFO( AFX_IDP_E_CANTSAVEFILETOTEMP ),
			MFC_VALUE_INFO( AFX_IDP_E_SEARCHTEXTNOTFOUND ),
			MFC_VALUE_INFO( AFX_IDP_E_REPLACEMENTSTOOLONG )
		};

		CValueInfo control[] =
		{
			// parts of dialogs
			VALUE_INFO( AFX_IDC_LISTBOX ),
			VALUE_INFO( AFX_IDC_CHANGE ),
			VALUE_INFO( AFX_IDC_BROWSER ),

			// for print dialog
			VALUE_INFO( AFX_IDC_PRINT_DOCNAME ),
			VALUE_INFO( AFX_IDC_PRINT_PRINTERNAME ),
			VALUE_INFO( AFX_IDC_PRINT_PORTNAME ),
			VALUE_INFO( AFX_IDC_PRINT_PAGENUM ),

			// standard components
			VALUE_INFO( AFX_IDD_FILEOPEN ),
			VALUE_INFO( AFX_IDD_FILESAVE ),
			VALUE_INFO( AFX_IDD_FONT ),
			VALUE_INFO( AFX_IDD_COLOR ),
			VALUE_INFO( AFX_IDD_PRINT ),
			VALUE_INFO( AFX_IDD_PRINTSETUP ),
			VALUE_INFO( AFX_IDD_FIND ),
			VALUE_INFO( AFX_IDD_REPLACE ),

			// standard dialogs
			VALUE_INFO( AFX_IDD_NEWTYPEDLG ),
			VALUE_INFO( AFX_IDD_PRINTDLG ),
			VALUE_INFO( AFX_IDD_PREVIEW_TOOLBAR ),

			// OLE2UI dialogs
			VALUE_INFO( AFX_IDD_INSERTOBJECT ),
			VALUE_INFO( AFX_IDD_CHANGEICON ),
			VALUE_INFO( AFX_IDD_CONVERT ),
			VALUE_INFO( AFX_IDD_PASTESPECIAL ),
			VALUE_INFO( AFX_IDD_EDITLINKS ),
			VALUE_INFO( AFX_IDD_FILEBROWSE ),
			VALUE_INFO( AFX_IDD_BUSY ),
			VALUE_INFO( AFX_IDD_OBJECTPROPERTIES ),
			VALUE_INFO( AFX_IDD_CHANGESOURCE ),

			// WinForms
			VALUE_INFO( AFX_IDD_EMPTYDIALOG )
		};

		// AFX OLE control
		CValueInfo oleControl[] =
		{
			// Font property page
			VALUE_INFO( AFX_IDC_FONTPROP ),
			VALUE_INFO( AFX_IDC_FONTNAMES ),
			VALUE_INFO( AFX_IDC_FONTSTYLES ),
			VALUE_INFO( AFX_IDC_FONTSIZES ),
			VALUE_INFO( AFX_IDC_STRIKEOUT ),
			VALUE_INFO( AFX_IDC_UNDERLINE ),
			VALUE_INFO( AFX_IDC_SAMPLEBOX ),

			// color property page
			VALUE_INFO( AFX_IDC_COLOR_BLACK ),
			VALUE_INFO( AFX_IDC_COLOR_WHITE ),
			VALUE_INFO( AFX_IDC_COLOR_RED ),
			VALUE_INFO( AFX_IDC_COLOR_GREEN ),
			VALUE_INFO( AFX_IDC_COLOR_BLUE ),
			VALUE_INFO( AFX_IDC_COLOR_YELLOW ),
			VALUE_INFO( AFX_IDC_COLOR_MAGENTA ),
			VALUE_INFO( AFX_IDC_COLOR_CYAN ),
			VALUE_INFO( AFX_IDC_COLOR_GRAY ),
			VALUE_INFO( AFX_IDC_COLOR_LIGHTGRAY ),
			VALUE_INFO( AFX_IDC_COLOR_DARKRED ),
			VALUE_INFO( AFX_IDC_COLOR_DARKGREEN ),
			VALUE_INFO( AFX_IDC_COLOR_DARKBLUE ),
			VALUE_INFO( AFX_IDC_COLOR_LIGHTBROWN ),
			VALUE_INFO( AFX_IDC_COLOR_DARKMAGENTA ),
			VALUE_INFO( AFX_IDC_COLOR_DARKCYAN ),
			VALUE_INFO( AFX_IDC_COLORPROP ),
			VALUE_INFO( AFX_IDC_SYSTEMCOLORS ),

			// picture porperty page
			VALUE_INFO( AFX_IDC_PROPNAME ),
			VALUE_INFO( AFX_IDC_PICTURE ),
			VALUE_INFO( AFX_IDC_BROWSE ),
			VALUE_INFO( AFX_IDC_CLEAR ),

			// OLE control standard components
			VALUE_INFO( AFX_IDD_PROPPAGE_COLOR ),
			VALUE_INFO( AFX_IDD_PROPPAGE_FONT ),
			VALUE_INFO( AFX_IDD_PROPPAGE_PICTURE )
		#if _MSC_VER <= 1500	// MSVC++ 9.0 (Visual Studio 2008)
			, VALUE_INFO( AFX_IDB_TRUETYPE )
		#endif
		};
	}
}


// CValueInfo implementation

std::tstring CValueInfo::Format( void ) const
{
	return str::Format( _T("%s (%s)"), m_pTag, m_pStore->GetName().c_str() );
}


// CValueStore implementation

CValueStore::CValueStore( const TCHAR* pName, CValueInfo valueInfos[], unsigned int count )
	: m_name( pName )
{
	for ( unsigned int i = 0; i != count; ++i )
	{
		CValueInfo* pValueInfo = &valueInfos[ i ];
		pValueInfo->m_pStore = this;
		m_valueInfos.push_back( pValueInfo );
	}
}


// CIdentRepository implementation

CIdentRepository::CIdentRepository( void )
{
	m_stores.push_back( new CValueStore( _T("Windows"), ident::windows, COUNT_OF( ident::windows ) ) );
	m_stores.push_back( new CValueStore( _T("MFC frame & view"), ident::mfc::frameAndView, COUNT_OF( ident::mfc::frameAndView ) ) );
	m_stores.push_back( new CValueStore( _T("MFC miscellaneous"), ident::mfc::misc, COUNT_OF( ident::mfc::misc ) ) );
	m_stores.push_back( new CValueStore( _T("MFC Control Bar"), ident::mfc::controlBar, COUNT_OF( ident::mfc::controlBar ) ) );
	m_stores.push_back( new CValueStore( _T("MFC Status Bar"), ident::mfc::statusBar, COUNT_OF( ident::mfc::statusBar ) ) );
	m_stores.push_back( new CValueStore( _T("MFC commands"), ident::mfc::menu, COUNT_OF( ident::mfc::menu ) ) );
	m_stores.push_back( new CValueStore( _T("MFC OLE commands"), ident::mfc::ole, COUNT_OF( ident::mfc::ole ) ) );
	m_stores.push_back( new CValueStore( _T("MFC print preview"), ident::mfc::printPreview, COUNT_OF( ident::mfc::printPreview ) ) );
	m_stores.push_back( new CValueStore( _T("MFC resources"), ident::mfc::resource, COUNT_OF( ident::mfc::resource ) ) );
	m_stores.push_back( new CValueStore( _T("MFC message strings"), ident::mfc::message, COUNT_OF( ident::mfc::message ) ) );
	m_stores.push_back( new CValueStore( _T("MFC OLE strings"), ident::mfc::oleMessage, COUNT_OF( ident::mfc::oleMessage ) ) );
	m_stores.push_back( new CValueStore( _T("MFC controls"), ident::mfc::control, COUNT_OF( ident::mfc::control ) ) );
	m_stores.push_back( new CValueStore( _T("MFC OLE control"), ident::mfc::oleControl, COUNT_OF( ident::mfc::oleControl ) ) );
}

CIdentRepository::~CIdentRepository()
{
	utl::ClearOwningContainer( m_stores );
}

const CIdentRepository& CIdentRepository::Instance( void )
{
	static const CIdentRepository identRepository;
	return identRepository;
}

const CValueInfo* CIdentRepository::FindValue( int value ) const
{
	if ( m_idToValuesMap.empty() )
		for ( std::vector<CValueStore*>::const_iterator itStore = m_stores.begin(); itStore != m_stores.end(); ++itStore )
			for ( std::vector<const CValueInfo*>::const_iterator itValueInfo = ( *itStore )->GetValues().begin();
				  itValueInfo != ( *itStore )->GetValues().end(); ++itValueInfo )
				m_idToValuesMap[ ( *itValueInfo )->m_value ] = *itValueInfo;

	std::unordered_map<int, const CValueInfo*>::const_iterator itFound = m_idToValuesMap.find( value );
	return itFound != m_idToValuesMap.end() ? itFound->second : nullptr;
}

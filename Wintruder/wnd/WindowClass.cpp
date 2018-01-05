
#include "stdafx.h"
#include "WindowClass.h"
#include "utl/StringUtilities.h"
#include "utl/Utilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace wc
{
	std::tstring GetClassName( HWND hWnd )
	{
		ASSERT( hWnd == NULL || ::IsWindow( hWnd ) );		// null or valid, not stale

		std::tstring className;
		if ( hWnd != NULL )
		{
			TCHAR text[ 256 ] = _T("");
			::GetClassName( hWnd, text, COUNT_OF( text ) );
			className = text;
		}
		return className;
	}

	std::tstring GetDisplayClassName( HWND hWnd )
	{
		std::tstring className = GetClassName( hWnd );

		if ( !className.empty() )
			if ( _T('#') == className[ 0 ] )
			{
				switch ( ::GlobalFindAtom( className.c_str() ) )
				{	// for known ordinal class names append class tag
					case WC_DIALOG:	className += _T(":Dialog"); break;
					case 32768:		className += _T(":PopupMenu"); break;
					case 32769:		className += _T(":Desktop"); break;
				}
			}
			else if ( IsButton( className.c_str() ) && EqFlag( ui::GetStyle( hWnd ), BS_GROUPBOX ) )
				className += _T(":GroupBox");

		return className;
	}

	std::tstring GetDisplayClassName( const TCHAR* pClassName )
	{
		ASSERT_PTR( pClassName );
		std::tstring decoratedClassName = pClassName;
		if ( _T('#') == pClassName[ 0 ] )
			switch ( ::GlobalFindAtom( pClassName ) )
			{	// for known ordinal class names append class tag
				case WC_DIALOG:
					return decoratedClassName + _T(":Dialog");
				case 32768:
					return decoratedClassName + _T(":PopupMenu");
				case 32769:
					return decoratedClassName + _T(":Desktop");
			}

		return decoratedClassName;
	}

	std::tstring FormatClassName( const TCHAR* pClassName )
	{
		return str::Format( _T("[%s]"), wc::GetDisplayClassName( pClassName ).c_str() );
	}

	std::tstring FormatClassName( HWND hWnd )
	{
		return str::Format( _T("[%s]"), GetDisplayClassName( hWnd ).c_str() );
	}


	bool IsDialogBox( const TCHAR* pClassName )
	{
		return str::Equals< str::IgnoreCase >( pClassName, _T("#32770") );
	}

	bool IsPopupMenu( const TCHAR* pClassName )
	{
		return str::Equals< str::IgnoreCase >( pClassName, _T("#32768") );
	}

	bool IsButton( const TCHAR* pClassName )
	{
		return str::Equals< str::IgnoreCase >( pClassName, _T("Button") );
	}

	bool IsEdit( const TCHAR* pClassName )
	{
		return str::Equals< str::IgnoreCase >( pClassName, _T("Edit") );
	}

	bool IsStatic( const TCHAR* pClassName )
	{
		return str::Equals< str::IgnoreCase >( pClassName, _T("Static") );
	}

	bool IsScrollBar( const TCHAR* pClassName )
	{
		return str::Equals< str::IgnoreCase >( pClassName, _T("ScrollBar") );
	}

	bool IsComboBox( const TCHAR* pClassName )
	{
		return str::Equals< str::IgnoreCase >( pClassName, _T("ComboBox") );
	}

	bool IsComboBoxEx( const TCHAR* pClassName )
	{
		return str::Equals< str::IgnoreCase >( pClassName, _T("ComboBoxEx32") );
	}

	bool IsListBox( const TCHAR* pClassName )
	{
		return str::Equals< str::IgnoreCase >( pClassName, _T("ListBox") );
	}

	bool IsHeaderCtrl( const TCHAR* pClassName )
	{
		return str::Find< str::IgnoreCase >( pClassName, _T("SysHeader") ) != std::tstring::npos;
	}

	bool IsListCtrl( const TCHAR* pClassName )
	{
		return str::Find< str::IgnoreCase >( pClassName, _T("SysListView") ) != std::tstring::npos;
	}

	bool IsTreeCtrl( const TCHAR* pClassName )
	{
		return str::Find< str::IgnoreCase >( pClassName, _T("SysTreeView") ) != std::tstring::npos;
	}

	bool IsTabCtrl( const TCHAR* pClassName )
	{
		return str::Find< str::IgnoreCase >( pClassName, _T("SysTabControl") ) != std::tstring::npos;
	}

	bool IsRichEdit( const TCHAR* pClassName )
	{
		return str::Find< str::IgnoreCase >( pClassName, _T("RichEdit") ) != std::tstring::npos;
	}

	bool IsMonthCalCtrl( const TCHAR* pClassName )
	{
		return str::Find< str::IgnoreCase >( pClassName, _T("SysMonthCal") ) != std::tstring::npos;
	}

	bool IsDateTimePickerCtrl( const TCHAR* pClassName )
	{
		return str::Find< str::IgnoreCase >( pClassName, _T("SysDateTimePick") ) != std::tstring::npos;
	}

	bool IsToolbarCtrl( const TCHAR* pClassName )
	{
		return str::Find< str::IgnoreCase >( pClassName, _T("ToolbarWindow") ) != std::tstring::npos;
	}

	bool IsStatusBarCtrl( const TCHAR* pClassName )
	{
		return str::Find< str::IgnoreCase >( pClassName, _T("msctls_statusbar") ) != std::tstring::npos;
	}

	bool IsReBarCtrl( const TCHAR* pClassName )
	{
		return str::Find< str::IgnoreCase >( pClassName, _T("ReBarWindow") ) != std::tstring::npos;
	}

	bool IsSpinButton( const TCHAR* pClassName )
	{
		return str::Find< str::IgnoreCase >( pClassName, _T("MSCtls_UpDown") ) != std::tstring::npos;
	}

	bool IsTrackBar( const TCHAR* pClassName )
	{
		return str::Find< str::IgnoreCase >( pClassName, _T("MSCtls_TrackBar") ) != std::tstring::npos;
	}

	bool IsProgress( const TCHAR* pClassName )
	{
		return str::Find< str::IgnoreCase >( pClassName, _T("msctls_progress") ) != std::tstring::npos;
	}

	bool IsToolTipCtrl( const TCHAR* pClassName )
	{
		return str::Find< str::IgnoreCase >( pClassName, _T("tooltips_class") ) != std::tstring::npos;
	}

	bool IsAnimateCtrl( const TCHAR* pClassName )
	{
		return str::Find< str::IgnoreCase >( pClassName, _T("SysAnimate") ) != std::tstring::npos;
	}


	// control-specific content formatting

	TCHAR lineSep[] = _T("\r\n");
	TCHAR colSep[] = _T("\t");
	TCHAR blankLine[] = _T("-----------------------------------------");


	bool FormatContent_ComboBox( std::tostringstream& ross, const CComboBox& combo )
	{
		DWORD style = combo.GetStyle();
		if ( HasFlag( style, CBS_OWNERDRAWFIXED | CBS_OWNERDRAWVARIABLE ) && !HasFlag( style, CBS_HASSTRINGS ) )
			return false;

		ross << ui::GetComboSelText( combo ) << lineSep;
		if ( int count = combo.GetCount() )
		{
			ross << blankLine << lineSep;
			for ( int i = 0; i < count; ++i )
			{
				CString itemText;
				combo.GetLBText( i, itemText );
				ross << (LPCTSTR)itemText << lineSep;
			}
		}
		return true;
	}

	bool FormatContent_ListBox( std::tostringstream& ross, const CListBox& listBox )
	{
		DWORD style = listBox.GetStyle();
		if ( ( style & ( LBS_OWNERDRAWFIXED | LBS_OWNERDRAWVARIABLE ) ) && !( style & LBS_HASSTRINGS ) )
			return false;

		CString itemText;
		int count = listBox.GetCount();

		if ( !HasFlag( style, LBS_MULTIPLESEL | LBS_NOSEL ) )
		{
			int curSel = listBox.GetCurSel();
			if ( LB_ERR == curSel )
				curSel = listBox.GetCaretIndex();
			if ( curSel != LB_ERR )
			{
				listBox.GetText( curSel, itemText );
				ross << lineSep << blankLine << lineSep;
			}
		}

		for ( int i = 0; i != count; ++i )
		{
			listBox.GetText( i, itemText );
			ross << (LPCTSTR)itemText << lineSep;
		}

		return true;
	}

	bool FormatContent_HeaderCtrl( std::tostringstream& ross, const CHeaderCtrl& headerCtrl )
	{
		ross, headerCtrl;
	/*
		HWND hWndParent = ::GetParent( headerCtrl.m_hWnd );

		if ( hWndParent != NULL )
		{
			DWORD parentStyle = ::GetWindowLong( hWndParent, GWL_STYLE );

			if ( ( parentStyle & WS_CHILD ) != 0 )
				// check if not a child of a list view
				if ( wc::IsListCtrl( wc::GetClassName( hWndParent ) ) )
				{
					CListCtrl parentList;

					parentList.m_hWnd = hWndParent;
					for ( int i = 0, count = headerCtrl.GetItemCount(); i < count; ++i )
					{
						TCHAR itemText[ 256 ] = _T("");
						LVCOLUMN colInfo = { LVCF_TEXT, 0, 0, itemText, COUNT_OF( itemText ), 0 };

						if ( parentList.GetColumn( i, &colInfo ) && ( colInfo.mask & LVCF_TEXT ) != 0 )
						{
							textContent += colInfo.pszText;
							textContent += colSep;
						}
					}
					textContent += lineSep;
					return true;
				}
		}

		return true;
		for ( int i = 0, count = headerCtrl.GetItemCount(); i < count; ++i )
		{
			TCHAR itemText[ 256 ] = "";
			HDITEM hdi;

			hdi.mask = HDI_TEXT | HDI_FORMAT;
			hdi.pszText = itemText;
			hdi.cchTextMax = COUNT_OF( itemText );

			if ( headerCtrl.GetItem( i, &hdi ) )
				if ( hdi.fmt | HDF_STRING )
				{
					textContent += hdi.pszText;
					textContent += colSep;
				}
		}
	*/
		return false;
	}

	bool FormatContent_ListView( std::tostringstream& ross, const CListCtrl& listCtrl )
	{
		ross, listCtrl;
	/*
		HWND hWndHeader = ListView_GetHeader( listCtrl );
		int subItemCount = 1;

		if ( hWndHeader != NULL )
			subItemCount = 1;//TabCtrl_GetItemCount( hWndHeader );
		for ( int i = 0, count = listCtrl.GetItemCount(); i < count; ++i )
		{
			TCHAR buffer[ 256 ] = "";

			for ( int subItem = 0; subItem < subItemCount; ++subItem )
			{
	//			textContent += listCtrl.GetItemText( i, subItem, buffer, 256 );
				ListView_GetItemText( listCtrl.m_hWnd, i, subItem, buffer, 256 )
				textContent += buffer;
				textContent += colSep;
			}
			textContent += lineSep;
		}
	*/
		return false;
	}

	bool FormatContent_TreeView( std::tostringstream& ross, const CTreeCtrl& treeCtrl )
	{
		ross, treeCtrl;
		return false;
	}

	bool FormatContent_TabCtrl( std::tostringstream& ross, const CTabCtrl& tabCtrl )
	{
		ross, tabCtrl;
		return false;
	}


	/*
		"RichEdit", "RichEdit20A", "RichEdit20W"

		"SysMonthCal32"
		"SysDateTimePick32"

		"SysAnimate32"
		"tooltips_class32", "tooltips_class"
		"ToolbarWindow32", "ToolbarWindow"
		"ReBarWindow32", "ReBarWindow"
	*/

	ContentType FormatTextContent( std::tostringstream& ross, const CWnd* pWnd )
	{
		ASSERT( ::IsWindow( pWnd->GetSafeHwnd() ) );
		std::tstring className = wc::GetClassName( pWnd->GetSafeHwnd() );
		const TCHAR* pClassName = className.c_str();

		if ( wc::IsComboBox( pClassName ) )
		{
			if ( FormatContent_ComboBox( ross, *(const CComboBox*)pWnd ) )
				return StructuredText;
		}
		else if ( wc::IsListBox( pClassName ) )
		{
			if ( FormatContent_ListBox( ross, *(const CListBox*)pWnd ) )
				return StructuredText;
		}
		else if ( pred::Equal == _tcsnicmp( pClassName, _T("Sys"), 3 ) )
		{
			if ( wc::IsHeaderCtrl( pClassName ) )
			{
				if ( FormatContent_HeaderCtrl( ross, *(const CHeaderCtrl*)pWnd ) )
					return StructuredText;
			}
			else if ( wc::IsListCtrl( pClassName ) )
			{
				if ( FormatContent_ListView( ross, *(const CListCtrl*)pWnd ) )
					return StructuredText;
			}
			else if ( wc::IsTreeCtrl( pClassName ) )
			{
				if ( FormatContent_TreeView( ross, *(const CTreeCtrl*)pWnd ) )
					return StructuredText;
			}
			else if ( wc::IsTabCtrl( pClassName ) )
			{
				if ( FormatContent_TabCtrl( ross, *(const CTabCtrl*)pWnd ) )
					return StructuredText;
			}
		}

		ross << ui::GetWindowText( pWnd );		// normal text caption window
		return CaptionText;
	}

} // namespace wc

#ifndef OptionsPages_h
#define OptionsPages_h
#pragma once

#include <vector>
#include "FormatterOptions.h"
#include "utl/Path.h"
#include "utl/DialogToolBar.h"
#include "utl/ItemContentEdit.h"
#include "utl/LayoutPropertyPage.h"
#include "utl/SpinEdit.h"


// property pages owned by COptionsSheet

class CGeneralOptionsPage : public CLayoutPropertyPage
{
public:
	CGeneralOptionsPage( void );
	virtual ~CGeneralOptionsPage();

	std::tstring m_developerName;
	std::tstring m_codeTemplateFile;
	UINT m_menuVertSplitCount;
	CSpinButtonCtrl m_menuVertSplitCountSpin;
	std::tstring m_singleLineCommentToken;
	std::tstring m_classPrefix;
	std::tstring m_structPrefix;
	std::tstring m_enumPrefix;
	bool m_autoCodeGeneration;
	bool m_displayErrorMessages;
	bool m_useCommentDecoration;
	bool m_duplicateLineMoveDown;
private:
	// enum { IDD = IDD_OPTIONS_GENERAL_PAGE };
	CItemContentEdit m_templateFileEdit;

	// generated overrides
	protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	// generated message map
	DECLARE_MESSAGE_MAP()
};


class CCodingStandardPage : public CLayoutPropertyPage
{
public:
	CCodingStandardPage( void );
	virtual ~CCodingStandardPage();
public:
	bool m_preserveMultipleWhiteSpace;
	bool m_deleteTrailingWhiteSpace;
	UINT m_splitMaxColumn;
	std::vector< std::tstring > m_breakSeparators;
	std::vector< code::CFormatterOptions::CBraceRule > m_braceRules;
	std::vector< code::CFormatterOptions::COperatorRule > m_operatorRules;
private:
	// enum { IDD = IDD_OPTIONS_CODE_FORMATTING_PAGE };
	CSpinEdit m_splitMaxColumnEdit;
	CItemListEdit m_splitSeparatorsEdit;
	CComboBox m_braceRulesCombo;
	CComboBox m_operatorRulesCombo;

	static const TCHAR m_breakSep[];
public:
	// generated overrides
	protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	// generated message map
	afx_msg void CBnSelChange_BraceRules( void );
	afx_msg void CBnSelChange_OperatorRules( void );
	afx_msg void BnClicked_BraceRulesButton( UINT cmdId );
	afx_msg void BnClicked_OperatorRulesButton( UINT cmdId );

	DECLARE_MESSAGE_MAP()
};


class CCppImplFormattingPage : public CLayoutPropertyPage
{
public:
	CCppImplFormattingPage( void );
	virtual ~CCppImplFormattingPage();

	// enum { IDD = IDD_OPTIONS_CPP_IMPL_FORMATTING_PAGE };
	bool m_returnTypeOnSeparateLine;
	bool m_commentOutDefaultParams;
	UINT m_linesBetweenFunctionImpls;
	CSpinButtonCtrl m_linesBetweenFunctionImplsSpin;

	// generated overrides
	protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	// generated message map
	DECLARE_MESSAGE_MAP()
};


class CBscPathPage : public CLayoutPropertyPage
{
public:
	CBscPathPage( void );
	virtual ~CBscPathPage();

	int MoveItemTo( int srcIndex, int destIndex, bool isDropped = false );

	static bool CheckSearchFilter( fs::CPath& rPath );
private:
	void LoadPathListBox( int selIndex = LB_ERR );
	void UpdateToolBarCmdUI( void );
	int FindPathItemPos( std::tstring& rFolderPath ) const;


	class CDragListBoxEx : public CDragListBox
	{
	public:
		CDragListBoxEx( CBscPathPage* pParentPage ) : m_pParentPage( pParentPage ) { ASSERT_PTR( m_pParentPage ); }

		virtual void Dropped( int srcIndex, CPoint pos )
		{
			int destIndex = ItemFromPt( pos, false );

			CDragListBox::Dropped( srcIndex, pos );
			if ( srcIndex != -1 && destIndex != -1 )
				if ( destIndex != srcIndex && destIndex != srcIndex + 1 )
					m_pParentPage->MoveItemTo( srcIndex, destIndex, true );
		}
	private:
		CBscPathPage* m_pParentPage;
	};


	struct CDirPathItem
	{
		std::tstring GetAsString( void );
		void SetFromString( const std::tstring& itemText );
	public:
		fs::CPath m_searchInfo;
		std::tstring m_displayName;
	};
public:
	std::tstring m_browseInfoPath;
private:
	std::vector< CDirPathItem > m_pathItems;
	ui::CItemContent m_folderContent;
	bool m_isUserUpdate;

	CDialogToolBar m_toolBar;
public:
	// enum { IDD = IDD_OPTIONS_BSC_PATH_PAGE };
	CDragListBoxEx m_pathsListBox;
	CEdit m_searchFilterEdit;
	CEdit m_displayTagEdit;

	// generated overrides
	protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	// generated message map
	afx_msg void LBnSelChange_BrowseFilesPath( void );
	afx_msg void LBnDblClk_BrowseFilesPath( void );
	afx_msg void EnChange_DisplayTag( void );
	afx_msg void EnChange_SearchFilter( void );
	afx_msg void CmAddBscPath( void );
	afx_msg void CmRemoveBscPath( void );
	afx_msg	void UUI_RemoveBscPath( CCmdUI* pCmdUI );
	afx_msg void CmEditBscPath( void );
	afx_msg	void UUI_EditBscPath( CCmdUI* pCmdUI );
	afx_msg void CmMoveUpBscPath( void );
	afx_msg	void UUI_MoveUpBscPath( CCmdUI* pCmdUI );
	afx_msg void CmMoveDownBscPath( void );
	afx_msg	void UUI_MoveDownBscPath( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


#include "utl/LayoutPropertySheet.h"


class COptionsSheet : public CLayoutPropertySheet
{
public:
	COptionsSheet( CWnd* pParent );
	virtual ~COptionsSheet();
private:
	bool m_savedActivePage;
public:
	CGeneralOptionsPage m_generalPage;
	CCodingStandardPage m_formattingPage;
	CCppImplFormattingPage m_cppImplFormattingPage;
	CBscPathPage m_bscPathPage;
};


#endif // OptionsPages_h

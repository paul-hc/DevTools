#ifndef OptionsSheet_h
#define OptionsSheet_h
#pragma once

#include "FormatterOptions.h"
#include "utl/Path.h"
#include "utl/UI/DialogToolBar.h"
#include "utl/UI/ItemContentHistoryCombo.h"
#include "utl/UI/ItemContentEdit.h"
#include "utl/UI/LayoutPropertyPage.h"
#include "utl/UI/LayoutPropertySheet.h"
#include "utl/UI/SpinEdit.h"


class COptionsSheet : public CLayoutPropertySheet
{
public:
	COptionsSheet( CWnd* pParent );
	virtual ~COptionsSheet();

	enum Page { GeneralPage, CodingStandardPage, CppFormattingPage, BscPathPage, DirectoriesPage };
};


// property pages owned by COptionsSheet

class CGeneralOptionsPage : public CLayoutPropertyPage
{
public:
	CGeneralOptionsPage( void );
	virtual ~CGeneralOptionsPage();

	// base overrides
	virtual void ApplyPageChanges( void ) throws_( CRuntimeException );
public:
	std::tstring m_developerName;
	fs::CPath m_codeSnippetsPath;
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

	CItemContentHistoryCombo m_snippetsPathCombo;

	// generated stuff
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	afx_msg void OnChange_SnippetsPath( void );

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

	// generated stuff
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
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

	// generated stuff
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	DECLARE_MESSAGE_MAP()
};


class CBscPathPage : public CLayoutPropertyPage
{
public:
	CBscPathPage( void );
	virtual ~CBscPathPage();

	int MoveItemTo( int srcIndex, int destIndex, bool isDropped = false );
private:
	void SetupPathListBox( int selIndex = LB_ERR );
	void UpdateToolBarCmdUI( void );


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


	struct CSearchPathItem
	{
		bool operator==( const fs::CPath& rightSearchInfo ) const { return m_searchPath == rightSearchInfo; }		// for std::find() with a search path

		fs::CPath& AugmentWildSpec( void );

		std::tstring Format( void );
		void Parse( const std::tstring& itemText );
	public:
		fs::CPath m_searchPath;
		std::tstring m_displayName;
	};
public:
	fs::CPath m_browseInfoPath;
private:
	std::vector< CSearchPathItem > m_pathItems;
	ui::CItemContent m_folderContent;
	bool m_isUserUpdate;

	CDialogToolBar m_toolBar;
public:
	// enum { IDD = IDD_OPTIONS_BSC_PATH_PAGE };
	CDragListBoxEx m_pathsListBox;
	CEdit m_searchFilterEdit;
	CEdit m_displayTagEdit;

	// generated stuff
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
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


class CIncludeDirectories;


class CDirectoriesPage : public CLayoutPropertyPage
{
public:
	CDirectoriesPage( void );
	virtual ~CDirectoriesPage();

// base overrides
	virtual void ApplyPageChanges( void ) throws_( CRuntimeException );
private:
	std::tstring MakeNewUniqueName( void ) const;
private:
	std::auto_ptr<CIncludeDirectories> m_pDirSets;			// a working copy, so user can cancel with no modification

	// enum { IDD = IDD_OPTIONS_DIRECTORIES_PAGE };
	CComboBox m_dirSetsCombo;
	CItemListEdit m_includePathEdit;
	CItemListEdit m_sourcePathEdit;
	CItemListEdit m_libraryPathEdit;
	CItemListEdit m_binaryPathEdit;
	CDialogToolBar m_toolbar;

	// generated stuff
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	afx_msg void CBnSelChange_DirSets( void );
	afx_msg void OnAddDirSet( void );
	afx_msg void OnRemoveDirSet( void );
	afx_msg void OnRenameDirSet( void );
	afx_msg void OnResetDefault( void );
	afx_msg void OnCopyPaths( void );
	afx_msg void OnUpdateCopyPaths( CCmdUI* pCmdUI );
	afx_msg void OnPastePaths( void );
	afx_msg void OnUpdatePastePaths( CCmdUI* pCmdUI );

	DECLARE_MESSAGE_MAP()
};


#endif // OptionsSheet_h

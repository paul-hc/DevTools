#ifndef LayoutBasePropertySheet_h
#define LayoutBasePropertySheet_h
#pragma once

#include <afxdlgs.h>
#include "Dialog_fwd.h"
#include "utl/Subject.h"


class CLayoutPropertyPage;
class CMacroCommand;
class CScopedApplyMacroCmd;
namespace utl { interface ICommandExecutor; }


abstract class CLayoutBasePropertySheet : public CPropertySheet
	, public TSubject
	, public ui::ICustomCmdInfo
{
	friend class CScopedApplyMacroCmd;

	// hidden base methods
	using CPropertySheet::IsModeless;		// hide the base version with BOOL return type
protected:
	CLayoutBasePropertySheet( const TCHAR* pTitle, CWnd* pParent, UINT selPageIndex );
public:
	virtual ~CLayoutBasePropertySheet();

	bool IsModeless( void ) const { return CPropertySheet::IsModeless() != FALSE; }

	const std::tstring& GetSection( void ) const { return m_regSection; }
	CMacroCommand* GetApplyMacroCmd( void ) const { return m_pApplyMacroCmd.get(); }

	CLayoutPropertyPage* GetPage( int pageIndex ) const;

	template< typename PageType >
	PageType* GetPageAs( int pageIndex ) const { return dynamic_cast<PageType*>( GetPage( pageIndex ) ); }	// dynamic so that it works with interfaces

	template< typename PageType >
	PageType* GetCreatedPageAs( int pageIndex ) const;			// only if page created (pPage->m_hWnd != nullptr); useful for selective UI updates

	CLayoutPropertyPage* GetActivePage( void ) const;
	void SetInitialPageIndex( UINT initialPageIndex ) { REQUIRE( nullptr == m_hWnd ); m_initialPageIndex = initialPageIndex; }

	std::tstring GetPageTitle( int pageIndex ) const;
	void SetPageTitle( int pageIndex, const std::tstring& pageTitle );

	CImageList& GetTabImageList( void ) { return m_tabImageList; }			// initialize before sheet creation

	void RemoveAllPages( void );
	void DeleteAllPages( void );

	virtual bool IsSheetModified( void ) const;
	void SetSheetModified( bool modified = true );

	void SetCreateAllPages( void );		// create all pages when sheet is created
	void SetSingleLineTab( bool singleLineTab = true );
	void SetPagesUseLazyUpdateData( bool useLazyUpdateData = true );
	void OutputPages( void );
	bool OutputPage( int pageIndex );		// safe, no update if page not created

	CToolTipCtrl* GetSheetTooltip( void ) const { return m_pTooltipCtrl.get(); }

	// utl::ISubject interface (partial)
	virtual const std::tstring& GetCode( void ) const override { return m_titleCode; }

	// ui::ICustomCmdInfo interface
	virtual void QueryTooltipText( OUT std::tstring& rText, UINT cmdId, CToolTipCtrl* pTooltip ) const;

	// overridables
	virtual void LoadFromRegistry( void );
	virtual void SaveToRegistry( void );

	virtual void LayoutSheet( void );
	virtual void LayoutPages( const CRect& rPageRect );
protected:
	virtual bool ApplyChanges( void );
	virtual void OnChangesApplied( void );
	virtual CButton* GetSheetButton( UINT buttonId ) const;

	bool AnyPageResizable( void ) const;
	virtual bool IsPageResizable( const CLayoutPropertyPage* pPage ) const;

	void RegisterTabTooltips( void );
private:
	UINT m_initialPageIndex;						// force initial page activation (otherwise uses the one saved in registry, or default)
	std::tstring m_titleCode;
protected:
	std::auto_ptr<CMacroCommand> m_pApplyMacroCmd;	// used optionally for Apply
	utl::ICommandExecutor* m_pCommandExecutor;		// to execute Apply macro

	bool m_manageOkButtonState;						// enable OK button when modified, disable it when not modified

	CImageList m_tabImageList;
	std::auto_ptr<CToolTipCtrl> m_pTooltipCtrl;
	enum { TabItem_BaseToolId = 0xDFF0 };			// base id used to encode tool ids for tab item (page) indexes
public:
	std::tstring m_regSection;

	// generated stuff
public:
	virtual void BuildPropPageArray( void ) override;
	virtual BOOL PreTranslateMessage( MSG* pMsg ) override;
	virtual BOOL OnInitDialog( void ) override;
protected:
	virtual void OnDestroy( void );
	virtual void OnSize( UINT sizeType, int cx, int cy );
	virtual BOOL OnTtnNeedText( UINT cmdId, NMHDR* pNmHdr, LRESULT* pResult );

	DECLARE_MESSAGE_MAP()
};


// template code

template< typename PageType >
PageType* CLayoutBasePropertySheet::GetCreatedPageAs( int pageIndex ) const
{
	CPropertyPage* pPage = CPropertySheet::GetPage( pageIndex );
	return pPage->GetSafeHwnd() != nullptr ? dynamic_cast<PageType*>( pPage ) : nullptr;
}


#endif // LayoutBasePropertySheet_h

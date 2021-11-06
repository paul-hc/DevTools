#ifndef ListLikeCtrlBase_h
#define ListLikeCtrlBase_h
#pragma once

#include "ObjectCtrlBase.h"
#include "TextEffect.h"
#include "CustomDrawImager_fwd.h"
#include <map>


class CListLikeCtrlBase;
class CReportListControl;
namespace lv { struct CMatchEffects; }


namespace ui
{
	class CHandledNotificationsCache;
	class CFontEffectCache;


	enum StdImageIndex { No_Image = -1, Transparent_Image = -2 };


	interface ITextEffectCallback		// passes the control to identify multile lists in parent (mediator pattern)
	{
		virtual void CombineTextEffectAt( ui::CTextEffect& rTextEffect, LPARAM rowKey, int subItem, CListLikeCtrlBase* pCtrl ) const = 0;		// for CTreeControl: rowKey is HTREEITEM hItem; subItem is unused

		// list-ctrl specific
		virtual void ModifyDiffTextEffectAt( lv::CMatchEffects& rEffects, LPARAM rowKey, int subItem, CReportListControl* pCtrl ) const { &rEffects, rowKey, subItem, pCtrl; }
	};
}


abstract class CListLikeCtrlBase
	: public CObjectCtrlBase
	, public ICustomDrawControl
	, protected ui::ITextEffectCallback
{
protected:
	CListLikeCtrlBase( CWnd* pCtrl, UINT ctrlAccelId = 0 );
	~CListLikeCtrlBase();
public:
	bool GetUseExplorerTheme( void ) const { return m_useExplorerTheme; }
	void SetUseExplorerTheme( bool useExplorerTheme = true );

	void SetTextEffectCallback( ui::ITextEffectCallback* pTextEffectCallback ) { m_pTextEffectCallback = pTextEffectCallback; }

	template< typename Type >
	static Type* AsPtr( LPARAM data ) { return reinterpret_cast<Type*>( data ); }

	static inline utl::ISubject* ToSubject( LPARAM data ) { return checked_static_cast<utl::ISubject*>( (utl::ISubject*)data ); }

	// ICustomDrawControl interface
	virtual CBaseCustomDrawImager* GetCustomDrawImager( void ) const;
	virtual void SetCustomFileGlyphDraw( bool showGlyphs = true );
protected:
	// ui::ITextEffectCallback interface
	virtual void CombineTextEffectAt( ui::CTextEffect& rTextEffect, LPARAM rowKey, int subItem, CListLikeCtrlBase* pCtrl ) const;
	virtual void ModifyDiffTextEffectAt( lv::CMatchEffects& rEffects, LPARAM rowKey, int subItem, CReportListControl* pCtrl ) const;

	virtual void SetupControl( void );

	ui::CFontEffectCache* GetFontEffectCache( void );

	bool ParentHandlesWmCommand( UINT cmdNotifyCode ) { return ParentHandles( WM_COMMAND, cmdNotifyCode ); }
	bool ParentHandlesWmNotify( UINT wmNotifyCode ) { return ParentHandles( WM_NOTIFY, wmNotifyCode ); }
private:
	bool ParentHandles( UINT cmdMessage, UINT notifyCode );
private:
	bool m_useExplorerTheme;
	std::auto_ptr<ui::CHandledNotificationsCache> m_pParentHandlesCache;
protected:
	std::auto_ptr<ui::CFontEffectCache> m_pFontCache;			// self-encapsulated
	ui::ITextEffectCallback* m_pTextEffectCallback;
	std::auto_ptr<CBaseCustomDrawImager> m_pCustomImager;
public:
	ui::CTextEffect m_ctrlTextEffect;							// for all items in the list
};


abstract class CListLikeCustomDrawBase
{
protected:
	CListLikeCustomDrawBase( NMCUSTOMDRAW* pDraw );
public:
	static bool IsTooltipDraw( const NMCUSTOMDRAW* pDraw );
private:
	NMCUSTOMDRAW* m_pDraw;
protected:
	CDC* m_pDC;
public:
	static bool s_useDefaultDraw;
	static bool s_dbgGuides;
};


namespace dbg
{
	const TCHAR* FormatDrawStage( DWORD dwDrawStage );
}


#endif // ListLikeCtrlBase_h

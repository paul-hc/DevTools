#ifndef ItemContentEdit_h
#define ItemContentEdit_h
#pragma once

#include "BaseDetailHostCtrl.h"
#include "TextEdit.h"


// edit for a string, dir-path or file-path with details button

class CItemContentEdit : public CBaseItemContentCtrl< CTextEdit >
{
	typedef CBaseItemContentCtrl< CTextEdit > BaseClass;
public:
	CItemContentEdit( ui::ContentType type = ui::String, const TCHAR* pFileFilter = NULL );
	virtual ~CItemContentEdit();
protected:
	// interface IBuddyCommand (may be overridden)
	virtual void OnBuddyCommand( UINT cmdId );
};


// edit for a separated list of items with details button

class CItemListEdit : public CBaseItemContentCtrl< CTextEdit >
{
	typedef CBaseItemContentCtrl< CTextEdit > BaseClass;
public:
	CItemListEdit( const TCHAR* pSeparator = _T(";") );
	virtual ~CItemListEdit();

	enum ListEditor { ListDialog, ListEditDialog, Custom };
	void SetListEditor( ListEditor listEditor ) { m_listEditor = listEditor; }

	void DDX_Items( CDataExchange* pDX, std::tstring& rFlatItems, int ctrlId = 0 );
	void DDX_Items( CDataExchange* pDX, std::vector< std::tstring >& rItems, int ctrlId = 0 );
	void DDX_ItemsUiEscapeSeqs( CDataExchange* pDX, std::tstring& rFlatItems, int ctrlId = 0 );
	void DDX_ItemsUiEscapeSeqs( CDataExchange* pDX, std::vector< std::tstring >& rItems, int ctrlId = 0 );
protected:
	// interface IBuddyCommand
	virtual void OnBuddyCommand( UINT cmdId );

	virtual UINT GetStockButtonIconId( void ) const;
private:
	const TCHAR* m_pSeparator;
	ListEditor m_listEditor;
};


#endif // ItemContentEdit_h

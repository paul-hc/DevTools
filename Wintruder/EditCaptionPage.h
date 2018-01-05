#ifndef EditCaptionPage_h
#define EditCaptionPage_h
#pragma once

#include "utl/TextEdit.h"
#include "DetailBasePage.h"


namespace wc { enum ContentType; }


class CEditCaptionPage : public CDetailBasePage
{
public:
	CEditCaptionPage( void );
	virtual ~CEditCaptionPage();

	// IWndDetailObserver interface
	virtual bool IsDirty( void ) const;
	virtual void OnTargetWndChanged( const CWndSpot& targetWnd );

	virtual void ApplyPageChanges( void ) throws_( CRuntimeException );
private:
	std::tstring m_textContent;
	wc::ContentType m_contentType;
private:
	// enum { IDD = IDD_EDIT_CAPTION_PAGE };
	CTextEdit m_contentEdit;
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	afx_msg void OnEnChange_Caption( void );

	DECLARE_MESSAGE_MAP()
};


#endif // EditCaptionPage_h

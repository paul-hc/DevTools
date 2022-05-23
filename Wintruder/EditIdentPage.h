#ifndef EditIdentPage_h
#define EditIdentPage_h
#pragma once

#include "DetailBasePage.h"
#include "utl/UI/NumericEdit.h"


class CEditIdentPage : public CDetailBasePage
{
public:
	CEditIdentPage( void );
	virtual ~CEditIdentPage();

	// IWndDetailObserver interface
	virtual bool IsDirty( void ) const;
	virtual void OnTargetWndChanged( const CWndSpot& targetWnd );

	virtual void ApplyPageChanges( void ) throws_( CRuntimeException );
protected:
	bool UseWndId( void ) const;
	int FindLiteralPos( int id ) const;
private:
	typedef short TCmdId;
	TCmdId m_id;
private:
	// enum { IDD = IDD_EDIT_IDENT_PAGE };
	CNumericEdit<TCmdId> m_decIdentEdit;
	CNumericEdit<unsigned short> m_hexIdentEdit;
	CComboBox m_literalCombo;

	// generated stuff
protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	afx_msg void OnEnChange_DecimalEdit( void );
	afx_msg void OnEnChange_HexEdit( void );
	afx_msg void OnCbnSelChange_Literal( void );

	DECLARE_MESSAGE_MAP()
};


#endif // EditIdentPage_h

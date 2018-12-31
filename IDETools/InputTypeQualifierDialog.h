#ifndef InputTypeQualifierDialog_h
#define InputTypeQualifierDialog_h
#pragma once


class CInputTypeQualifierDialog : public CDialog
{
public:
	CInputTypeQualifierDialog( const CString& typeQualifier, CWnd* pParent );
public:
	//enum { IDD = IDD_INPUT_TYPE_QUALIFIER_DIALOG };

	CString	m_typeQualifier;

	// generated stuff
	protected:
	virtual void DoDataExchange( CDataExchange* pDX );
protected:
	virtual void OnOK( void );

	DECLARE_MESSAGE_MAP()
};


#endif // InputTypeQualifierDialog_h

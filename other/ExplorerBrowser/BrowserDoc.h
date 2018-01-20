#pragma once


class CBrowserDoc : public CDocument
{
	DECLARE_DYNCREATE(CBrowserDoc)
protected:
	CBrowserDoc( void );
public:
	virtual ~CBrowserDoc();

	// base overrides
	virtual BOOL OnNewDocument( void );
	virtual void Serialize( CArchive& ar );
protected:
	// generated dtuff
protected:
	DECLARE_MESSAGE_MAP()
};



// ExplorerBrowserDoc.h : interface of the CExplorerBrowserDoc class
//


#pragma once


class CExplorerBrowserDoc : public CDocument
{
protected: // create from serialization only
	CExplorerBrowserDoc();
	DECLARE_DYNCREATE(CExplorerBrowserDoc)

// Attributes
public:

// Operations
public:

// Overrides
public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);

// Implementation
public:
	virtual ~CExplorerBrowserDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	DECLARE_MESSAGE_MAP()
};



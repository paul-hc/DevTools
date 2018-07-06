#pragma once


class CBrowserDoc : public CDocument
{
	DECLARE_DYNCREATE( CBrowserDoc )
protected:
	CBrowserDoc( void );
public:
	virtual ~CBrowserDoc();

	// base overrides
	virtual BOOL OnNewDocument( void );
	virtual BOOL OnOpenDocument( LPCTSTR pDirPath );
	virtual void OnCloseDocument( void );
	virtual void Serialize( CArchive& ar );

	FOLDERVIEWMODE GetFilePaneViewMode( void ) const { return m_filePaneViewMode; }
	void SetFilePaneViewMode( FOLDERVIEWMODE filePaneViewMode ) { m_filePaneViewMode = filePaneViewMode; }
private:
	FOLDERVIEWMODE m_filePaneViewMode;
protected:
	// generated stuff
protected:
	DECLARE_MESSAGE_MAP()
};



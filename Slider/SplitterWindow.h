#ifndef SplitterWindow_h
#define SplitterWindow_h
#pragma once


class CAlbumThumbListView;


class CSplitterWindow : public CSplitterWnd
{
public:
	CSplitterWindow( void );
	virtual ~CSplitterWindow();

	bool DoRecreateWindow( CAlbumThumbListView& rThumbView, const CSize& thumbSize );
private:
	bool m_hasTracked;
public:
	// splitter overrides
	virtual void RecalcLayout( void );

	void MoveColumnToPos( int x, int col, bool checkLayout = false );
protected:
	virtual void TrackColumnSize( int x, int col );
protected:
	afx_msg void OnLButtonDblClk( UINT mkFlags, CPoint point );

	DECLARE_MESSAGE_MAP()
};


#endif // SplitterWindow_h

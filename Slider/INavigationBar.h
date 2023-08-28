#ifndef INavigationBar_h
#define INavigationBar_h
#pragma once


interface INavigationBar
{
	virtual bool OutputNavigRange( UINT imageCount ) = 0;
	virtual bool OutputNavigPos( int imagePos ) = 0;
	virtual int InputNavigPos( void ) const = 0;
};


class CAlbumImageView;


interface IAlbumBar
{
	virtual void InitAlbumImageView( CAlbumImageView* pAlbumView ) = 0;
	virtual void ShowBar( bool show ) = 0;

	// events
	virtual void OnCurrPosChanged( void ) = 0;
	virtual void OnNavRangeChanged( void ) = 0;
	virtual void OnSlideDelayChanged( void ) = 0;
};


#endif // INavigationBar_h

#ifndef INavigationBar_h
#define INavigationBar_h
#pragma once


interface INavigationBar
{
	virtual bool OutputNavigRange( UINT imageCount ) = 0;
	virtual bool OutputNavigPos( int imagePos ) = 0;
	virtual int InputNavigPos( void ) const = 0;
};


#endif // INavigationBar_h

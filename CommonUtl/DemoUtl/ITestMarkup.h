#ifndef ITestMarkup_h
#define ITestMarkup_h
#pragma once


// identifies the top parent implementing a demo feature; prevents infinite recursion for details creation

interface ITestMarkup
{
	virtual ~ITestMarkup() {}
};


template< typename CountableType >
int GetMarkupDepth( const CountableType* pWnd )
{
	int depth = 1;
	for ( const CWnd* pParent = pWnd; pParent != NULL && !is_a< ITestMarkup >( pParent ); pParent = pParent->GetParent() )
		if ( is_a< CountableType >( pParent ) )
			++depth;

	return depth;
}


#endif // ITestMarkup_h

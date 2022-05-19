#ifndef ToolStrip_h
#define ToolStrip_h
#pragma once

#include "ToolImageList.h"


class CImageStore;


class CToolStrip : public CToolImageList
{
public:
	CToolStrip( IconStdSize iconStdSize = SmallIcon ) : CToolImageList( iconStdSize ) {}
	CToolStrip( const UINT buttonIds[], size_t count, IconStdSize iconStdSize = SmallIcon ) : CToolImageList( buttonIds, count, iconStdSize ) {}
	~CToolStrip();

	enum { UseButtonId = -1, NullIconId };

	CToolStrip& AddButton( UINT buttonId, UINT iconId = (UINT)UseButtonId );
	CToolStrip& AddButton( UINT buttonId, HICON hIcon );
	CToolStrip& AddSeparator( void ) { return AddButton( 0 ); }

	CToolStrip& AddButtons( const UINT buttonIds[], size_t buttonCount, IconStdSize iconStdSize = SmallIcon );		// same buttonId and iconId
};


#endif // ToolStrip_h

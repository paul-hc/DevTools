#ifndef WicBitmap_h
#define WicBitmap_h
#pragma once

#include "ImagingWic.h"
#include "ErrorHandler.h"


// WIC bitmap that can be loaded from file/resource/stream and saved to file/stream using WIC utilities
//
class CWicBitmap : public CErrorHandler
				 , private utl::noncopyable
{
public:
	CWicBitmap( utl::ErrorHandling handlingMode = utl::CheckMode )
		: CErrorHandler( handlingMode )
	{
	}

	CWicBitmap( IWICBitmapSource* pWicBitmap, utl::ErrorHandling handlingMode = utl::CheckMode )
		: CErrorHandler( handlingMode )
	{
		SetWicBitmap( pWicBitmap );
	}

	virtual ~CWicBitmap() {}

	void Clear( void ) { m_pBitmapOrigin.reset(); }

	IWICBitmapSource* GetWicBitmap( void ) const { return GetOrigin().GetSourceBitmap(); }

	bool SetWicBitmap( IWICBitmapSource* pWicBitmap )
	{
		m_pBitmapOrigin.reset( new wic::CBitmapOrigin( safe_ptr( pWicBitmap ), GetHandlingMode() ) );
		return m_pBitmapOrigin->IsValid();
	}

	bool IsValid( void ) const { return m_pBitmapOrigin.get() != NULL && GetBmpFmt().IsValid(); }
	wic::CBitmapOrigin& GetOrigin( void ) const { return *m_pBitmapOrigin; }
	const wic::CBitmapFormat& GetBmpFmt( void ) const { return GetOrigin().GetBmpFmt(); }
private:
	std::auto_ptr<wic::CBitmapOrigin> m_pBitmapOrigin;			// keeps track of the source bitmap for saving with metadata
};


#endif // WicBitmap_h

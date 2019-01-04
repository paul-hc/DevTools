#ifndef WicBitmap_h
#define WicBitmap_h
#pragma once

#include "ImagingWic.h"
#include "ThrowMode.h"


// WIC bitmap that can be loaded from file/resource/stream and saved to file/stream using WIC utilities
//
class CWicBitmap : public CThrowMode, private utl::noncopyable
{
public:
	CWicBitmap( bool throwMode = false ) : CThrowMode( throwMode ) {}
	CWicBitmap( IWICBitmapSource* pWicBitmap, bool throwMode = false ) : CThrowMode( throwMode ) { SetWicBitmap( pWicBitmap ); }
	virtual ~CWicBitmap() {}

	void Clear( void ) { m_pBitmapOrigin.reset(); }

	IWICBitmapSource* GetWicBitmap( void ) const { return GetOrigin().GetSourceBitmap(); }

	bool SetWicBitmap( IWICBitmapSource* pWicBitmap )
	{
		m_pBitmapOrigin.reset( new wic::CBitmapOrigin( safe_ptr( pWicBitmap ), IsThrowMode() ) );
		return m_pBitmapOrigin->IsValid();
	}

	bool IsValid( void ) const { return m_pBitmapOrigin.get() != NULL && GetBmpFmt().IsValid(); }
	wic::CBitmapOrigin& GetOrigin( void ) const { return *m_pBitmapOrigin; }
	const wic::CBitmapFormat& GetBmpFmt( void ) const { return GetOrigin().GetBmpFmt(); }
private:
	std::auto_ptr< wic::CBitmapOrigin > m_pBitmapOrigin;			// keeps track of the source bitmap for saving with metadata
};


#endif // WicBitmap_h

#ifndef FileAttr_h
#define FileAttr_h
#pragma once

#include "utl/PathItemBase.h"
#include "utl/UI/ImagePathKey.h"
#include "FileAttr_fwd.h"


class CTimer;
namespace fs { struct CFileState; }
namespace wic { enum ImageFormat; }
namespace app { enum ModelSchema; }


class CFileAttr : public CPathItemBase
{
public:
	CFileAttr( void );
	CFileAttr( const fs::CPath& filePath );				// only for concrete files
	CFileAttr( const fs::CFileState& streamState );
	virtual ~CFileAttr();

	// utl::ISubject interface
	virtual const std::tstring& GetCode( void ) const;
	virtual std::tstring GetDisplayCode( void ) const;

	bool operator==( const CFileAttr& right ) const { return GetPath() == right.GetPath(); }
	bool operator!=( const CFileAttr& right ) const { return !operator==( right ); }

	void Stream( CArchive& archive );

	bool IsValid( void ) const { return GetPath().FileExist(); }

	const fs::ImagePathKey& GetPathKey( void ) const { return m_pathKey; }
	void SetPathKey( const fs::ImagePathKey& pathKey ) { m_pathKey = pathKey; }

	const fs::CFlexPath& GetPath( void ) const { return m_pathKey.first; }
	fs::CFlexPath& RefPath( void ) { return m_pathKey.first; }
	void SetPath( const fs::CPath& filePath, UINT framePos = 0 ) { m_pathKey.first = filePath.Get(); m_pathKey.second = framePos; }

	wic::ImageFormat GetImageFormat( void ) const { return m_imageFormat; }
	const FILETIME& GetLastModifTime( void ) const { return m_lastModifTime; }
	UINT GetFileSize( void ) const { return m_fileSize; }
	const CSize& GetImageDim( void ) const;

	size_t GetBaselinePos( void ) const { ASSERT( m_baselinePos != utl::npos ); return m_baselinePos; }
	void StoreBaselinePos( size_t baselinePos ) { m_baselinePos = baselinePos; }		// store it only once (unless this is from an embedded image archive)

	std::tstring FormatFileSize( DWORD divideBy = KiloByte, const TCHAR* pFormat = _T("%s KB") ) const;
	std::tstring FormatLastModifTime( LPCTSTR format = _T("%d-%m-%Y %H:%M:%S") ) const { return CTime( m_lastModifTime ).Format( format ).GetString(); }
private:
	bool ReadFileStatus( void );
	const CSize& GetSavingImageDim( void ) const;

	enum SavingFlags
	{
		Saving_PromptedSpeedUp			= BIT_FLAG( 8 ),
		Saving_SkipImageDimEvaluation	= BIT_FLAG( 9 ),
		Loading_InspectedPathEncoding	= BIT_FLAG( 10 )
	};

	static bool PromptedSpeedUpSaving( CTimer& rSavingTimer );
	static app::ModelSchema EvalLoadingSchema( CArchive& rLoadArchive );
private:
	persist fs::ImagePathKey m_pathKey;
	persist wic::ImageFormat m_imageFormat;		// formerly FileType in Slider_v5_5- (no longer needed, really)
	persist FILETIME m_lastModifTime;
	persist UINT m_fileSize;
	persist mutable CSize m_imageDim;			// used for image dimensions comparison

	// transient
	size_t m_baselinePos;						// for baseline sequence: original found position on searching
};


namespace fs
{
	namespace traits
	{
		inline const fs::CPath& GetPath( const CFileAttr* pFileAttr ) { ASSERT_PTR( pFileAttr ); return pFileAttr->GetPath(); }
		inline void SetPath( CFileAttr* pFileAttr, const fs::CPath& filePath ) { ASSERT_PTR( pFileAttr ); pFileAttr->SetPath( filePath ); }
	}
}


#endif // FileAttr_h

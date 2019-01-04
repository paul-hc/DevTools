#ifndef FileAttr_h
#define FileAttr_h
#pragma once

#include "utl/ComparePredicates.h"
#include "utl/Subject.h"
#include "utl/UI/ImagePathKey.h"


enum FileType { FT_Generic, FT_BMP, FT_JPEG, FT_GIFF, FT_TIFF };


struct CFileAttr : public CSubject
{
	CFileAttr( void );
	CFileAttr( const fs::CPath& filePath );				// only for concrete files
	CFileAttr( const CFileFind& foundFile );			// get file attributes
	~CFileAttr();

	bool operator==( const CFileAttr& right ) const { return GetPath() == right.GetPath(); }
	bool operator!=( const CFileAttr& right ) const { return !operator==( right ); }

	void SetFromTransferPair( const fs::CFlexPath& srcPath, const fs::CFlexPath& destPath );
	void Stream( CArchive& archive );

	bool IsValid( void ) const { return GetPath().FileExist(); }

	const fs::CFlexPath& GetPath( void ) const { return m_pathKey.first; }
	const CSize& GetImageDim( void ) const;
	std::tstring FormatFileSize( DWORD divideBy = KiloByte, const TCHAR* pFormat = _T("%s KB") ) const;
	std::tstring FormatLastModifTime( LPCTSTR format = _T("%d-%m-%Y %H:%M:%S") ) const { return CTime( m_lastModifTime ).Format( format ).GetString(); }

	// utl::ISubject interface
	virtual const std::tstring& GetCode( void ) const;
	virtual std::tstring GetDisplayCode( void ) const;

	static FileType LookupFileType( const TCHAR* pFilePath );
private:
	bool ReadFileStatus( void );
public:
	persist fs::ImagePathKey m_pathKey;
	persist FileType m_type;
	persist FILETIME m_lastModifTime;
	persist UINT m_fileSize;
private:
	persist mutable CSize m_imageDim;			// used for image dimensions comparison
};


namespace fs
{
	namespace traits
	{
		inline const fs::CPath& GetPath( const CFileAttr& fileAttr ) { return fileAttr.GetPath(); }
		inline void SetPath( CFileAttr& rDestFileAttr, const fs::CPath& filePath ) { rDestFileAttr.m_pathKey.first.Set( filePath.Get() ); }
	}
}


namespace func
{
	struct ToFilePath
	{
		const fs::CFlexPath& operator()( const CFileAttr& fileAttr ) const
		{
			return fileAttr.GetPath();
		}
	};

	struct ToFileSize
	{
		UINT operator()( const CFileAttr& fileAttr ) const
		{
			return fileAttr.m_fileSize;
		}
	};

	struct ToImageArea
	{
		size_t operator()( const CFileAttr& fileAttr ) const
		{
			CSize imageDim = fileAttr.GetImageDim();
			if ( imageDim.cx < 0 || imageDim.cy < 0 )		// error accessing the image file
				return 0;
			return static_cast< size_t >( imageDim.cx ) * static_cast< size_t >( imageDim.cy );
		}
	};

	struct ToImageWidth
	{
		int operator()( const CFileAttr& fileAttr ) const
		{
			int imageWidth = fileAttr.GetImageDim().cx;
			return imageWidth >= 0 ? imageWidth : 0;
		}
	};

	struct ToImageHeight
	{
		int operator()( const CFileAttr& fileAttr ) const
		{
			int imageHeight = fileAttr.GetImageDim().cy;
			return imageHeight >= 0 ? imageHeight : 0;
		}
	};


	struct AddFileSize
	{
		size_t operator()( size_t totalFileSize, const CFileAttr& right ) const
		{
			return totalFileSize + right.m_fileSize;
		}
	};
}


namespace pred
{
	typedef CompareAdapter< CompareNaturalPath, func::ToFilePath > CompareFileAttrPath;

	typedef CompareAdapter< CompareValue, func::ToFileSize > CompareFileAttrSize;

	typedef CompareAdapter< CompareValue, func::ToImageArea > CompareImageArea;
	typedef CompareAdapter< CompareValue, func::ToImageWidth > CompareImageWidth;
	typedef CompareAdapter< CompareValue, func::ToImageHeight > CompareImageHeight;
	
	typedef JoinCompare< CompareImageArea, JoinCompare< CompareImageWidth, CompareImageHeight > > CompareImageDimensions;		// area | width | height
}


#endif // FileAttr_h

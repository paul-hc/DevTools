#ifndef FileType_h
#define FileType_h
#pragma once


namespace ft
{
	enum FileType		// (!) careful to maintain C++ Macros.dsm corresponding constants when changing these constants
	{
		Unknown,
		Extless,		// extensionless headers such as <vector>
		H,
		CPP,
		C,
		HXX,
		CXX,
		IDL,
		TLB,
		RC,
		RES,
		DEF,
		DSP,
		DSW,
		DSM,
		BAS,

		DLL,
		EXE,
		LIB,

		SQL,
		TAB,
		PK,
		PKG,
		PKB,
		PKS,
		PAC,
		OT,
		OTB,

			_ImageCount = EXE + 1
	};


	FileType FindFileType( const TCHAR* pFilePath );
	FileType FindTypeOfExtension( const TCHAR* pExt );
	CImageList& GetFileTypeImageList( void );
}


#endif // FileType_h

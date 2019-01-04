
#include "stdafx.h"
#include "FileType.h"
#include "resource.h"
#include "utl/UI/Image_fwd.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ft
{
	FileType FindFileType( const TCHAR* pFilePath )
	{
		ASSERT( !str::IsEmpty( pFilePath ) );
		TCHAR ext[ _MAX_EXT ];
		_tsplitpath( pFilePath, NULL, NULL, NULL, ext );
		return FindTypeOfExtension( ext );
	}

	FileType FindTypeOfExtension( const TCHAR* pExt )
	{
		static const struct { const TCHAR* m_pExt; FileType m_fileType; } knownExts[] =
		{
			{ _T(""),     Extless },		// extensionless headers
			{ _T(".h"),   H   },
			{ _T(".cpp"), CPP },
			{ _T(".c"),   CPP },
			{ _T(".lcc"), CPP },
			{ _T(".hxx"), HXX },
			{ _T(".hpp"), HXX },
			{ _T(".inl"), HXX },
			{ _T(".cxx"), CXX },

			{ _T(".odl"), IDL },
			{ _T(".idl"), IDL },
			{ _T(".tlb"), TLB },

			{ _T(".rc"),  RC  },
			{ _T(".rc2"), RC  },
			{ _T(".rh"),  RC  },
			{ _T(".dlg"), RC  },
			{ _T(".res"), RES },
			{ _T(".def"), DEF },
			{ _T(".dsp"), DSP },
			{ _T(".dsw"), DSW },
			{ _T(".dsm"), DSM },
			{ _T(".bas"), BAS },
			{ _T(".vbs"), BAS },
			{ _T(".wsh"), BAS },

			{ _T(".dll"), DLL },
			{ _T(".pkg"), DLL },
			{ _T(".exe"), EXE },
			{ _T(".lib"), LIB },

			{ _T(".sql"), SQL },
			{ _T(".tab"), TAB },
			{ _T(".pk"),  PK  },
			{ _T(".pkg"), PKG },
			{ _T(".pkb"), PKB },
			{ _T(".pks"), PKS },
			{ _T(".pac"), PAC },
			{ _T(".ot"),  OT },
			{ _T(".otb"), OTB }
		};

		ASSERT_PTR( pExt );

		for ( unsigned int i = 0; i != COUNT_OF( knownExts ); ++i )
			if ( str::Equals< str::IgnoreCase >( knownExts[ i ].m_pExt, pExt ) )
				return knownExts[ i ].m_fileType;

		return Unknown;
	}


	CImageList& GetFileTypeImageList( void )
	{
		static CImageList imageList;
		if ( NULL == imageList.GetSafeHandle() )
			res::LoadImageList( imageList, IDB_FILE_TYPE_STRIP, ft::_ImageCount, CIconId::GetStdSize( SmallIcon ), color::ToolStripPink );

		return imageList;
	}
}

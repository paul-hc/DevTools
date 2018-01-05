#ifndef ContainerComponents_h
#define ContainerComponents_h
#pragma once


#include "TokenRange.h"
#include "LanguageSearchEngine.h"
#include "utl/RuntimeException.h"


namespace code
{
	enum LibraryType { STL, MFC };

	struct ContainerComponents
	{
		ContainerComponents( const TCHAR* pCodeText );
		~ContainerComponents();

		void showComponents( void );
	private:
		void parseStatement( void ) throws_( mfc::CRuntimeException );
		void extractIteratorName( void );
	public:
		const TCHAR* m_pCodeText;
		int m_length;
		TokenRange m_leadingWhiteSpace;
		TokenRange m_containerType;
		TokenRange m_objectType;
		TokenRange m_container;

		bool m_isConst;
		bool m_isMfcList;
		const TCHAR* m_objectSelector;
		CString m_objectName;
		LibraryType m_libraryType;
	};
}


#endif // ContainerComponents_h

#ifndef IterationSlices_h
#define IterationSlices_h
#pragma once


#include "TokenRange.h"
#include "utl/RuntimeException.h"


namespace code
{
	struct CIterationSlices
	{
		enum LibraryType { STL, MFC };

		CIterationSlices( void );

		void ParseStatement( const std::tstring& codeText ) throws_( mfc::CRuntimeException );
	private:
		void Reset( const std::tstring& codeText );
		void ExtractIteratorName( void );

		void Trace( void );
	private:
		std::tstring m_codeText;
		const TCHAR* m_pCodeText;
	public:
		TokenRange m_leadingWhiteSpace;
		TokenRange m_containerType;
		TokenRange m_containerName;
		TokenRange m_valueType;
		std::tstring m_iteratorName;	// "Item" for "m_items"

		bool m_isConst;
		bool m_isMfcList;
		const TCHAR* m_pObjSelOp;		// either "." or "->"
		LibraryType m_libraryType;
	};
}


#endif // IterationSlices_h

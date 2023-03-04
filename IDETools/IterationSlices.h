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

		void ParseCode( const std::tstring& codeText ) throws_( CRuntimeException );
	private:
		void Reset( const std::tstring& codeText );
		void ExtractIteratorName( void );

		void Trace( void );

		static bool IsMfcList( const std::tstring& containerType );
		static bool IsMfcContainer( const std::tstring& containerType );
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

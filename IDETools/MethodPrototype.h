#ifndef MethodPrototype_h
#define MethodPrototype_h
#pragma once

#include "TokenRange.h"


namespace code
{
	struct CMethodPrototype
	{
		CMethodPrototype( void ) {}

		virtual void SplitMethod( const std::tstring& proto );

		std::tstring FormatInfo( void ) const;
	private:
		void Reset( const std::tstring& proto );
	private:
		std::tstring m_proto;
	public:
		TokenRange m_templateDecl;			// e.g. "template< typename PathType, typename ObjectType >"
		TokenRange m_inlineModifier;		// e.g. "inline"
		TokenRange m_returnType;			// e.g. "std::pair<ObjectType*, cache::TStatusFlags>"
		TokenRange m_qualifiedMethod;		// e.g. "CCacheLoader<PathType, ObjectType>::Acquire"
		TokenRange m_functionName;			// e.g. "Acquire" or "operator!="
		TokenRange m_classQualifier;		// e.g. "CCacheLoader<PathType, ObjectType>::"
		TokenRange m_argList;				// e.g. "( const PathType& pathKey )"
		TokenRange m_postArgListSuffix;		// e.g. " const" - excluding the terminating line-end
	};
}


#endif // MethodPrototype_h

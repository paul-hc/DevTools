#ifndef StreamStdTypes_h
#define StreamStdTypes_h
#pragma once


namespace stream
{
	// prevent interference due to implicit type conversions in constructors of CPoint, CSize, etc;
	// usage in code: declare in local scope 'use namespace stream;'
	//
	inline std::tostream& operator<<( std::tostream& os, const CPoint& point ) { return os << _T("(") << point.x << _T(", ") << point.y << _T(")"); }
	inline std::tostream& operator<<( std::tostream& os, const CSize& size ) { return os << _T("(") << size.cx << _T("x") << size.cy << _T(")"); }
	inline std::tostream& operator<<( std::tostream& os, const CRect& rect ) { return os << _T("[origin=") << rect.TopLeft() << _T(" size=") << rect.Size() << _T("]"); }
}


#endif // StreamStdTypes_h

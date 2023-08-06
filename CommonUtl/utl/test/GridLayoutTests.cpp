
#include "pch.h"

#ifdef USE_UT		// no UT code in release builds
#include "test/GridLayoutTests.h"
#include "StringUtilities.h"
//#include <iostream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ut
{
	struct CGrid
	{
		CGrid( size_t count, size_t columnCount )
			: m_count( count )
			, m_columnCount( columnCount )
		{
			REQUIRE( m_count != 0 && m_columnCount != 0 );

			m_rowCount = m_count / m_columnCount + ( 0 == ( m_count % m_columnCount ) ? 0 : 1 );
		}

		void GetRowColumn( OUT size_t* pRow, OUT size_t* pColumn, size_t gridPos ) const
		{
			REQUIRE( gridPos < m_count && m_rowCount != 0 );

			*pRow = gridPos % m_rowCount;
			*pColumn = gridPos / m_rowCount;
		}

		size_t ToIndex( size_t row, size_t column ) const
		{
			REQUIRE( row < m_rowCount && column < m_columnCount );
			return column * m_rowCount + row;
		}

		size_t ToTransposedIndex( size_t row, size_t column ) const
		{
			REQUIRE( row < m_rowCount && column < m_columnCount );
			return row * m_columnCount + column;
		}

		size_t ToTransposedIndex( size_t gridPos ) const
		{
			size_t row, column;
			GetRowColumn( &row, &column, gridPos );

			return ToTransposedIndex( row, column );
		}
	public:
		size_t m_count;
		size_t m_columnCount;
		size_t m_rowCount;
	};


	// grid layout helpers

	std::string FormatTopDown( const CGrid& grid, const std::string& src )
	{
		std::vector<std::string> lines;

		for ( size_t row = 0; row != grid.m_rowCount; ++row )
		{
			std::string text;

			for ( size_t column = 0; column != grid.m_columnCount; ++column )
			{
				if ( !text.empty() )
					text += "  ";

				size_t i = grid.ToIndex( row, column );
				text += str::Format( "[r=%d, c=%d]=%c", row, column, i < grid.m_count ? src[i] : '?' );
			}

			lines.push_back( text );
		}
		return str::Join( lines, "\n" );
	}

	std::string FormatLeftRight( const CGrid& grid, const std::string& src )
	{
		std::vector<std::string> lines;

		for ( size_t column = 0; column != grid.m_columnCount; ++column )
		{
			std::string text;

			for ( size_t row = 0; row != grid.m_rowCount; ++row )
			{
				if ( !text.empty() )
					text += "  ";

				size_t i = grid.ToIndex( row, column );
				text += str::Format( "[r=%d, c=%d]=%c", row, column, i < grid.m_count ? src[i] : '?' );
			}

			lines.push_back( text );
		}
		return str::Join( lines, "\n" );
	}

	std::string FormatToptLeftByIndex( const CGrid& grid, const std::string& src )
	{
		std::vector<std::string> lines( grid.m_rowCount );

		for ( size_t i = 0; i != grid.m_count; ++i )
		{
			size_t row, column;
			grid.GetRowColumn( &row, &column, i );

			std::string& tText = lines[row];
			if ( !tText.empty() )
				tText += "  ";

			size_t pos = grid.ToIndex( row, column );

			tText += str::Format( "[i=%d, pos=%d]=%c", i, pos, pos < grid.m_count ? src[pos] : '?' );
		}
		return str::Join( lines, "\n" );
	}

	std::string FormatTransposedByIndex( const CGrid& grid, const std::string& src )
	{
		std::vector<std::string> lines( grid.m_rowCount );

		for ( size_t i = 0; i != grid.m_count; ++i )
		{
			size_t row, column;
			grid.GetRowColumn( &row, &column, i );

			std::string& tText = lines[row];
			if ( !tText.empty() )
				tText += "  ";

			size_t pos = grid.ToTransposedIndex( row, column );

			tText += str::Format( "[i=%d => pos=%d]=%c", i, pos, pos < grid.m_count ? src[pos] : '?' );

			ASSERT_EQUAL( pos, grid.ToTransposedIndex( i ) );
		}
		return str::Join( lines, "\n" );
	}
}


CGridLayoutTests::CGridLayoutTests( void )
{
	ut::CTestSuite::Instance().RegisterTestCase( this );		// self-registration
}

CGridLayoutTests& CGridLayoutTests::Instance( void )
{
	static CGridLayoutTests s_testCase;
	return s_testCase;
}

void CGridLayoutTests::TestGridLayout( void )
{
	std::string src = "ABCDE";
	const ut::CGrid grid( src.length(), 3 );

	ASSERT_EQUAL_SWAP( ut::FormatTopDown( grid, src ), "\
[r=0, c=0]=A  [r=0, c=1]=C  [r=0, c=2]=E\n\
[r=1, c=0]=B  [r=1, c=1]=D  [r=1, c=2]=?\
" );
	//std::cout << std::endl << ut::FormatTopDown( grid, src ) << std::endl;

	ASSERT_EQUAL_SWAP( ut::FormatLeftRight( grid, src ), "\
[r=0, c=0]=A  [r=1, c=0]=B\n\
[r=0, c=1]=C  [r=1, c=1]=D\n\
[r=0, c=2]=E  [r=1, c=2]=?\
" );
	//std::cout << std::endl << ut::FormatLeftRight( grid, src ) << std::endl;
}

void CGridLayoutTests::TestTransposeGridLayout( void )
{
	std::string src = "ABCDE";
	const ut::CGrid grid( src.length(), 3 );

	ASSERT_EQUAL_SWAP( ut::FormatToptLeftByIndex( grid, src ), "\
[i=0, pos=0]=A  [i=2, pos=2]=C  [i=4, pos=4]=E\n\
[i=1, pos=1]=B  [i=3, pos=3]=D\
" );
	//std::cout << std::endl << ut::FormatToptLeftByIndex( grid, src ) << std::endl;

	ASSERT_EQUAL_SWAP( ut::FormatTransposedByIndex( grid, src ), "\
[i=0 => pos=0]=A  [i=2 => pos=1]=B  [i=4 => pos=2]=C\n\
[i=1 => pos=3]=D  [i=3 => pos=4]=E\
" );
	//std::cout << std::endl << ut::FormatTransposedByIndex( grid, src ) << std::endl;
}


void CGridLayoutTests::Run( void )
{
	RUN_TEST( TestGridLayout );
	RUN_TEST( TestTransposeGridLayout );
}


#endif //USE_UT

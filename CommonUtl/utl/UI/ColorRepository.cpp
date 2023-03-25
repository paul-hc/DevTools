
#include "stdafx.h"
#include "ColorRepository.h"
#include "Color.h"
#include "utl/Range.h"
#include "utl/StringUtilities.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ui
{
	bool HasLuminanceVariation( COLORREF color, int startPct /*= 10*/, int endPct /*= 100*/ )
	{
		if ( !ui::IsActualColor( color ) )
			color = ui::EvalColor( color );

		Range<COLORREF> colorRange( color );

		ui::AdjustLuminance( colorRange.m_start, startPct );
		ui::AdjustLuminance( colorRange.m_end, endPct );
		return !colorRange.IsEmpty();
	}

	bool HasSaturationVariation( COLORREF color, int startPct /*= 10*/, int endPct /*= 100*/ )
	{
		if ( !ui::IsActualColor( color ) )
			color = ui::EvalColor( color );

		Range<COLORREF> colorRange( color );

		ui::AdjustSaturation( colorRange.m_start, startPct );
		ui::AdjustSaturation( colorRange.m_end, endPct );

		if ( colorRange.IsEmpty() )
			return false;

		Range<CHslColor> hslRange( colorRange.m_start, colorRange.m_end );

		enum { MinVariationPct = 5 };
		unsigned int variationPct = (unsigned int)( abs( hslRange.m_end.m_saturation - hslRange.m_start.m_saturation ) * 100 );

		return variationPct >= MinVariationPct;
	}
}


// CColorEntry implementation

CColorEntry::CColorEntry( COLORREF color, const char* pLiteral )
	: m_color( color )
	, m_name( word::ToSpacedWordBreaks( str::FromAnsi( pLiteral ).c_str() ) )
{
}


// CColorTable implementation

CColorTable::CColorTable( size_t rowCount, UINT baseCmdId )
	: m_rowCount( rowCount )
	, m_baseCmdId( baseCmdId )
{
	ASSERT( m_baseCmdId != 0 );
}

UINT CColorTable::GetCmdIdAt( size_t index ) const
{
	ASSERT( index < m_colors.size() );
	return m_baseCmdId + static_cast<UINT>( index );
}

const CColorEntry* CColorTable::FindColor( COLORREF rawColor ) const
{
	for ( std::vector<CColorEntry>::const_iterator itColor = m_colors.begin(); itColor != m_colors.end(); ++itColor )
		if ( rawColor == itColor->m_color )
			return &*itColor;

	return nullptr;
}

size_t CColorTable::FindCmdIndex( UINT cmdId ) const
{
	size_t foundIndex = cmdId - m_baseCmdId;
	return foundIndex < m_colors.size() ? foundIndex : utl::npos;
}

CColorTable* CColorTable::MakeShades( size_t shadesCount, COLORREF selColor )
{
	if ( 0 == shadesCount || ui::IsUndefinedColor( selColor ) )
		return nullptr;

	selColor = ui::EvalColor( selColor );

	CColorTable* pShadesTable = new CColorTable( shadesCount, CColorRepository::ShadesTableBaseId );

	static const Range<ui::TPercent> s_pctRange( 10, 100 );			// [10% to 100%]
	std::vector<ui::TPercent> percentages;
	const ui::TPercent pctStep = ( s_pctRange.m_end - s_pctRange.m_start ) / ( (int)shadesCount - 1 );

	percentages.reserve( shadesCount );
	for ( unsigned int i = 0; i != shadesCount; ++i )
		percentages.push_back( s_pctRange.m_start + i * pctStep );

	static std::vector<std::tstring> s_shadeTags;
	if ( s_shadeTags.empty() )
		str::Split( s_shadeTags, _T("Lightness|Saturation"), _T("|") );

	enum ShadeTag { Lightness, Saturation };

	if ( ui::HasLuminanceVariation( selColor, percentages[ 0 ], percentages[ 1 ] ) )			// some variation?
		for ( size_t i = 0; i != percentages.size(); ++i )
		{
			COLORREF lighter = selColor;
			ui::AdjustLuminance( lighter, percentages[ i ] );

			CColorEntry colorEntry( lighter, str::Format( _T("%s +%d%%"), s_shadeTags[ Lightness ].c_str(), percentages[i] ) );
			pShadesTable->m_colors.push_back( colorEntry );
		}

	if ( ui::HasLuminanceVariation( selColor, -percentages[ 0 ], -percentages[ 1 ] ) )		// some variation?
		for ( unsigned int i = 0; i != shadesCount; ++i )
		{
			COLORREF darker = selColor;
			ui::AdjustLuminance( darker, -percentages[ i ] );

			CColorEntry colorEntry( darker, str::Format( _T("%s %d%%"), s_shadeTags[ Lightness ].c_str(), -percentages[i] ) );
			pShadesTable->m_colors.push_back( colorEntry );
		}

	if ( ui::HasSaturationVariation( selColor, -percentages[ 0 ], -percentages[ 1 ] ) )		// some variation?
		for ( unsigned int i = 0; i != shadesCount; ++i )
		{
			COLORREF desaturated = selColor;
			ui::AdjustLuminance( desaturated, -percentages[ i ] );

			CColorEntry colorEntry( desaturated, str::Format( _T("%s %d%%"), s_shadeTags[ Saturation ].c_str(), -percentages[i] ) );
			pShadesTable->m_colors.push_back( colorEntry );
		}

	return pShadesTable;
}


// CColorRepository implementation

CColorRepository::CColorRepository( void )
{
	m_colorTables.push_back( GetTable_Standard() );
	m_colorTables.push_back( GetTable_Custom() );
	m_colorTables.push_back( GetHtmlTable() );
	m_colorTables.push_back( GetPaintTable() );
	m_colorTables.push_back( GetX11Table() );
}

CColorRepository::~CColorRepository()
{
}

const CColorRepository& CColorRepository::Instance( void )
{
	static const CColorRepository repository;
	return repository;
}

CColorTable* CColorRepository::GetStockColorTable( void )
{
	return GetHtmlTable();
}

const CColorEntry* CColorRepository::FindEntry( COLORREF rawColor ) const
{
	if ( ui::IsUndefinedColor( rawColor ) )
		return nullptr;			// no entry for NULL color

	if ( ui::IsSysColor( rawColor ) )
		if ( const CColorEntry* pFound = GetSystemTable()->FindColor( rawColor ) )
			return pFound;

	COLORREF color = ui::EvalColor( rawColor );

	for ( std::vector<CColorTable*>::const_iterator itTable = m_colorTables.begin(); itTable != m_colorTables.end(); ++itTable )
		if ( const CColorEntry* pFound = ( *itTable )->FindColor( color ) )
			return pFound;

	return nullptr;
}

void CColorRepository::QueryTablesWithColor( OUT std::vector<CColorTable*>& rColorTables, COLORREF rawColor ) const
{
	if ( ui::IsUndefinedColor( rawColor ) )
		return;			// no entry for NULL color

	if ( ui::IsSysColor( rawColor ) )
		if ( const CColorEntry* pFound = GetSystemTable()->FindColor( rawColor ) )
			rColorTables.push_back( GetSystemTable() );

	COLORREF color = ui::EvalColor( rawColor );

	for ( std::vector< CColorTable* >::const_iterator itTable = m_colorTables.begin(); itTable != m_colorTables.end(); ++itTable )
		if ( const CColorEntry* pFound = ( *itTable )->FindColor( color ) )
			rColorTables.push_back( *itTable );
}

CColorTable* CColorRepository::GetTable_Standard( void )
{
	static CColorTable table( 5, BaseId_Standard );

	if ( table.m_colors.empty() )
	{
		static const struct { COLORREF color; const TCHAR* pName; } colors[] =
		{	// non-translatable color names
			// column 0
			{ color::Black,          _T("Black") },
			{ color::DarkRed,        _T("Dark Red") },
			{ color::Red,            _T("Red") },
			{ color::Magenta,        _T("Magenta") },
			{ color::Rose,           _T("Rose") },
			// column 1
			{ color::Brown,          _T("Brown") },
			{ color::Orange,         _T("Orange") },
			{ color::LightOrange,    _T("Light Orange") },
			{ color::Gold,           _T("Gold") },
			{ color::Tan,            _T("Tan") },
			// column 2
			{ color::OliveGreen,     _T("Olive Green") },
			{ color::DarkYellow,     _T("Dark Yellow") },
			{ color::Lime,           _T("Lime") },
			{ color::Yellow,         _T("Yellow") },
			{ color::LightYellow,    _T("Light Yellow") },
			// column 3
			{ color::DarkGreen,      _T("Dark Green") },
			{ color::Green,          _T("Green") },
			{ color::SeaGreen,       _T("Sea Green") },
			{ color::BrightGreen,    _T("Bright Green") },
			{ color::LightGreen,     _T("Light Green ") },
			// column 4
			{ color::DarkTeal,       _T("Dark Teal") },
			{ color::Teal,           _T("Teal") },
			{ color::Aqua,           _T("Aqua") },
			{ color::Turquoise,      _T("Turquoise") },
			{ color::LightTurquoise, _T("Light Turquoise") },
			// column 5
			{ color::DarkBlue,       _T("Dark Blue") },
			{ color::Blue,           _T("Blue") },
			{ color::LightBlue,      _T("Light Blue") },
			{ color::SkyBlue,        _T("Sky Blue") },
			{ color::PaleBlue,       _T("Pale Blue") },
			// column 6
			{ color::Indigo,         _T("Indigo") },
			{ color::BlueGray,       _T("Blue-Gray") },
			{ color::Violet,         _T("Violet") },
			{ color::Plum,           _T("Plum") },
			{ color::Lavender,       _T("Lavender") },
			// column 7
			{ color::Grey80,         _T("Grey 80%") },
			{ color::Grey40,         _T("Grey 40%") },
			{ color::Grey50,         _T("Grey 50%") },
			{ color::Grey25,         _T("Grey 25%") },
			{ color::White,          _T("White") }
		};

		table.m_colors.reserve( COUNT_OF( colors ) );
		for ( unsigned int i = 0; i != COUNT_OF( colors ); ++i )
			table.m_colors.push_back( CColorEntry( colors[ i ].color, colors[ i ].pName ) );
	}

	return &table;
}

CColorTable* CColorRepository::GetTable_Custom( void )
{
	static CColorTable table( 1, BaseId_Custom );

	if ( table.m_colors.empty() )
	{
		static const struct { COLORREF color; const TCHAR* pName; } colors[] =
		{
			// column 0
			{ color::VeryDarkGrey,   _T("Darker Shadow Grey") },
			{ color::LightGreenish,  _T("Light Greenish") },
			{ color::SpringGreen,    _T("Spring Green") },
			{ color::NeonGreen,      _T("Neon Green") },
			{ color::AzureBlue,      _T("Azure Blue") },
			{ color::ScarletRed,     _T("Scarlet Red") },
			{ color::Salmon,         _T("Salmon") },
			{ color::Pink,           _T("Pink") },
			{ color::SolidOrange,    _T("Orange (solid)") },
			{ color::Amber,          _T("Amber") },
			{ color::PaleYellow,     _T("Pale Yellow") },
			{ color::GhostWhite,     _T("Ghost White") }
		};

		table.m_colors.reserve( COUNT_OF( colors ) );
		for ( unsigned int i = 0; i != COUNT_OF( colors ); ++i )
			table.m_colors.push_back( CColorEntry( colors[ i ].color, colors[ i ].pName ) );
	}

	return &table;
}

CColorTable* CColorRepository::GetTable_Status( void )
{
	return nullptr;
}

CColorTable* CColorRepository::GetHtmlTable( void )
{
	static CColorTable table( 15, HtmlTableBaseId );

	if ( table.m_colors.empty() )
	{
		static const struct { COLORREF color; const TCHAR* pName; } colors[] =
		{
			{ color::html::Black,                _T("Black") },
			{ color::html::Gray0,                _T("Gray 0") },
			{ color::html::Gray18,               _T("Gray 18") },
			{ color::html::Gray21,               _T("Gray 21") },
			{ color::html::Gray23,               _T("Gray 23") },
			{ color::html::Gray24,               _T("Gray 24") },
			{ color::html::Gray25,               _T("Gray 25") },
			{ color::html::Gray26,               _T("Gray 26") },
			{ color::html::Gray27,               _T("Gray 27") },
			{ color::html::Gray28,               _T("Gray 28") },
			{ color::html::Gray29,               _T("Gray 29") },
			{ color::html::Gray30,               _T("Gray 30") },
			{ color::html::Gray31,               _T("Gray 31") },
			{ color::html::Gray32,               _T("Gray 32") },
			{ color::html::Gray34,               _T("Gray 34") },
			{ color::html::Gray35,               _T("Gray 35") },
			{ color::html::Gray36,               _T("Gray 36") },
			{ color::html::Gray37,               _T("Gray 37") },
			{ color::html::Gray38,               _T("Gray 38") },
			{ color::html::Gray39,               _T("Gray 39") },
			{ color::html::Gray40,               _T("Gray 40") },
			{ color::html::Gray41,               _T("Gray 41") },
			{ color::html::Gray42,               _T("Gray 42") },
			{ color::html::Gray43,               _T("Gray 43") },
			{ color::html::Gray44,               _T("Gray 44") },
			{ color::html::Gray45,               _T("Gray 45") },
			{ color::html::Gray46,               _T("Gray 46") },
			{ color::html::Gray47,               _T("Gray 47") },
			{ color::html::Gray48,               _T("Gray 48") },
			{ color::html::Gray49,               _T("Gray 49") },
			{ color::html::Gray50,               _T("Gray 50") },
			{ color::html::Gray,                 _T("Gray") },
			{ color::html::SlateGray4,           _T("Slate Gray 4") },
			{ color::html::SlateGray,            _T("Slate Gray") },
			{ color::html::LightSteelBlue4,      _T("Light Steel Blue 4") },
			{ color::html::LightSlateGray,       _T("Light Slate Gray") },
			{ color::html::CadetBlue4,           _T("Cadet Blue 4") },
			{ color::html::DarkSlateGray4,       _T("Dark Slate Gray 4") },
			{ color::html::Thistle4,             _T("Thistle 4") },
			{ color::html::MediumSlateBlue,      _T("Medium Slate Blue") },
			{ color::html::MediumPurple4,        _T("Medium Purple 4") },
			{ color::html::MidnightBlue,         _T("Midnight Blue") },
			{ color::html::DarkSlateBlue,        _T("Dark Slate Blue") },
			{ color::html::DarkSlateGray,        _T("Dark Slate Gray") },
			{ color::html::DimGray,              _T("Dim Gray") },
			{ color::html::CornflowerBlue,       _T("Cornflower Blue") },
			{ color::html::RoyalBlue4,           _T("Royal Blue 4") },
			{ color::html::SlateBlue4,           _T("Slate Blue 4") },
			{ color::html::RoyalBlue,            _T("Royal Blue") },
			{ color::html::RoyalBlue1,           _T("Royal Blue 1") },
			{ color::html::RoyalBlue2,           _T("Royal Blue 2") },
			{ color::html::RoyalBlue3,           _T("Royal Blue 3") },
			{ color::html::DeepSkyBlue,          _T("Deep Sky Blue") },
			{ color::html::DeepSkyBlue2,         _T("Deep Sky Blue 2") },
			{ color::html::SlateBlue,            _T("Slate Blue") },
			{ color::html::DeepSkyBlue3,         _T("Deep Sky Blue 3") },
			{ color::html::DeepSkyBlue4,         _T("Deep Sky Blue 4") },
			{ color::html::DodgerBlue,           _T("Dodger Blue") },
			{ color::html::DodgerBlue2,          _T("Dodger Blue 2") },
			{ color::html::DodgerBlue3,          _T("Dodger Blue 3") },
			{ color::html::DodgerBlue4,          _T("Dodger Blue 4") },
			{ color::html::SteelBlue4,           _T("Steel Blue 4") },
			{ color::html::SteelBlue,            _T("Steel Blue") },
			{ color::html::SlateBlue2,           _T("Slate Blue 2") },
			{ color::html::Violet,               _T("Violet") },
			{ color::html::MediumPurple3,        _T("Medium Purple 3") },
			{ color::html::MediumPurple,         _T("Medium Purple") },
			{ color::html::MediumPurple2,        _T("Medium Purple 2") },
			{ color::html::MediumPurple1,        _T("Medium Purple 1") },
			{ color::html::LightSteelBlue,       _T("Light Steel Blue") },
			{ color::html::SteelBlue3,           _T("Steel Blue 3") },
			{ color::html::SteelBlue2,           _T("Steel Blue 2") },
			{ color::html::SteelBlue1,           _T("Steel Blue 1") },
			{ color::html::SkyBlue3,             _T("Sky Blue 3") },
			{ color::html::SkyBlue4,             _T("Sky Blue 4") },
			{ color::html::SlateBlue3,           _T("Slate Blue 3") },
			{ color::html::SlateBlue5,           _T("Slate Blue 5") },
			{ color::html::SlateGray3,           _T("Slate Gray 3") },
			{ color::html::VioletRed,            _T("Violet Red") },
			{ color::html::VioletRed1,           _T("Violet Red 1") },
			{ color::html::VioletRed2,           _T("Violet Red 2") },
			{ color::html::DeepPink,             _T("Deep Pink") },
			{ color::html::DeepPink2,            _T("Deep Pink 2") },
			{ color::html::DeepPink3,            _T("Deep Pink 3") },
			{ color::html::DeepPink4,            _T("Deep Pink 4") },
			{ color::html::MediumVioletRed,      _T("Medium Violet Red") },
			{ color::html::VioletRed3,           _T("Violet Red 3") },
			{ color::html::Firebrick,            _T("Firebrick") },
			{ color::html::VioletRed4,           _T("Violet Red 4") },
			{ color::html::Maroon4,              _T("Maroon 4") },
			{ color::html::Maroon,               _T("Maroon") },
			{ color::html::Maroon3,              _T("Maroon 3") },
			{ color::html::Maroon2,              _T("Maroon 2") },
			{ color::html::Maroon1,              _T("Maroon 1") },
			{ color::html::Magenta,              _T("Magenta") },
			{ color::html::Magenta1,             _T("Magenta 1") },
			{ color::html::Magenta2,             _T("Magenta 2") },
			{ color::html::Magenta3,             _T("Magenta 3") },
			{ color::html::MediumOrchid,         _T("Medium Orchid") },
			{ color::html::MediumOrchid1,        _T("Medium Orchid 1") },
			{ color::html::MediumOrchid2,        _T("Medium Orchid 2") },
			{ color::html::MediumOrchid3,        _T("Medium Orchid 3") },
			{ color::html::MediumOrchid4,        _T("Medium Orchid 4") },
			{ color::html::Purple,               _T("Purple") },
			{ color::html::Purple1,              _T("Purple 1") },
			{ color::html::Purple2,              _T("Purple 2") },
			{ color::html::Purple3,              _T("Purple 3") },
			{ color::html::Purple4,              _T("Purple 4") },
			{ color::html::DarkOrchid4,          _T("Dark Orchid 4") },
			{ color::html::DarkOrchid,           _T("Dark Orchid") },
			{ color::html::DarkViolet,           _T("Dark Violet") },
			{ color::html::DarkOrchid3,          _T("Dark Orchid 3") },
			{ color::html::DarkOrchid2,          _T("Dark Orchid 2") },
			{ color::html::DarkOrchid1,          _T("Dark Orchid 1") },
			{ color::html::Plum4,                _T("Plum 4") },
			{ color::html::PaleVioletRed,        _T("Pale Violet Red") },
			{ color::html::PaleVioletRed1,       _T("Pale Violet Red 1") },
			{ color::html::PaleVioletRed2,       _T("Pale Violet Red 2") },
			{ color::html::PaleVioletRed3,       _T("Pale Violet Red 3") },
			{ color::html::PaleVioletRed4,       _T("Pale Violet Red 4") },
			{ color::html::Plum,                 _T("Plum") },
			{ color::html::Plum1,                _T("Plum 1") },
			{ color::html::Plum2,                _T("Plum 2") },
			{ color::html::Plum3,                _T("Plum 3") },
			{ color::html::Thistle,              _T("Thistle") },
			{ color::html::Thistle3,             _T("Thistle 3") },
			{ color::html::LavenderBlush2,       _T("Lavender Blush 2") },
			{ color::html::LavenderBlush3,       _T("Lavender Blush 3") },
			{ color::html::Thistle2,             _T("Thistle 2") },
			{ color::html::Thistle1,             _T("Thistle 1") },
			{ color::html::Lavender,             _T("Lavender") },
			{ color::html::LavenderBlush,        _T("Lavender Blush") },
			{ color::html::LightSteelBlue1,      _T("Light Steel Blue 1") },
			{ color::html::LightBlue,            _T("Light Blue") },
			{ color::html::LightBlue1,           _T("Light Blue 1") },
			{ color::html::LightCyan,            _T("Light Cyan") },
			{ color::html::SlateGray1,           _T("Slate Gray 1") },
			{ color::html::SlateGray2,           _T("Slate Gray 2") },
			{ color::html::LightSteelBlue2,      _T("Light Steel Blue 2") },
			{ color::html::Turquoise1,           _T("Turquoise 1") },
			{ color::html::Cyan,                 _T("Cyan") },
			{ color::html::Cyan1,                _T("Cyan 1") },
			{ color::html::Cyan2,                _T("Cyan 2") },
			{ color::html::Turquoise2,           _T("Turquoise 2") },
			{ color::html::MediumTurquoise,      _T("Medium Turquoise") },
			{ color::html::Turquoise,            _T("Turquoise") },
			{ color::html::DarkSlateGray1,       _T("Dark Slate Gray 1") },
			{ color::html::DarkSlateGray2,       _T("Dark Slate Gray 2") },
			{ color::html::DarkSlateGray3,       _T("Dark Slate Gray 3") },
			{ color::html::Cyan3,                _T("Cyan 3") },
			{ color::html::Turquoise3,           _T("Turquoise 3") },
			{ color::html::CadetBlue3,           _T("Cadet Blue 3") },
			{ color::html::PaleTurquoise3,       _T("Pale Turquoise 3") },
			{ color::html::LightBlue2,           _T("Light Blue 2") },
			{ color::html::DarkTurquoise,        _T("Dark Turquoise") },
			{ color::html::Cyan4,                _T("Cyan 4") },
			{ color::html::LightSeaGreen,        _T("Light Sea Green") },
			{ color::html::LightSkyBlue,         _T("Light Sky Blue") },
			{ color::html::LightSkyBlue2,        _T("Light Sky Blue 2") },
			{ color::html::LightSkyBlue3,        _T("Light Sky Blue 3") },
			{ color::html::SkyBlue5,             _T("Sky Blue 5") },
			{ color::html::SkyBlue6,             _T("Sky Blue 6") },
			{ color::html::LightSkyBlue4,        _T("Light Sky Blue 4") },
			{ color::html::SkyBlue,              _T("Sky Blue") },
			{ color::html::LightSlateBlue,       _T("Light Slate Blue") },
			{ color::html::LightCyan2,           _T("Light Cyan 2") },
			{ color::html::LightCyan3,           _T("Light Cyan 3") },
			{ color::html::LightCyan4,           _T("Light Cyan 4") },
			{ color::html::LightBlue3,           _T("Light Blue 3") },
			{ color::html::LightBlue4,           _T("Light Blue 4") },
			{ color::html::PaleTurquoise4,       _T("Pale Turquoise 4") },
			{ color::html::DarkSeaGreen4,        _T("Dark Sea Green 4") },
			{ color::html::MediumAquamarine,     _T("Medium Aquamarine") },
			{ color::html::MediumSeaGreen,       _T("Medium Sea Green") },
			{ color::html::SeaGreen,             _T("Sea Green") },
			{ color::html::DarkGreen,            _T("Dark Green") },
			{ color::html::SeaGreen4,            _T("Sea Green 4") },
			{ color::html::ForestGreen,          _T("Forest Green") },
			{ color::html::MediumForestGreen,    _T("Medium Forest Green") },
			{ color::html::SpringGreen4,         _T("Spring Green 4") },
			{ color::html::DarkOliveGreen4,      _T("Dark Olive Green 4") },
			{ color::html::Chartreuse4,          _T("Chartreuse 4") },
			{ color::html::Green4,               _T("Green 4") },
			{ color::html::MediumSpringGreen,    _T("Medium Spring Green") },
			{ color::html::SpringGreen,          _T("Spring Green 2") },
			{ color::html::LimeGreen,            _T("Lime Green") },
			{ color::html::SpringGreen3,         _T("Spring Green") },
			{ color::html::DarkSeaGreen,         _T("Dark Sea Green") },
			{ color::html::DarkSeaGreen3,        _T("Dark Sea Green 3") },
			{ color::html::Green3,               _T("Green 3") },
			{ color::html::Chartreuse3,          _T("Chartreuse 3") },
			{ color::html::YellowGreen,          _T("Yellow Green") },
			{ color::html::SpringGreen5,         _T("Spring Green 3") },
			{ color::html::SeaGreen3,            _T("Sea Green 3") },
			{ color::html::SpringGreen2,         _T("Spring Green 2") },
			{ color::html::SpringGreen1,         _T("Spring Green 1") },
			{ color::html::SeaGreen2,            _T("Sea Green 2") },
			{ color::html::SeaGreen1,            _T("Sea Green 1") },
			{ color::html::DarkSeaGreen2,        _T("Dark Sea Green 2") },
			{ color::html::DarkSeaGreen1,        _T("Dark Sea Green 1") },
			{ color::html::Green,                _T("Green") },
			{ color::html::LawnGreen,            _T("Lawn Green") },
			{ color::html::Green1,               _T("Green 1") },
			{ color::html::Green2,               _T("Green 2") },
			{ color::html::Chartreuse2,          _T("Chartreuse 2") },
			{ color::html::Chartreuse,           _T("Chartreuse") },
			{ color::html::GreenYellow,          _T("Green Yellow") },
			{ color::html::DarkOliveGreen1,      _T("Dark Olive Green 1") },
			{ color::html::DarkOliveGreen2,      _T("Dark Olive Green 2") },
			{ color::html::DarkOliveGreen3,      _T("Dark Olive Green 3") },
			{ color::html::Yellow,               _T("Yellow") },
			{ color::html::Yellow1,              _T("Yellow 1") },
			{ color::html::Khaki1,               _T("Khaki 1") },
			{ color::html::Khaki2,               _T("Khaki 2") },
			{ color::html::Goldenrod,            _T("Goldenrod") },
			{ color::html::Gold2,                _T("Gold 2") },
			{ color::html::Gold1,                _T("Gold 1") },
			{ color::html::Goldenrod1,           _T("Goldenrod 1") },
			{ color::html::Goldenrod2,           _T("Goldenrod 2") },
			{ color::html::Gold,                 _T("Gold") },
			{ color::html::Gold3,                _T("Gold 3") },
			{ color::html::Goldenrod3,           _T("Goldenrod 3") },
			{ color::html::DarkGoldenrod,        _T("Dark Goldenrod") },
			{ color::html::Khaki,                _T("Khaki") },
			{ color::html::Khaki3,               _T("Khaki 3") },
			{ color::html::Khaki4,               _T("Khaki 4") },
			{ color::html::DarkGoldenrod1,       _T("Dark Goldenrod 1") },
			{ color::html::DarkGoldenrod2,       _T("Dark Goldenrod 2") },
			{ color::html::DarkGoldenrod3,       _T("Dark Goldenrod 3") },
			{ color::html::Sienna1,              _T("Sienna 1") },
			{ color::html::Sienna2,              _T("Sienna 2") },
			{ color::html::DarkOrange,           _T("Dark Orange") },
			{ color::html::DarkOrange1,          _T("Dark Orange 1") },
			{ color::html::DarkOrange2,          _T("Dark Orange 2") },
			{ color::html::DarkOrange3,          _T("Dark Orange 3") },
			{ color::html::Sienna3,              _T("Sienna 3") },
			{ color::html::Sienna,               _T("Sienna") },
			{ color::html::Sienna4,              _T("Sienna 4") },
			{ color::html::IndianRed4,           _T("Indian Red 4") },
			{ color::html::DarkOrange4,          _T("Dark Orange 4") },
			{ color::html::Salmon4,              _T("Salmon 4") },
			{ color::html::DarkGoldenrod4,       _T("Dark Goldenrod 4") },
			{ color::html::Gold4,                _T("Gold 4") },
			{ color::html::Goldenrod4,           _T("Goldenrod 4") },
			{ color::html::LightSalmon4,         _T("Light Salmon 4") },
			{ color::html::Chocolate,            _T("Chocolate") },
			{ color::html::Coral3,               _T("Coral 3") },
			{ color::html::Coral2,               _T("Coral 2") },
			{ color::html::Coral,                _T("Coral") },
			{ color::html::DarkSalmon,           _T("Dark Salmon") },
			{ color::html::Salmon1,              _T("Salmon 1") },
			{ color::html::Salmon2,              _T("Salmon 2") },
			{ color::html::Salmon3,              _T("Salmon 3") },
			{ color::html::LightSalmon3,         _T("Light Salmon 3") },
			{ color::html::LightSalmon2,         _T("Light Salmon 2") },
			{ color::html::LightSalmon,          _T("Light Salmon") },
			{ color::html::SandyBrown,           _T("Sandy Brown") },
			{ color::html::HotPink,              _T("Hot Pink") },
			{ color::html::HotPink1,             _T("Hot Pink 1") },
			{ color::html::HotPink2,             _T("Hot Pink 2") },
			{ color::html::HotPink3,             _T("Hot Pink 3") },
			{ color::html::HotPink4,             _T("Hot Pink 4") },
			{ color::html::LightCoral,           _T("Light Coral") },
			{ color::html::IndianRed1,           _T("Indian Red 1") },
			{ color::html::IndianRed2,           _T("Indian Red 2") },
			{ color::html::IndianRed3,           _T("Indian Red 3") },
			{ color::html::Red,                  _T("Red") },
			{ color::html::Red1,                 _T("Red 1") },
			{ color::html::Red2,                 _T("Red 2") },
			{ color::html::Firebrick1,           _T("Firebrick 1") },
			{ color::html::Firebrick2,           _T("Firebrick 2") },
			{ color::html::Firebrick3,           _T("Firebrick 3") },
			{ color::html::Pink,                 _T("Pink") },
			{ color::html::RosyBrown1,           _T("Rosy Brown 1") },
			{ color::html::RosyBrown2,           _T("Rosy Brown 2") },
			{ color::html::Pink2,                _T("Pink 2") },
			{ color::html::LightPink,            _T("Light Pink") },
			{ color::html::LightPink1,           _T("Light Pink 1") },
			{ color::html::LightPink2,           _T("Light Pink 2") },
			{ color::html::Pink3,                _T("Pink 3") },
			{ color::html::RosyBrown3,           _T("Rosy Brown 3") },
			{ color::html::RosyBrown,            _T("Rosy Brown") },
			{ color::html::LightPink3,           _T("Light Pink 3") },
			{ color::html::RosyBrown4,           _T("Rosy Brown 4") },
			{ color::html::LightPink4,           _T("Light Pink 4") },
			{ color::html::Pink4,                _T("Pink 4") },
			{ color::html::LavenderBlush4,       _T("Lavender Blush 4") },
			{ color::html::LightGoldenrod4,      _T("Light Goldenrod 4") },
			{ color::html::LemonChiffon4,        _T("Lemon Chiffon 4") },
			{ color::html::LemonChiffon3,        _T("Lemon Chiffon 3") },
			{ color::html::LightGoldenrod3,      _T("Light Goldenrod 3") },
			{ color::html::LightGolden2,         _T("Light Golden 2") },
			{ color::html::LightGoldenrod,       _T("Light Goldenrod") },
			{ color::html::LightGoldenrod1,      _T("Light Goldenrod 1") },
			{ color::html::BlanchedAlmond,       _T("Blanched Almond") },
			{ color::html::LemonChiffon2,        _T("Lemon Chiffon 2") },
			{ color::html::LemonChiffon,         _T("Lemon Chiffon") },
			{ color::html::LightGoldenrodYellow, _T("Light Goldenrod Yellow") },
			{ color::html::Cornsilk,             _T("Cornsilk") },
			{ color::html::White,                _T("White") }
		};

		table.m_colors.reserve( COUNT_OF( colors ) );
		for ( unsigned int i = 0; i != COUNT_OF( colors ); ++i )
			table.m_colors.push_back( CColorEntry( colors[ i ].color, colors[ i ].pName ) );
	}

	return &table;
}

CColorTable* CColorRepository::GetPaintTable( void )
{
	static CColorTable table( 12, PaintTableBaseId );

	if ( table.m_colors.empty() )
	{
		static const struct { COLORREF color; const TCHAR* pName; } colors[] =
		{
			{ color::paint::Black,               _T("Black") },
			{ color::paint::White,               _T("White") }
		};

		table.m_colors.reserve( COUNT_OF( colors ) );
		for ( unsigned int i = 0; i != COUNT_OF( colors ); ++i )
			table.m_colors.push_back( CColorEntry( colors[ i ].color, colors[ i ].pName ) );
	}

	return &table;
}

CColorTable* CColorRepository::GetX11Table( void )
{
	static CColorTable table( 10, X11TableBaseId );

	if ( table.m_colors.empty() )
	{
		static const struct { COLORREF color; const TCHAR* pName; } colors[] =
		{
			// gray colors
			{ color::x11::Black,                _T("Black") },
			{ color::x11::DarkSlateGray,        _T("Dark Slate Gray") },
			{ color::x11::SlateGray,            _T("Slate Gray") },
			{ color::x11::LightSlateGray,       _T("Light Slate Gray") },
			{ color::x11::DimGray,              _T("Dim Gray") },
			{ color::x11::Gray,                 _T("Gray") },
			{ color::x11::DarkGray,             _T("Dark Gray") },
			{ color::x11::Silver,               _T("Silver") },
			{ color::x11::LightGrey,            _T("Light Grey") },
			{ color::x11::Gainsboro,            _T("Gainsboro") },
			// red colors
			{ color::x11::IndianRed,            _T("Indian Red") },
			{ color::x11::LightCoral,           _T("Light Coral") },
			{ color::x11::Salmon,               _T("Salmon") },
			{ color::x11::DarkSalmon,           _T("Dark Salmon") },
			{ color::x11::LightSalmon,          _T("Light Salmon") },
			{ color::x11::Red,                  _T("Red") },
			{ color::x11::Crimson,              _T("Crimson") },
			{ color::x11::FireBrick,            _T("Fire Brick") },
			{ color::x11::DarkRed,              _T("Dark Red") },
			// pink colors
			{ color::x11::Pink,                 _T("Pink") },
			{ color::x11::LightPink,            _T("Light Pink") },
			{ color::x11::HotPink,              _T("Hot Pink") },
			{ color::x11::DeepPink,             _T("Deep Pink") },
			{ color::x11::MediumVioletRed,      _T("Medium Violet Red") },
			{ color::x11::PaleVioletRed,        _T("Pale Violet Red") },
			// orange colors
			{ color::x11::LightSalmon2,         _T("Light Salmon") },
			{ color::x11::Coral,                _T("Coral") },
			{ color::x11::Tomato,               _T("Tomato") },
			{ color::x11::OrangeRed,            _T("Orange Red") },
			{ color::x11::DarkOrange,           _T("Dark Orange") },
			{ color::x11::Orange,               _T("Orange") },
			// yellow colors
			{ color::x11::Gold,                 _T("Gold") },
			{ color::x11::Yellow,               _T("Yellow") },
			{ color::x11::LightYellow,          _T("Light Yellow") },
			{ color::x11::LemonChiffon,         _T("Lemon Chiffon") },
			{ color::x11::LightGoldenrodYellow, _T("Light Goldenrod Yellow") },
			{ color::x11::PapayaWhip,           _T("Papaya Whip") },
			{ color::x11::Moccasin,             _T("Moccasin") },
			{ color::x11::PeachPuff,            _T("Peach Puff") },
			{ color::x11::PaleGoldenrod,        _T("Pale Goldenrod") },
			{ color::x11::Khaki,                _T("Khaki") },
			{ color::x11::DarkKhaki,            _T("Dark Khaki") },
			// purple colors
			{ color::x11::Lavender,             _T("Lavender") },
			{ color::x11::Thistle,              _T("Thistle") },
			{ color::x11::Plum,                 _T("Plum") },
			{ color::x11::Violet,               _T("Violet") },
			{ color::x11::Orchid,               _T("Orchid") },
			{ color::x11::Fuchsia,              _T("Fuchsia") },
			{ color::x11::Magenta,              _T("Magenta") },
			{ color::x11::MediumOrchid,         _T("Medium Orchid") },
			{ color::x11::MediumPurple,         _T("Medium Purple") },
			{ color::x11::BlueViolet,           _T("Blue Violet") },
			{ color::x11::DarkViolet,           _T("Dark Violet") },
			{ color::x11::DarkOrchid,           _T("Dark Orchid") },
			{ color::x11::DarkMagenta,          _T("Dark Magenta") },
			{ color::x11::Purple,               _T("Purple") },
			{ color::x11::Indigo,               _T("Indigo") },
			{ color::x11::DarkSlateBlue,        _T("Dark Slate Blue") },
			{ color::x11::SlateBlue,            _T("Slate Blue") },
			{ color::x11::MediumSlateBlue,      _T("Medium Slate Blue") },
			// green colors
			{ color::x11::GreenYellow,          _T("Green Yellow") },
			{ color::x11::Chartreuse,           _T("Chartreuse") },
			{ color::x11::LawnGreen,            _T("Lawn Green") },
			{ color::x11::Lime,                 _T("Lime") },
			{ color::x11::LimeGreen,            _T("Lime Green") },
			{ color::x11::PaleGreen,            _T("Pale Green") },
			{ color::x11::LightGreen,           _T("Light Green") },
			{ color::x11::MediumSpringGreen,    _T("Medium Spring Green") },
			{ color::x11::SpringGreen,          _T("Spring Green") },
			{ color::x11::MediumSeaGreen,       _T("Medium Sea Green") },
			{ color::x11::SeaGreen,             _T("Sea Green") },
			{ color::x11::ForestGreen,          _T("Forest Green") },
			{ color::x11::Green,                _T("Green") },
			{ color::x11::DarkGreen,            _T("Dark Green") },
			{ color::x11::YellowGreen,          _T("Yellow Green") },
			{ color::x11::OliveDrab,            _T("Olive Drab") },
			{ color::x11::Olive,                _T("Olive") },
			{ color::x11::DarkOliveGreen,       _T("Dark Olive Green") },
			{ color::x11::MediumAquamarine,     _T("Medium Aquamarine") },
			{ color::x11::DarkSeaGreen,         _T("Dark Sea Green") },
			{ color::x11::LightSeaGreen,        _T("Light Sea Green") },
			{ color::x11::DarkCyan,             _T("Dark Cyan") },
			{ color::x11::Teal,                 _T("Teal") },
			// blue & cyan colors
			{ color::x11::Aqua,                 _T("Aqua") },
			{ color::x11::Cyan,                 _T("Cyan") },
			{ color::x11::LightCyan,            _T("Light Cyan") },
			{ color::x11::PaleTurquoise,        _T("Pale Turquoise") },
			{ color::x11::Aquamarine,           _T("Aquamarine") },
			{ color::x11::Turquoise,            _T("Turquoise") },
			{ color::x11::MediumTurquoise,      _T("Medium Turquoise") },
			{ color::x11::DarkTurquoise,        _T("Dark Turquoise") },
			{ color::x11::CadetBlue,            _T("Cadet Blue") },
			{ color::x11::SteelBlue,            _T("Steel Blue") },
			{ color::x11::LightSteelBlue,       _T("Light Steel Blue") },
			{ color::x11::PowderBlue,           _T("Powder Blue") },
			{ color::x11::LightBlue,            _T("Light Blue") },
			{ color::x11::SkyBlue,              _T("Sky Blue") },
			{ color::x11::LightSkyBlue,         _T("Light Sky Blue") },
			{ color::x11::DeepSkyBlue,          _T("Deep Sky Blue") },
			{ color::x11::DodgerBlue,           _T("Dodger Blue") },
			{ color::x11::CornflowerBlue,       _T("Cornflower Blue") },
			{ color::x11::RoyalBlue,            _T("Royal Blue") },
			{ color::x11::Blue,                 _T("Blue") },
			{ color::x11::MediumBlue,           _T("Medium Blue") },
			{ color::x11::DarkBlue,             _T("Dark Blue") },
			{ color::x11::Navy,                 _T("Navy") },
			{ color::x11::MidnightBlue,         _T("Midnight Blue") },
			// brown colors
			{ color::x11::Cornsilk,             _T("Cornsilk") },
			{ color::x11::BlanchedAlmond,       _T("Blanched Almond") },
			{ color::x11::Bisque,               _T("Bisque") },
			{ color::x11::NavajoWhite,          _T("Navajo White") },
			{ color::x11::Wheat,                _T("Wheat") },
			{ color::x11::BurlyWood,            _T("Burly Wood") },
			{ color::x11::Tan,                  _T("Tan") },
			{ color::x11::RosyBrown,            _T("Rosy Brown") },
			{ color::x11::SandyBrown,           _T("Sandy Brown") },
			{ color::x11::Goldenrod,            _T("Goldenrod") },
			{ color::x11::DarkGoldenrod,        _T("Dark Goldenrod") },
			{ color::x11::Peru,                 _T("Peru") },
			{ color::x11::Chocolate,            _T("Chocolate") },
			{ color::x11::SaddleBrown,          _T("Saddle Brown") },
			{ color::x11::Sienna,               _T("Sienna") },
			{ color::x11::Brown,                _T("Brown") },
			{ color::x11::Maroon,               _T("Maroon") },
			// white colors
			{ color::x11::MistyRose,            _T("Misty Rose") },
			{ color::x11::LavenderBlush,        _T("Lavender Blush") },
			{ color::x11::Linen,                _T("Linen") },
			{ color::x11::AntiqueWhite,         _T("Antique White") },
			{ color::x11::Ivory,                _T("Ivory") },
			{ color::x11::FloralWhite,          _T("Floral White") },
			{ color::x11::OldLace,              _T("Old Lace") },
			{ color::x11::Beige,                _T("Beige") },
			{ color::x11::Seashell,             _T("Seashell") },
			{ color::x11::WhiteSmoke,           _T("White Smoke") },
			{ color::x11::GhostWhite,           _T("Ghost White") },
			{ color::x11::AliceBlue,            _T("Alice Blue") },
			{ color::x11::Azure,                _T("Azure") },
			{ color::x11::MintCream,            _T("Mint Cream") },
			{ color::x11::Honeydew,             _T("Honeydew") },
			{ color::x11::White,                _T("White") }
		};

		table.m_colors.reserve( COUNT_OF( colors ) );
		for ( unsigned int i = 0; i != COUNT_OF( colors ); ++i )
			table.m_colors.push_back( CColorEntry( colors[ i ].color, colors[ i ].pName ) );
	}

	return &table;
}

CColorTable* CColorRepository::GetSystemTable( void )
{
	static CColorTable sysTable( 1, SysTableBaseId );

	if ( sysTable.m_colors.empty() )
	{
		static const struct { ui::TSysColorIndex sysColorIndex; const TCHAR* pName; } sysColors[] =
		{
			{ COLOR_BACKGROUND, _T("Desktop Background") },
			{ COLOR_WINDOW, _T("Window Background") },
			{ COLOR_WINDOWTEXT, _T("Window Text") },
			{ COLOR_WINDOWFRAME, _T("Window Frame") },
			{ COLOR_BTNFACE, _T("Button Face") },
			{ COLOR_BTNHIGHLIGHT, _T("Button Highlight") },
			{ COLOR_BTNSHADOW, _T("Button Shadow") },
			{ COLOR_BTNTEXT, _T("Button Text") },
			{ COLOR_GRAYTEXT, _T("Disabled Text") },
			{ COLOR_3DDKSHADOW, _T("3D Dark Shadow") },
			{ COLOR_3DLIGHT, _T("3D Light Color") },
			{ COLOR_HIGHLIGHT, _T("Selected") },
			{ COLOR_HIGHLIGHTTEXT, _T("Selected Text") },
			{ COLOR_CAPTIONTEXT, _T("Scroll Bar Arrow") },
			{ COLOR_HOTLIGHT, _T("Hot-Track") },
			{ COLOR_INFOBK, _T("Tooltip Background") },
			{ COLOR_INFOTEXT, _T("Tooltip Text") },
			{ COLOR_SCROLLBAR, _T("Scroll Bar Gray Area") },
			{ COLOR_MENU, _T("Menu Background") },
			{ COLOR_MENUBAR, _T("Flat Menu Bar") },
			{ COLOR_MENUHILIGHT, _T("Menu Highlight") },
			{ COLOR_MENUTEXT, _T("Menu Text") },
			{ COLOR_ACTIVECAPTION, _T("Active Window Title Bar") },
			{ COLOR_GRADIENTACTIVECAPTION, _T("Active Window Gradient") },
			{ COLOR_ACTIVEBORDER, _T("Active Window Border") },
			{ COLOR_INACTIVECAPTION, _T("Inactive Window Caption") },
			{ COLOR_GRADIENTINACTIVECAPTION, _T("Inactive Window Gradient") },
			{ COLOR_INACTIVEBORDER, _T("Inactive Window Border") },
			{ COLOR_INACTIVECAPTIONTEXT, _T("Inactive Window Caption Text") },
			{ COLOR_APPWORKSPACE, _T("MDI Background") },
			{ COLOR_DESKTOP, _T("Desktop Background") }
		};

		sysTable.m_colors.reserve( COUNT_OF( sysColors ) );
		for ( unsigned int i = 0; i != COUNT_OF( sysColors ); ++i )
			sysTable.m_colors.push_back( CColorEntry( ui::MakeSysColor( sysColors[ i ].sysColorIndex ), sysColors[ i ].pName ) );

		sysTable.m_rowCount = sysTable.m_colors.size() / 2;
	}

	return &sysTable;
}

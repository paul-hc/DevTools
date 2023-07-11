
#include "pch.h"
#include "ColorRepository.h"
#include "Color.h"
#include "utl/ContainerOwnership.h"
#include "utl/EnumTags.h"
#include "utl/Range.h"
#include "utl/StringUtilities.h"
#include <unordered_set>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


namespace ui
{
	const CEnumTags& GetTags_ColorTable( void )
	{
		static const CEnumTags s_tags( _T("Windows (System)|Standard|Custom|Office 2003|DirectX|HTML|X11|Shades|User") );
		return s_tags;
	}


	bool HasLuminanceVariation( COLORREF color, int startPct /*= 10*/, int endPct /*= 100*/ )
	{
		if ( !ui::IsRealColor( color ) )
			color = ui::EvalColor( color );

		Range<COLORREF> colorRange( color );

		ui::AdjustLuminance( colorRange.m_start, startPct );
		ui::AdjustLuminance( colorRange.m_end, endPct );
		return !colorRange.IsEmpty();
	}

	bool HasSaturationVariation( COLORREF color, int startPct /*= 10*/, int endPct /*= 100*/ )
	{
		if ( !ui::IsRealColor( color ) )
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

const TCHAR CColorEntry::s_fieldSep[] = _T("  ");

CColorEntry::CColorEntry( COLORREF color, const char* pLiteral )
	: m_color( color )
	, m_name( word::ToSpacedWordBreaks( str::FromAnsi( FindScopedLiteral( pLiteral ) ).c_str() ) )
	, m_pParentTable( nullptr )
{
}

const char* CColorEntry::FindScopedLiteral( const char* pScopedColorName )
{
	static const std::string s_scopeOp = "::";
	size_t posLastScope = str::FindLast<str::Case>( pScopedColorName, s_scopeOp.c_str(), s_scopeOp.length() );

	return posLastScope != std::string::npos ? ( pScopedColorName + posLastScope + s_scopeOp.length() ) : pScopedColorName;
}

std::tstring CColorEntry::FormatColor( const TCHAR fieldSep[] /*= s_fieldSep*/ ) const
{
	std::tstring colorName = m_name;

	if ( m_pParentTable != nullptr )
		stream::Tag( colorName, str::Enquote( m_pParentTable->GetTableName().c_str(), _T("["), _T("]") ), _T(" ") );

	stream::Tag( colorName, ui::FormatColor( m_color, fieldSep ), fieldSep );
	return colorName;
}


// CColorTable implementation

CColorTable::CColorTable( ui::StdColorTable tableType, UINT baseCmdId, size_t capacity, int layoutCount /*= 1*/ )
	: m_tableType( tableType )
	, m_baseCmdId( baseCmdId )
	, m_layoutCount( layoutCount )
{
	m_colors.reserve( capacity );
	ENSURE( m_baseCmdId != 0 );
	ENSURE( m_layoutCount != 0 );
}

CColorTable::~CColorTable()
{
}

const std::tstring& CColorTable::GetTableName( void ) const
{
	return ui::GetTags_ColorTable().FormatUi( m_tableType );
}

void CColorTable::Add( const CColorEntry& colorEntry )
{
	m_colors.push_back( colorEntry );
	m_colors.back().m_pParentTable = this;
}

const CColorEntry* CColorTable::FindColor( COLORREF rawColor ) const
{
	std::vector<CColorEntry>::const_iterator itFound = std::find( m_colors.begin(), m_colors.end(), rawColor );
	if ( itFound == m_colors.end() )
		return nullptr;

	return &*itFound;
}

const CColorEntry* CColorTable::FindEvaluatedColor( COLORREF color ) const
{
	std::vector<CColorEntry>::const_iterator itFound = std::find_if( m_colors.begin(), m_colors.end(), pred::HasEvalColor( color ) );
	if ( itFound == m_colors.end() )
		return nullptr;

	return &*itFound;
}

size_t CColorTable::FindCmdIndex( UINT cmdId ) const
{
	size_t foundIndex = cmdId - m_baseCmdId;
	return foundIndex < m_colors.size() ? foundIndex : utl::npos;
}

int CColorTable::GetColumnsLayout( void ) const
{
	if ( m_layoutCount > 0 )		// column layout?
		return m_layoutCount;
	else
		return static_cast<int>( m_colors.size() ) / -m_layoutCount;
}

void CColorTable::GetLayout( OUT size_t* pRowCount, OUT size_t* pColumnCount ) const
{
	if ( m_layoutCount > 0 )		// column layout?
	{
		utl::AssignPtr( pRowCount, m_colors.size() / m_layoutCount );
		utl::AssignPtr( pColumnCount, static_cast<size_t>( m_layoutCount ) );
	}
	else
	{
		utl::AssignPtr( pRowCount, static_cast<size_t>( -m_layoutCount ) );
		utl::AssignPtr( pColumnCount, m_colors.size() / -m_layoutCount );
	}
}

void CColorTable::QueryMfcColors( ui::TMFCColorArray& rColorArray ) const
{
	rColorArray.SetSize( m_colors.size() );

	for ( size_t i = 0; i != m_colors.size(); ++i )
		rColorArray[ i ] = m_colors[ i ].EvalColor();
}

void CColorTable::QueryMfcColors( ui::TMFCColorList& rColorList ) const
{
	for ( std::vector<CColorEntry>::const_iterator itColorEntry = m_colors.begin(); itColorEntry != m_colors.end(); ++itColorEntry )
		rColorList.AddTail( ui::EvalColor( itColorEntry->m_color ) );
}

CColorTable* CColorTable::MakeShadesTable( size_t shadesCount, COLORREF selColor )
{
	if ( 0 == shadesCount || ui::IsUndefinedColor( selColor ) )
		return nullptr;

	selColor = ui::EvalColor( selColor );

	CColorTable* pShadesTable = new CColorTable( ui::Shades_Colors, CColorRepository::BaseId_Shades, shadesCount );

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
			pShadesTable->Add( colorEntry );
		}

	if ( ui::HasLuminanceVariation( selColor, -percentages[ 0 ], -percentages[ 1 ] ) )		// some variation?
		for ( unsigned int i = 0; i != shadesCount; ++i )
		{
			COLORREF darker = selColor;
			ui::AdjustLuminance( darker, -percentages[ i ] );

			CColorEntry colorEntry( darker, str::Format( _T("%s %d%%"), s_shadeTags[ Lightness ].c_str(), -percentages[i] ) );
			pShadesTable->Add( colorEntry );
		}

	if ( ui::HasSaturationVariation( selColor, -percentages[ 0 ], -percentages[ 1 ] ) )		// some variation?
		for ( unsigned int i = 0; i != shadesCount; ++i )
		{
			COLORREF desaturated = selColor;
			ui::AdjustLuminance( desaturated, -percentages[ i ] );

			CColorEntry colorEntry( desaturated, str::Format( _T("%s %d%%"), s_shadeTags[ Saturation ].c_str(), -percentages[i] ) );
			pShadesTable->Add( colorEntry );
		}

	return pShadesTable;
}


// CColorStore implementation

const CColorTable* CColorStore::FindTable( ui::StdColorTable tableType ) const
{
	for ( std::vector<CColorTable*>::const_iterator itTable = m_colorTables.begin(); itTable != m_colorTables.end(); ++itTable )
		if ( (*itTable)->GetTableType() == tableType )
			return *itTable;

	return nullptr;
}

const CColorTable* CColorStore::FindTableByName( const std::tstring& tableName ) const
{
	ui::StdColorTable tableType;

	if ( !ui::GetTags_ColorTable().ParseUiAs( tableType, tableName ) )
		return nullptr;

	return FindTable( tableType );
}

const CColorEntry* CColorStore::FindColorEntry( COLORREF rawColor ) const
{
	if ( !ui::IsUndefinedColor( rawColor ) )		// a repo color?
	{
		if ( ui::IsSysColor( rawColor ) )
		{	// look it up only in sys-colors batch
			const CColorTable* pSysColors = FindSystemTable();
			return pSysColors != nullptr ? pSysColors->FindColor( rawColor ) : nullptr;
		}

		rawColor = ui::EvalColor( rawColor );		// for the rest of the tables search for evaluated color

		for ( std::vector<CColorTable*>::const_iterator itTable = m_colorTables.begin(); itTable != m_colorTables.end(); ++itTable )
			if ( (*itTable)->GetTableType() != ui::WindowsSys_Colors )		// look only in tables with real colors
				if ( const CColorEntry* pFound = ( *itTable )->FindColor( rawColor ) )
					return pFound;
	}

	return nullptr;
}

void CColorStore::QueryMatchingColors( OUT std::vector<const CColorEntry*>& rColorEntries, COLORREF rawColor ) const
{
	if ( !ui::IsUndefinedColor( rawColor ) )				// color has entry?
	{
		if ( ui::IsSysColor( rawColor ) )
		{	// look it up only in sys-colors batch
			if ( const CColorTable* pSysColors = FindSystemTable() )
				if ( const CColorEntry* pFound = pSysColors->FindColor( rawColor ) )
					rColorEntries.push_back( pFound );
		}
		else
		{
			rawColor = ui::EvalColor( rawColor );			// for the rest of the tables search for evaluated color

			for ( std::vector<CColorTable*>::const_iterator itTable = m_colorTables.begin(); itTable != m_colorTables.end(); ++itTable )
				if ( (*itTable)->GetTableType() != ui::WindowsSys_Colors )		// look only in tables with real colors
					if ( const CColorEntry* pFound = ( *itTable )->FindColor( rawColor ) )
						rColorEntries.push_back( pFound );
		}
	}
}

std::tstring CColorStore::FormatColorMatch( COLORREF rawColor, bool multiple /*= true*/ ) const
{
	std::vector<const CColorEntry*> colorEntries;

	if ( multiple )
		QueryMatchingColors( colorEntries, rawColor );
	else if ( const CColorEntry* pFoundEntry = FindColorEntry( rawColor ) )
		colorEntries.push_back( pFoundEntry );

	func::ToQualifiedColorName toQualifiedName;
	std::tstring outText;

	if ( !colorEntries.empty() )
		if ( colorEntries.size() > 1 )
		{
			std::unordered_set<std::tstring> colorNames;
			std::transform( colorEntries.begin(), colorEntries.end(), std::inserter( colorNames, colorNames.begin() ), func::ToColorName() );

			if ( 1 == colorNames.size() )		// single shared color name?
				outText = *colorNames.begin() + CColorEntry::s_fieldSep + str::Enquote( str::Join( colorEntries, _T(", "), func::ToTableName() ).c_str(), _T("("), _T(")") );
			else
				outText = str::Join( colorEntries, _T(", "), toQualifiedName );
		}
		else
			outText = toQualifiedName( colorEntries.front(), CColorEntry::s_fieldSep );

	stream::Tag( outText, ui::FormatColor( rawColor ), CColorEntry::s_fieldSep );
	return outText;
}


// CColorRepository implementation

CColorRepository::CColorRepository( void )
{
	// add color tables in ui::StdColorTable order:
	m_colorTables.push_back( MakeTable_System() );
	m_colorTables.push_back( MakeTable_Standard() );
	m_colorTables.push_back( MakeTable_Custom() );
	m_colorTables.push_back( MakeTable_Office2003() );
	m_colorTables.push_back( MakeTable_DirectX() );
	m_colorTables.push_back( MakeTable_HTML() );
	m_colorTables.push_back( MakeTable_X11() );
}

const CColorRepository* CColorRepository::Instance( void )
{
	static const CColorRepository s_colorRepo;
	return &s_colorRepo;
}

void CColorRepository::Clear( void )
{
	utl::ClearOwningContainer( m_colorTables );
}


CColorTable* CColorRepository::MakeTable_System( void )
{
	CColorTable* pSysTable = new CColorTable( ui::WindowsSys_Colors, BaseId_System, 31, 8 );		// 31 colors: 2 columns x 16 rows

	pSysTable->Add( CColorEntry( ui::MakeSysColor( COLOR_BACKGROUND ), _T("Desktop Background") ) );
	pSysTable->Add( CColorEntry( ui::MakeSysColor( COLOR_WINDOW ), _T("Window Background") ) );
	pSysTable->Add( CColorEntry( ui::MakeSysColor( COLOR_WINDOWTEXT ), _T("Window Text") ) );
	pSysTable->Add( CColorEntry( ui::MakeSysColor( COLOR_WINDOWFRAME ), _T("Window Frame") ) );
	pSysTable->Add( CColorEntry( ui::MakeSysColor( COLOR_BTNFACE ), _T("Button Face") ) );
	pSysTable->Add( CColorEntry( ui::MakeSysColor( COLOR_BTNHIGHLIGHT ), _T("Button Highlight") ) );
	pSysTable->Add( CColorEntry( ui::MakeSysColor( COLOR_BTNSHADOW ), _T("Button Shadow") ) );
	pSysTable->Add( CColorEntry( ui::MakeSysColor( COLOR_BTNTEXT ), _T("Button Text") ) );
	pSysTable->Add( CColorEntry( ui::MakeSysColor( COLOR_GRAYTEXT ), _T("Disabled Text") ) );
	pSysTable->Add( CColorEntry( ui::MakeSysColor( COLOR_3DDKSHADOW ), _T("3D Dark Shadow") ) );
	pSysTable->Add( CColorEntry( ui::MakeSysColor( COLOR_3DLIGHT ), _T("3D Light Color") ) );
	pSysTable->Add( CColorEntry( ui::MakeSysColor( COLOR_HIGHLIGHT ), _T("Selected") ) );
	pSysTable->Add( CColorEntry( ui::MakeSysColor( COLOR_HIGHLIGHTTEXT ), _T("Selected Text") ) );
	pSysTable->Add( CColorEntry( ui::MakeSysColor( COLOR_CAPTIONTEXT ), _T("Scroll Bar Arrow") ) );
	pSysTable->Add( CColorEntry( ui::MakeSysColor( COLOR_HOTLIGHT ), _T("Hot-Track") ) );
	pSysTable->Add( CColorEntry( ui::MakeSysColor( COLOR_INFOBK ), _T("Tooltip Background") ) );
	pSysTable->Add( CColorEntry( ui::MakeSysColor( COLOR_INFOTEXT ), _T("Tooltip Text") ) );
	pSysTable->Add( CColorEntry( ui::MakeSysColor( COLOR_SCROLLBAR ), _T("Scroll Bar Gray Area") ) );
	pSysTable->Add( CColorEntry( ui::MakeSysColor( COLOR_MENU ), _T("Menu Background") ) );
	pSysTable->Add( CColorEntry( ui::MakeSysColor( COLOR_MENUBAR ), _T("Flat Menu Bar") ) );
	pSysTable->Add( CColorEntry( ui::MakeSysColor( COLOR_MENUHILIGHT ), _T("Menu Highlight") ) );
	pSysTable->Add( CColorEntry( ui::MakeSysColor( COLOR_MENUTEXT ), _T("Menu Text") ) );
	pSysTable->Add( CColorEntry( ui::MakeSysColor( COLOR_ACTIVECAPTION ), _T("Active Window Title Bar") ) );
	pSysTable->Add( CColorEntry( ui::MakeSysColor( COLOR_GRADIENTACTIVECAPTION ), _T("Active Window Gradient") ) );
	pSysTable->Add( CColorEntry( ui::MakeSysColor( COLOR_ACTIVEBORDER ), _T("Active Window Border") ) );
	pSysTable->Add( CColorEntry( ui::MakeSysColor( COLOR_INACTIVECAPTION ), _T("Inactive Window Caption") ) );
	pSysTable->Add( CColorEntry( ui::MakeSysColor( COLOR_GRADIENTINACTIVECAPTION ), _T("Inactive Window Gradient") ) );
	pSysTable->Add( CColorEntry( ui::MakeSysColor( COLOR_INACTIVEBORDER ), _T("Inactive Window Border") ) );
	pSysTable->Add( CColorEntry( ui::MakeSysColor( COLOR_INACTIVECAPTIONTEXT ), _T("Inactive Window Caption Text") ) );
	pSysTable->Add( CColorEntry( ui::MakeSysColor( COLOR_APPWORKSPACE ), _T("MDI Background") ) );
	pSysTable->Add( CColorEntry( ui::MakeSysColor( COLOR_DESKTOP ), _T("Desktop Background") ) );

	return pSysTable;
}

CColorTable* CColorRepository::MakeTable_Standard( void )
{
	CColorTable* pTable = new CColorTable( ui::Standard_Colors, BaseId_Standard, color::_Standard_ColorCount, 8 );		// 40 colors: 8 columns x 6 rows

	pTable->Add( COLOR_ENTRY( color::Black ) );
	pTable->Add( COLOR_ENTRY( color::DarkRed ) );
	pTable->Add( COLOR_ENTRY( color::Red ) );
	pTable->Add( COLOR_ENTRY( color::Magenta ) );
	pTable->Add( COLOR_ENTRY( color::Rose ) );
	pTable->Add( COLOR_ENTRY( color::Brown ) );
	pTable->Add( COLOR_ENTRY( color::Orange ) );
	pTable->Add( COLOR_ENTRY( color::LightOrange ) );
	pTable->Add( COLOR_ENTRY( color::Gold ) );
	pTable->Add( COLOR_ENTRY( color::Tan ) );
	pTable->Add( COLOR_ENTRY( color::OliveGreen ) );
	pTable->Add( COLOR_ENTRY( color::DarkYellow ) );
	pTable->Add( COLOR_ENTRY( color::Lime ) );
	pTable->Add( COLOR_ENTRY( color::Yellow ) );
	pTable->Add( COLOR_ENTRY( color::LightYellow ) );
	pTable->Add( COLOR_ENTRY( color::DarkGreen ) );
	pTable->Add( COLOR_ENTRY( color::Green ) );
	pTable->Add( COLOR_ENTRY( color::SeaGreen ) );
	pTable->Add( COLOR_ENTRY( color::BrightGreen ) );
	pTable->Add( COLOR_ENTRY( color::LightGreen ) );
	pTable->Add( COLOR_ENTRY( color::DarkTeal ) );
	pTable->Add( COLOR_ENTRY( color::Teal ) );
	pTable->Add( COLOR_ENTRY( color::Aqua ) );
	pTable->Add( COLOR_ENTRY( color::Turquoise ) );
	pTable->Add( COLOR_ENTRY( color::LightTurquoise ) );
	pTable->Add( COLOR_ENTRY( color::DarkBlue ) );
	pTable->Add( COLOR_ENTRY( color::Blue ) );
	pTable->Add( COLOR_ENTRY( color::LightBlue ) );
	pTable->Add( COLOR_ENTRY( color::SkyBlue ) );
	pTable->Add( COLOR_ENTRY( color::PaleBlue ) );
	pTable->Add( COLOR_ENTRY( color::Indigo ) );
	pTable->Add( COLOR_ENTRY( color::BlueGray ) );
	pTable->Add( COLOR_ENTRY( color::Violet ) );
	pTable->Add( COLOR_ENTRY( color::Plum ) );
	pTable->Add( COLOR_ENTRY( color::Lavender ) );
	pTable->Add( CColorEntry( color::Grey80, "Grey 80%" ) );
	pTable->Add( CColorEntry( color::Grey50, "Grey 50%" ) );
	pTable->Add( CColorEntry( color::Grey40, "Grey 40%" ) );
	pTable->Add( CColorEntry( color::Grey25, "Grey 25%" ) );
	pTable->Add( COLOR_ENTRY( color::White ) );

	return pTable;
}

CColorTable* CColorRepository::MakeTable_Custom( void )
{
	CColorTable* pTable = new CColorTable( ui::Custom_Colors, BaseId_Custom, color::_Custom_ColorCount, 4 );		// 23 colors: 4 columns x 6 rows

	pTable->Add( COLOR_ENTRY( color::VeryDarkGrey ) );
	pTable->Add( COLOR_ENTRY( color::DarkGrey ) );
	pTable->Add( COLOR_ENTRY( color::LightGrey ) );
	pTable->Add( COLOR_ENTRY( color::Grey60 ) );
	pTable->Add( COLOR_ENTRY( color::Cyan ) );
	pTable->Add( COLOR_ENTRY( color::DarkMagenta ) );
	pTable->Add( COLOR_ENTRY( color::LightGreenish ) );
	pTable->Add( COLOR_ENTRY( color::SpringGreen ) );
	pTable->Add( COLOR_ENTRY( color::NeonGreen ) );
	pTable->Add( COLOR_ENTRY( color::AzureBlue ) );
	pTable->Add( COLOR_ENTRY( color::BlueWindows10 ) );
	pTable->Add( COLOR_ENTRY( color::BlueTextWin10 ) );
	pTable->Add( COLOR_ENTRY( color::ScarletRed ) );
	pTable->Add( COLOR_ENTRY( color::Salmon ) );
	pTable->Add( COLOR_ENTRY( color::Pink ) );
	pTable->Add( COLOR_ENTRY( color::PastelPink ) );
	pTable->Add( COLOR_ENTRY( color::LightPastelPink ) );
	pTable->Add( COLOR_ENTRY( color::ToolStripPink ) );
	pTable->Add( COLOR_ENTRY( color::TranspPink ) );
	pTable->Add( COLOR_ENTRY( color::SolidOrange ) );
	pTable->Add( COLOR_ENTRY( color::Amber ) );
	pTable->Add( COLOR_ENTRY( color::PaleYellow ) );
	pTable->Add( COLOR_ENTRY( color::GhostWhite ) );

	return pTable;
}

CColorTable* CColorRepository::MakeTable_Office2003( void )
{
	CColorTable* pTable = new CColorTable( ui::Office2003_Colors, BaseId_Office2003, color::Office2003::_Office2003_ColorCount, 8 );		// 40 colors: 8 columns x 5 rows

	pTable->Add( COLOR_ENTRY( color::Office2003::Black ) );
	pTable->Add( COLOR_ENTRY( color::Office2003::Brown ) );
	pTable->Add( COLOR_ENTRY( color::Office2003::OliveGreen ) );
	pTable->Add( COLOR_ENTRY( color::Office2003::DarkGreen ) );
	pTable->Add( COLOR_ENTRY( color::Office2003::DarkTeal ) );
	pTable->Add( COLOR_ENTRY( color::Office2003::DarkBlue ) );
	pTable->Add( COLOR_ENTRY( color::Office2003::Indigo ) );
	pTable->Add( CColorEntry( color::Office2003::Gray80, "Gray 80%" ) );
	pTable->Add( COLOR_ENTRY( color::Office2003::DarkRed ) );
	pTable->Add( COLOR_ENTRY( color::Office2003::Orange ) );
	pTable->Add( COLOR_ENTRY( color::Office2003::DarkYellow ) );
	pTable->Add( COLOR_ENTRY( color::Office2003::Green ) );
	pTable->Add( COLOR_ENTRY( color::Office2003::Teal ) );
	pTable->Add( COLOR_ENTRY( color::Office2003::Blue ) );
	pTable->Add( COLOR_ENTRY( color::Office2003::BlueGray ) );
	pTable->Add( CColorEntry( color::Office2003::Gray50, "Gray 50%" ) );
	pTable->Add( COLOR_ENTRY( color::Office2003::Red ) );
	pTable->Add( COLOR_ENTRY( color::Office2003::LightOrange ) );
	pTable->Add( COLOR_ENTRY( color::Office2003::Lime ) );
	pTable->Add( COLOR_ENTRY( color::Office2003::SeaGreen ) );
	pTable->Add( COLOR_ENTRY( color::Office2003::Aqua ) );
	pTable->Add( COLOR_ENTRY( color::Office2003::LightBlue ) );
	pTable->Add( COLOR_ENTRY( color::Office2003::Violet ) );
	pTable->Add( CColorEntry( color::Office2003::Gray40, "Gray 40%" ) );
	pTable->Add( COLOR_ENTRY( color::Office2003::Pink ) );
	pTable->Add( COLOR_ENTRY( color::Office2003::Gold ) );
	pTable->Add( COLOR_ENTRY( color::Office2003::Yellow ) );
	pTable->Add( COLOR_ENTRY( color::Office2003::BrightGreen ) );
	pTable->Add( COLOR_ENTRY( color::Office2003::Turqoise ) );
	pTable->Add( COLOR_ENTRY( color::Office2003::SkyBlue ) );
	pTable->Add( COLOR_ENTRY( color::Office2003::Plum ) );
	pTable->Add( CColorEntry( color::Office2003::Gray25, "Gray 25%" ) );
	pTable->Add( COLOR_ENTRY( color::Office2003::Rose ) );
	pTable->Add( COLOR_ENTRY( color::Office2003::Tan ) );
	pTable->Add( COLOR_ENTRY( color::Office2003::LightYellow ) );
	pTable->Add( COLOR_ENTRY( color::Office2003::LightGreen ) );
	pTable->Add( COLOR_ENTRY( color::Office2003::LightTurqoise ) );
	pTable->Add( COLOR_ENTRY( color::Office2003::PaleBlue ) );
	pTable->Add( COLOR_ENTRY( color::Office2003::Lavender ) );
	pTable->Add( COLOR_ENTRY( color::Office2003::White ) );

	return pTable;
}

CColorTable* CColorRepository::MakeTable_DirectX( void )
{
	CColorTable* pTable = new CColorTable( ui::DirectX_Colors, BaseId_DirectX, color::directx::_DirectX_ColorCount, 14 );		// 140 colors: 10 columns x 14 rows

	pTable->Add( COLOR_ENTRY( color::directx::AliceBlue ) );
	pTable->Add( COLOR_ENTRY( color::directx::AntiqueWhite ) );
	pTable->Add( COLOR_ENTRY( color::directx::Aqua ) );
	pTable->Add( COLOR_ENTRY( color::directx::Aquamarine ) );
	pTable->Add( COLOR_ENTRY( color::directx::Azure ) );
	pTable->Add( COLOR_ENTRY( color::directx::Beige ) );
	pTable->Add( COLOR_ENTRY( color::directx::Bisque ) );
	pTable->Add( COLOR_ENTRY( color::directx::Black ) );
	pTable->Add( COLOR_ENTRY( color::directx::BlanchedAlmond ) );
	pTable->Add( COLOR_ENTRY( color::directx::Blue ) );
	pTable->Add( COLOR_ENTRY( color::directx::BlueViolet ) );
	pTable->Add( COLOR_ENTRY( color::directx::Brown ) );
	pTable->Add( COLOR_ENTRY( color::directx::BurlyWood ) );
	pTable->Add( COLOR_ENTRY( color::directx::CadetBlue ) );
	pTable->Add( COLOR_ENTRY( color::directx::Chartreuse ) );
	pTable->Add( COLOR_ENTRY( color::directx::Chocolate ) );
	pTable->Add( COLOR_ENTRY( color::directx::Coral ) );
	pTable->Add( COLOR_ENTRY( color::directx::CornflowerBlue ) );
	pTable->Add( COLOR_ENTRY( color::directx::Cornsilk ) );
	pTable->Add( COLOR_ENTRY( color::directx::Crimson ) );
	pTable->Add( COLOR_ENTRY( color::directx::Cyan ) );
	pTable->Add( COLOR_ENTRY( color::directx::DarkBlue ) );
	pTable->Add( COLOR_ENTRY( color::directx::DarkCyan ) );
	pTable->Add( COLOR_ENTRY( color::directx::DarkGoldenrod ) );
	pTable->Add( COLOR_ENTRY( color::directx::DarkGray ) );
	pTable->Add( COLOR_ENTRY( color::directx::DarkGreen ) );
	pTable->Add( COLOR_ENTRY( color::directx::DarkKhaki ) );
	pTable->Add( COLOR_ENTRY( color::directx::DarkMagenta ) );
	pTable->Add( COLOR_ENTRY( color::directx::DarkOliveGreen ) );
	pTable->Add( COLOR_ENTRY( color::directx::DarkOrange ) );
	pTable->Add( COLOR_ENTRY( color::directx::DarkOrchid ) );
	pTable->Add( COLOR_ENTRY( color::directx::DarkRed ) );
	pTable->Add( COLOR_ENTRY( color::directx::DarkSalmon ) );
	pTable->Add( COLOR_ENTRY( color::directx::DarkSeaGreen ) );
	pTable->Add( COLOR_ENTRY( color::directx::DarkSlateBlue ) );
	pTable->Add( COLOR_ENTRY( color::directx::DarkSlateGray ) );
	pTable->Add( COLOR_ENTRY( color::directx::DarkTurquoise ) );
	pTable->Add( COLOR_ENTRY( color::directx::DarkViolet ) );
	pTable->Add( COLOR_ENTRY( color::directx::DeepPink ) );
	pTable->Add( COLOR_ENTRY( color::directx::DeepSkyBlue ) );
	pTable->Add( COLOR_ENTRY( color::directx::DimGray ) );
	pTable->Add( COLOR_ENTRY( color::directx::DodgerBlue ) );
	pTable->Add( COLOR_ENTRY( color::directx::Firebrick ) );
	pTable->Add( COLOR_ENTRY( color::directx::FloralWhite ) );
	pTable->Add( COLOR_ENTRY( color::directx::ForestGreen ) );
	pTable->Add( COLOR_ENTRY( color::directx::Fuchsia ) );
	pTable->Add( COLOR_ENTRY( color::directx::Gainsboro ) );
	pTable->Add( COLOR_ENTRY( color::directx::GhostWhite ) );
	pTable->Add( COLOR_ENTRY( color::directx::Gold ) );
	pTable->Add( COLOR_ENTRY( color::directx::Goldenrod ) );
	pTable->Add( COLOR_ENTRY( color::directx::Gray ) );
	pTable->Add( COLOR_ENTRY( color::directx::Green ) );
	pTable->Add( COLOR_ENTRY( color::directx::GreenYellow ) );
	pTable->Add( COLOR_ENTRY( color::directx::Honeydew ) );
	pTable->Add( COLOR_ENTRY( color::directx::HotPink ) );
	pTable->Add( COLOR_ENTRY( color::directx::IndianRed ) );
	pTable->Add( COLOR_ENTRY( color::directx::Indigo ) );
	pTable->Add( COLOR_ENTRY( color::directx::Ivory ) );
	pTable->Add( COLOR_ENTRY( color::directx::Khaki ) );
	pTable->Add( COLOR_ENTRY( color::directx::Lavender ) );
	pTable->Add( COLOR_ENTRY( color::directx::LavenderBlush ) );
	pTable->Add( COLOR_ENTRY( color::directx::LawnGreen ) );
	pTable->Add( COLOR_ENTRY( color::directx::LemonChiffon ) );
	pTable->Add( COLOR_ENTRY( color::directx::LightBlue ) );
	pTable->Add( COLOR_ENTRY( color::directx::LightCoral ) );
	pTable->Add( COLOR_ENTRY( color::directx::LightCyan ) );
	pTable->Add( COLOR_ENTRY( color::directx::LightGoldenrodYellow ) );
	pTable->Add( COLOR_ENTRY( color::directx::LightGreen ) );
	pTable->Add( COLOR_ENTRY( color::directx::LightGray ) );
	pTable->Add( COLOR_ENTRY( color::directx::LightPink ) );
	pTable->Add( COLOR_ENTRY( color::directx::LightSalmon ) );
	pTable->Add( COLOR_ENTRY( color::directx::LightSeaGreen ) );
	pTable->Add( COLOR_ENTRY( color::directx::LightSkyBlue ) );
	pTable->Add( COLOR_ENTRY( color::directx::LightSlateGray ) );
	pTable->Add( COLOR_ENTRY( color::directx::LightSteelBlue ) );
	pTable->Add( COLOR_ENTRY( color::directx::LightYellow ) );
	pTable->Add( COLOR_ENTRY( color::directx::Lime ) );
	pTable->Add( COLOR_ENTRY( color::directx::LimeGreen ) );
	pTable->Add( COLOR_ENTRY( color::directx::Linen ) );
	pTable->Add( COLOR_ENTRY( color::directx::Magenta ) );
	pTable->Add( COLOR_ENTRY( color::directx::Maroon ) );
	pTable->Add( COLOR_ENTRY( color::directx::MediumAquamarine ) );
	pTable->Add( COLOR_ENTRY( color::directx::MediumBlue ) );
	pTable->Add( COLOR_ENTRY( color::directx::MediumOrchid ) );
	pTable->Add( COLOR_ENTRY( color::directx::MediumPurple ) );
	pTable->Add( COLOR_ENTRY( color::directx::MediumSeaGreen ) );
	pTable->Add( COLOR_ENTRY( color::directx::MediumSlateBlue ) );
	pTable->Add( COLOR_ENTRY( color::directx::MediumSpringGreen ) );
	pTable->Add( COLOR_ENTRY( color::directx::MediumTurquoise ) );
	pTable->Add( COLOR_ENTRY( color::directx::MediumVioletRed ) );
	pTable->Add( COLOR_ENTRY( color::directx::MidnightBlue ) );
	pTable->Add( COLOR_ENTRY( color::directx::MintCream ) );
	pTable->Add( COLOR_ENTRY( color::directx::MistyRose ) );
	pTable->Add( COLOR_ENTRY( color::directx::Moccasin ) );
	pTable->Add( COLOR_ENTRY( color::directx::NavajoWhite ) );
	pTable->Add( COLOR_ENTRY( color::directx::Navy ) );
	pTable->Add( COLOR_ENTRY( color::directx::OldLace ) );
	pTable->Add( COLOR_ENTRY( color::directx::Olive ) );
	pTable->Add( COLOR_ENTRY( color::directx::OliveDrab ) );
	pTable->Add( COLOR_ENTRY( color::directx::Orange ) );
	pTable->Add( COLOR_ENTRY( color::directx::OrangeRed ) );
	pTable->Add( COLOR_ENTRY( color::directx::Orchid ) );
	pTable->Add( COLOR_ENTRY( color::directx::PaleGoldenrod ) );
	pTable->Add( COLOR_ENTRY( color::directx::PaleGreen ) );
	pTable->Add( COLOR_ENTRY( color::directx::PaleTurquoise ) );
	pTable->Add( COLOR_ENTRY( color::directx::PaleVioletRed ) );
	pTable->Add( COLOR_ENTRY( color::directx::PapayaWhip ) );
	pTable->Add( COLOR_ENTRY( color::directx::PeachPuff ) );
	pTable->Add( COLOR_ENTRY( color::directx::Peru ) );
	pTable->Add( COLOR_ENTRY( color::directx::Pink ) );
	pTable->Add( COLOR_ENTRY( color::directx::Plum ) );
	pTable->Add( COLOR_ENTRY( color::directx::PowderBlue ) );
	pTable->Add( COLOR_ENTRY( color::directx::Purple ) );
	pTable->Add( COLOR_ENTRY( color::directx::Red ) );
	pTable->Add( COLOR_ENTRY( color::directx::RosyBrown ) );
	pTable->Add( COLOR_ENTRY( color::directx::RoyalBlue ) );
	pTable->Add( COLOR_ENTRY( color::directx::SaddleBrown ) );
	pTable->Add( COLOR_ENTRY( color::directx::Salmon ) );
	pTable->Add( COLOR_ENTRY( color::directx::SandyBrown ) );
	pTable->Add( COLOR_ENTRY( color::directx::SeaGreen ) );
	pTable->Add( COLOR_ENTRY( color::directx::SeaShell ) );
	pTable->Add( COLOR_ENTRY( color::directx::Sienna ) );
	pTable->Add( COLOR_ENTRY( color::directx::Silver ) );
	pTable->Add( COLOR_ENTRY( color::directx::SkyBlue ) );
	pTable->Add( COLOR_ENTRY( color::directx::SlateBlue ) );
	pTable->Add( COLOR_ENTRY( color::directx::SlateGray ) );
	pTable->Add( COLOR_ENTRY( color::directx::Snow ) );
	pTable->Add( COLOR_ENTRY( color::directx::SpringGreen ) );
	pTable->Add( COLOR_ENTRY( color::directx::SteelBlue ) );
	pTable->Add( COLOR_ENTRY( color::directx::Tan ) );
	pTable->Add( COLOR_ENTRY( color::directx::Teal ) );
	pTable->Add( COLOR_ENTRY( color::directx::Thistle ) );
	pTable->Add( COLOR_ENTRY( color::directx::Tomato ) );
	pTable->Add( COLOR_ENTRY( color::directx::Turquoise ) );
	pTable->Add( COLOR_ENTRY( color::directx::Violet ) );
	pTable->Add( COLOR_ENTRY( color::directx::Wheat ) );
	pTable->Add( COLOR_ENTRY( color::directx::White ) );
	pTable->Add( COLOR_ENTRY( color::directx::WhiteSmoke ) );
	pTable->Add( COLOR_ENTRY( color::directx::Yellow ) );
	pTable->Add( COLOR_ENTRY( color::directx::YellowGreen ) );

	return pTable;
}

CColorTable* CColorRepository::MakeTable_HTML( void )
{
	CColorTable* pTable = new CColorTable( ui::HTML_Colors, BaseId_HTML, color::html::_Html_ColorCount, 20 );		// 300 colors: 20 columns x 15 rows

	pTable->Add( COLOR_ENTRY( color::html::Black ) );
	pTable->Add( COLOR_ENTRY( color::html::Gray0 ) );
	pTable->Add( COLOR_ENTRY( color::html::Gray18 ) );
	pTable->Add( COLOR_ENTRY( color::html::Gray21 ) );
	pTable->Add( COLOR_ENTRY( color::html::Gray23 ) );
	pTable->Add( COLOR_ENTRY( color::html::Gray24 ) );
	pTable->Add( COLOR_ENTRY( color::html::Gray25 ) );
	pTable->Add( COLOR_ENTRY( color::html::Gray26 ) );
	pTable->Add( COLOR_ENTRY( color::html::Gray27 ) );
	pTable->Add( COLOR_ENTRY( color::html::Gray28 ) );
	pTable->Add( COLOR_ENTRY( color::html::Gray29 ) );
	pTable->Add( COLOR_ENTRY( color::html::Gray30 ) );
	pTable->Add( COLOR_ENTRY( color::html::Gray31 ) );
	pTable->Add( COLOR_ENTRY( color::html::Gray32 ) );
	pTable->Add( COLOR_ENTRY( color::html::Gray34 ) );
	pTable->Add( COLOR_ENTRY( color::html::Gray35 ) );
	pTable->Add( COLOR_ENTRY( color::html::Gray36 ) );
	pTable->Add( COLOR_ENTRY( color::html::Gray37 ) );
	pTable->Add( COLOR_ENTRY( color::html::Gray38 ) );
	pTable->Add( COLOR_ENTRY( color::html::Gray39 ) );
	pTable->Add( COLOR_ENTRY( color::html::Gray40 ) );
	pTable->Add( COLOR_ENTRY( color::html::Gray41 ) );
	pTable->Add( COLOR_ENTRY( color::html::Gray42 ) );
	pTable->Add( COLOR_ENTRY( color::html::Gray43 ) );
	pTable->Add( COLOR_ENTRY( color::html::Gray44 ) );
	pTable->Add( COLOR_ENTRY( color::html::Gray45 ) );
	pTable->Add( COLOR_ENTRY( color::html::Gray46 ) );
	pTable->Add( COLOR_ENTRY( color::html::Gray47 ) );
	pTable->Add( COLOR_ENTRY( color::html::Gray48 ) );
	pTable->Add( COLOR_ENTRY( color::html::Gray49 ) );
	pTable->Add( COLOR_ENTRY( color::html::Gray50 ) );
	pTable->Add( COLOR_ENTRY( color::html::Gray ) );
	pTable->Add( COLOR_ENTRY( color::html::SlateGray4 ) );
	pTable->Add( COLOR_ENTRY( color::html::SlateGray ) );
	pTable->Add( COLOR_ENTRY( color::html::LightSteelBlue4 ) );
	pTable->Add( COLOR_ENTRY( color::html::LightSlateGray ) );
	pTable->Add( COLOR_ENTRY( color::html::CadetBlue4 ) );
	pTable->Add( COLOR_ENTRY( color::html::DarkSlateGray4 ) );
	pTable->Add( COLOR_ENTRY( color::html::Thistle4 ) );
	pTable->Add( COLOR_ENTRY( color::html::MediumSlateBlue ) );
	pTable->Add( COLOR_ENTRY( color::html::MediumPurple4 ) );
	pTable->Add( COLOR_ENTRY( color::html::MidnightBlue ) );
	pTable->Add( COLOR_ENTRY( color::html::DarkSlateBlue ) );
	pTable->Add( COLOR_ENTRY( color::html::DarkSlateGray ) );
	pTable->Add( COLOR_ENTRY( color::html::DimGray ) );
	pTable->Add( COLOR_ENTRY( color::html::CornflowerBlue ) );
	pTable->Add( COLOR_ENTRY( color::html::RoyalBlue4 ) );
	pTable->Add( COLOR_ENTRY( color::html::SlateBlue4 ) );
	pTable->Add( COLOR_ENTRY( color::html::RoyalBlue ) );
	pTable->Add( COLOR_ENTRY( color::html::RoyalBlue1 ) );
	pTable->Add( COLOR_ENTRY( color::html::RoyalBlue2 ) );
	pTable->Add( COLOR_ENTRY( color::html::RoyalBlue3 ) );
	pTable->Add( COLOR_ENTRY( color::html::DeepSkyBlue ) );
	pTable->Add( COLOR_ENTRY( color::html::DeepSkyBlue2 ) );
	pTable->Add( COLOR_ENTRY( color::html::SlateBlue ) );
	pTable->Add( COLOR_ENTRY( color::html::DeepSkyBlue3 ) );
	pTable->Add( COLOR_ENTRY( color::html::DeepSkyBlue4 ) );
	pTable->Add( COLOR_ENTRY( color::html::DodgerBlue ) );
	pTable->Add( COLOR_ENTRY( color::html::DodgerBlue2 ) );
	pTable->Add( COLOR_ENTRY( color::html::DodgerBlue3 ) );
	pTable->Add( COLOR_ENTRY( color::html::DodgerBlue4 ) );
	pTable->Add( COLOR_ENTRY( color::html::SteelBlue4 ) );
	pTable->Add( COLOR_ENTRY( color::html::SteelBlue ) );
	pTable->Add( COLOR_ENTRY( color::html::SlateBlue2 ) );
	pTable->Add( COLOR_ENTRY( color::html::Violet ) );
	pTable->Add( COLOR_ENTRY( color::html::MediumPurple3 ) );
	pTable->Add( COLOR_ENTRY( color::html::MediumPurple ) );
	pTable->Add( COLOR_ENTRY( color::html::MediumPurple2 ) );
	pTable->Add( COLOR_ENTRY( color::html::MediumPurple1 ) );
	pTable->Add( COLOR_ENTRY( color::html::LightSteelBlue ) );
	pTable->Add( COLOR_ENTRY( color::html::SteelBlue3 ) );
	pTable->Add( COLOR_ENTRY( color::html::SteelBlue2 ) );
	pTable->Add( COLOR_ENTRY( color::html::SteelBlue1 ) );
	pTable->Add( COLOR_ENTRY( color::html::SkyBlue3 ) );
	pTable->Add( COLOR_ENTRY( color::html::SkyBlue4 ) );
	pTable->Add( COLOR_ENTRY( color::html::SlateBlue3 ) );
	pTable->Add( COLOR_ENTRY( color::html::SlateBlue5 ) );
	pTable->Add( COLOR_ENTRY( color::html::SlateGray3 ) );
	pTable->Add( COLOR_ENTRY( color::html::VioletRed ) );
	pTable->Add( COLOR_ENTRY( color::html::VioletRed1 ) );
	pTable->Add( COLOR_ENTRY( color::html::VioletRed2 ) );
	pTable->Add( COLOR_ENTRY( color::html::DeepPink ) );
	pTable->Add( COLOR_ENTRY( color::html::DeepPink2 ) );
	pTable->Add( COLOR_ENTRY( color::html::DeepPink3 ) );
	pTable->Add( COLOR_ENTRY( color::html::DeepPink4 ) );
	pTable->Add( COLOR_ENTRY( color::html::MediumVioletRed ) );
	pTable->Add( COLOR_ENTRY( color::html::VioletRed3 ) );
	pTable->Add( COLOR_ENTRY( color::html::Firebrick ) );
	pTable->Add( COLOR_ENTRY( color::html::VioletRed4 ) );
	pTable->Add( COLOR_ENTRY( color::html::Maroon4 ) );
	pTable->Add( COLOR_ENTRY( color::html::Maroon ) );
	pTable->Add( COLOR_ENTRY( color::html::Maroon3 ) );
	pTable->Add( COLOR_ENTRY( color::html::Maroon2 ) );
	pTable->Add( COLOR_ENTRY( color::html::Maroon1 ) );
	pTable->Add( COLOR_ENTRY( color::html::Magenta ) );
	pTable->Add( COLOR_ENTRY( color::html::Magenta1 ) );
	pTable->Add( COLOR_ENTRY( color::html::Magenta2 ) );
	pTable->Add( COLOR_ENTRY( color::html::Magenta3 ) );
	pTable->Add( COLOR_ENTRY( color::html::MediumOrchid ) );
	pTable->Add( COLOR_ENTRY( color::html::MediumOrchid1 ) );
	pTable->Add( COLOR_ENTRY( color::html::MediumOrchid2 ) );
	pTable->Add( COLOR_ENTRY( color::html::MediumOrchid3 ) );
	pTable->Add( COLOR_ENTRY( color::html::MediumOrchid4 ) );
	pTable->Add( COLOR_ENTRY( color::html::Purple ) );
	pTable->Add( COLOR_ENTRY( color::html::Purple1 ) );
	pTable->Add( COLOR_ENTRY( color::html::Purple2 ) );
	pTable->Add( COLOR_ENTRY( color::html::Purple3 ) );
	pTable->Add( COLOR_ENTRY( color::html::Purple4 ) );
	pTable->Add( COLOR_ENTRY( color::html::DarkOrchid4 ) );
	pTable->Add( COLOR_ENTRY( color::html::DarkOrchid ) );
	pTable->Add( COLOR_ENTRY( color::html::DarkViolet ) );
	pTable->Add( COLOR_ENTRY( color::html::DarkOrchid3 ) );
	pTable->Add( COLOR_ENTRY( color::html::DarkOrchid2 ) );
	pTable->Add( COLOR_ENTRY( color::html::DarkOrchid1 ) );
	pTable->Add( COLOR_ENTRY( color::html::Plum4 ) );
	pTable->Add( COLOR_ENTRY( color::html::PaleVioletRed ) );
	pTable->Add( COLOR_ENTRY( color::html::PaleVioletRed1 ) );
	pTable->Add( COLOR_ENTRY( color::html::PaleVioletRed2 ) );
	pTable->Add( COLOR_ENTRY( color::html::PaleVioletRed3 ) );
	pTable->Add( COLOR_ENTRY( color::html::PaleVioletRed4 ) );
	pTable->Add( COLOR_ENTRY( color::html::Plum ) );
	pTable->Add( COLOR_ENTRY( color::html::Plum1 ) );
	pTable->Add( COLOR_ENTRY( color::html::Plum2 ) );
	pTable->Add( COLOR_ENTRY( color::html::Plum3 ) );
	pTable->Add( COLOR_ENTRY( color::html::Thistle ) );
	pTable->Add( COLOR_ENTRY( color::html::Thistle3 ) );
	pTable->Add( COLOR_ENTRY( color::html::LavenderBlush2 ) );
	pTable->Add( COLOR_ENTRY( color::html::LavenderBlush3 ) );
	pTable->Add( COLOR_ENTRY( color::html::Thistle2 ) );
	pTable->Add( COLOR_ENTRY( color::html::Thistle1 ) );
	pTable->Add( COLOR_ENTRY( color::html::Lavender ) );
	pTable->Add( COLOR_ENTRY( color::html::LavenderBlush ) );
	pTable->Add( COLOR_ENTRY( color::html::LightSteelBlue1 ) );
	pTable->Add( COLOR_ENTRY( color::html::LightBlue ) );
	pTable->Add( COLOR_ENTRY( color::html::LightBlue1 ) );
	pTable->Add( COLOR_ENTRY( color::html::LightCyan ) );
	pTable->Add( COLOR_ENTRY( color::html::SlateGray1 ) );
	pTable->Add( COLOR_ENTRY( color::html::SlateGray2 ) );
	pTable->Add( COLOR_ENTRY( color::html::LightSteelBlue2 ) );
	pTable->Add( COLOR_ENTRY( color::html::Turquoise1 ) );
	pTable->Add( COLOR_ENTRY( color::html::Cyan ) );
	pTable->Add( COLOR_ENTRY( color::html::Cyan1 ) );
	pTable->Add( COLOR_ENTRY( color::html::Cyan2 ) );
	pTable->Add( COLOR_ENTRY( color::html::Turquoise2 ) );
	pTable->Add( COLOR_ENTRY( color::html::MediumTurquoise ) );
	pTable->Add( COLOR_ENTRY( color::html::Turquoise ) );
	pTable->Add( COLOR_ENTRY( color::html::DarkSlateGray1 ) );
	pTable->Add( COLOR_ENTRY( color::html::DarkSlateGray2 ) );
	pTable->Add( COLOR_ENTRY( color::html::DarkSlateGray3 ) );
	pTable->Add( COLOR_ENTRY( color::html::Cyan3 ) );
	pTable->Add( COLOR_ENTRY( color::html::Turquoise3 ) );
	pTable->Add( COLOR_ENTRY( color::html::CadetBlue3 ) );
	pTable->Add( COLOR_ENTRY( color::html::PaleTurquoise3 ) );
	pTable->Add( COLOR_ENTRY( color::html::LightBlue2 ) );
	pTable->Add( COLOR_ENTRY( color::html::DarkTurquoise ) );
	pTable->Add( COLOR_ENTRY( color::html::Cyan4 ) );
	pTable->Add( COLOR_ENTRY( color::html::LightSeaGreen ) );
	pTable->Add( COLOR_ENTRY( color::html::LightSkyBlue ) );
	pTable->Add( COLOR_ENTRY( color::html::LightSkyBlue2 ) );
	pTable->Add( COLOR_ENTRY( color::html::LightSkyBlue3 ) );
	pTable->Add( COLOR_ENTRY( color::html::SkyBlue5 ) );
	pTable->Add( COLOR_ENTRY( color::html::SkyBlue6 ) );
	pTable->Add( COLOR_ENTRY( color::html::LightSkyBlue4 ) );
	pTable->Add( COLOR_ENTRY( color::html::SkyBlue ) );
	pTable->Add( COLOR_ENTRY( color::html::LightSlateBlue ) );
	pTable->Add( COLOR_ENTRY( color::html::LightCyan2 ) );
	pTable->Add( COLOR_ENTRY( color::html::LightCyan3 ) );
	pTable->Add( COLOR_ENTRY( color::html::LightCyan4 ) );
	pTable->Add( COLOR_ENTRY( color::html::LightBlue3 ) );
	pTable->Add( COLOR_ENTRY( color::html::LightBlue4 ) );
	pTable->Add( COLOR_ENTRY( color::html::PaleTurquoise4 ) );
	pTable->Add( COLOR_ENTRY( color::html::DarkSeaGreen4 ) );
	pTable->Add( COLOR_ENTRY( color::html::MediumAquamarine ) );
	pTable->Add( COLOR_ENTRY( color::html::MediumSeaGreen ) );
	pTable->Add( COLOR_ENTRY( color::html::SeaGreen ) );
	pTable->Add( COLOR_ENTRY( color::html::DarkGreen ) );
	pTable->Add( COLOR_ENTRY( color::html::SeaGreen4 ) );
	pTable->Add( COLOR_ENTRY( color::html::ForestGreen ) );
	pTable->Add( COLOR_ENTRY( color::html::MediumForestGreen ) );
	pTable->Add( COLOR_ENTRY( color::html::SpringGreen4 ) );
	pTable->Add( COLOR_ENTRY( color::html::DarkOliveGreen4 ) );
	pTable->Add( COLOR_ENTRY( color::html::Chartreuse4 ) );
	pTable->Add( COLOR_ENTRY( color::html::Green4 ) );
	pTable->Add( COLOR_ENTRY( color::html::MediumSpringGreen ) );
	pTable->Add( COLOR_ENTRY( color::html::SpringGreen ) );
	pTable->Add( COLOR_ENTRY( color::html::LimeGreen ) );
	pTable->Add( COLOR_ENTRY( color::html::SpringGreen3 ) );
	pTable->Add( COLOR_ENTRY( color::html::DarkSeaGreen ) );
	pTable->Add( COLOR_ENTRY( color::html::DarkSeaGreen3 ) );
	pTable->Add( COLOR_ENTRY( color::html::Green3 ) );
	pTable->Add( COLOR_ENTRY( color::html::Chartreuse3 ) );
	pTable->Add( COLOR_ENTRY( color::html::YellowGreen ) );
	pTable->Add( COLOR_ENTRY( color::html::SpringGreen5 ) );
	pTable->Add( COLOR_ENTRY( color::html::SeaGreen3 ) );
	pTable->Add( COLOR_ENTRY( color::html::SpringGreen2 ) );
	pTable->Add( COLOR_ENTRY( color::html::SpringGreen1 ) );
	pTable->Add( COLOR_ENTRY( color::html::SeaGreen2 ) );
	pTable->Add( COLOR_ENTRY( color::html::SeaGreen1 ) );
	pTable->Add( COLOR_ENTRY( color::html::DarkSeaGreen2 ) );
	pTable->Add( COLOR_ENTRY( color::html::DarkSeaGreen1 ) );
	pTable->Add( COLOR_ENTRY( color::html::Green ) );
	pTable->Add( COLOR_ENTRY( color::html::LawnGreen ) );
	pTable->Add( COLOR_ENTRY( color::html::Green1 ) );
	pTable->Add( COLOR_ENTRY( color::html::Green2 ) );
	pTable->Add( COLOR_ENTRY( color::html::Chartreuse2 ) );
	pTable->Add( COLOR_ENTRY( color::html::Chartreuse ) );
	pTable->Add( COLOR_ENTRY( color::html::GreenYellow ) );
	pTable->Add( COLOR_ENTRY( color::html::DarkOliveGreen1 ) );
	pTable->Add( COLOR_ENTRY( color::html::DarkOliveGreen2 ) );
	pTable->Add( COLOR_ENTRY( color::html::DarkOliveGreen3 ) );
	pTable->Add( COLOR_ENTRY( color::html::Yellow ) );
	pTable->Add( COLOR_ENTRY( color::html::Yellow1 ) );
	pTable->Add( COLOR_ENTRY( color::html::Khaki1 ) );
	pTable->Add( COLOR_ENTRY( color::html::Khaki2 ) );
	pTable->Add( COLOR_ENTRY( color::html::Goldenrod ) );
	pTable->Add( COLOR_ENTRY( color::html::Gold2 ) );
	pTable->Add( COLOR_ENTRY( color::html::Gold1 ) );
	pTable->Add( COLOR_ENTRY( color::html::Goldenrod1 ) );
	pTable->Add( COLOR_ENTRY( color::html::Goldenrod2 ) );
	pTable->Add( COLOR_ENTRY( color::html::Gold ) );
	pTable->Add( COLOR_ENTRY( color::html::Gold3 ) );
	pTable->Add( COLOR_ENTRY( color::html::Goldenrod3 ) );
	pTable->Add( COLOR_ENTRY( color::html::DarkGoldenrod ) );
	pTable->Add( COLOR_ENTRY( color::html::Khaki ) );
	pTable->Add( COLOR_ENTRY( color::html::Khaki3 ) );
	pTable->Add( COLOR_ENTRY( color::html::Khaki4 ) );
	pTable->Add( COLOR_ENTRY( color::html::DarkGoldenrod1 ) );
	pTable->Add( COLOR_ENTRY( color::html::DarkGoldenrod2 ) );
	pTable->Add( COLOR_ENTRY( color::html::DarkGoldenrod3 ) );
	pTable->Add( COLOR_ENTRY( color::html::Sienna1 ) );
	pTable->Add( COLOR_ENTRY( color::html::Sienna2 ) );
	pTable->Add( COLOR_ENTRY( color::html::DarkOrange ) );
	pTable->Add( COLOR_ENTRY( color::html::DarkOrange1 ) );
	pTable->Add( COLOR_ENTRY( color::html::DarkOrange2 ) );
	pTable->Add( COLOR_ENTRY( color::html::DarkOrange3 ) );
	pTable->Add( COLOR_ENTRY( color::html::Sienna3 ) );
	pTable->Add( COLOR_ENTRY( color::html::Sienna ) );
	pTable->Add( COLOR_ENTRY( color::html::Sienna4 ) );
	pTable->Add( COLOR_ENTRY( color::html::IndianRed4 ) );
	pTable->Add( COLOR_ENTRY( color::html::DarkOrange4 ) );
	pTable->Add( COLOR_ENTRY( color::html::Salmon4 ) );
	pTable->Add( COLOR_ENTRY( color::html::DarkGoldenrod4 ) );
	pTable->Add( COLOR_ENTRY( color::html::Gold4 ) );
	pTable->Add( COLOR_ENTRY( color::html::Goldenrod4 ) );
	pTable->Add( COLOR_ENTRY( color::html::LightSalmon4 ) );
	pTable->Add( COLOR_ENTRY( color::html::Chocolate ) );
	pTable->Add( COLOR_ENTRY( color::html::Coral3 ) );
	pTable->Add( COLOR_ENTRY( color::html::Coral2 ) );
	pTable->Add( COLOR_ENTRY( color::html::Coral ) );
	pTable->Add( COLOR_ENTRY( color::html::DarkSalmon ) );
	pTable->Add( COLOR_ENTRY( color::html::Salmon1 ) );
	pTable->Add( COLOR_ENTRY( color::html::Salmon2 ) );
	pTable->Add( COLOR_ENTRY( color::html::Salmon3 ) );
	pTable->Add( COLOR_ENTRY( color::html::LightSalmon3 ) );
	pTable->Add( COLOR_ENTRY( color::html::LightSalmon2 ) );
	pTable->Add( COLOR_ENTRY( color::html::LightSalmon ) );
	pTable->Add( COLOR_ENTRY( color::html::SandyBrown ) );
	pTable->Add( COLOR_ENTRY( color::html::HotPink ) );
	pTable->Add( COLOR_ENTRY( color::html::HotPink1 ) );
	pTable->Add( COLOR_ENTRY( color::html::HotPink2 ) );
	pTable->Add( COLOR_ENTRY( color::html::HotPink3 ) );
	pTable->Add( COLOR_ENTRY( color::html::HotPink4 ) );
	pTable->Add( COLOR_ENTRY( color::html::LightCoral ) );
	pTable->Add( COLOR_ENTRY( color::html::IndianRed1 ) );
	pTable->Add( COLOR_ENTRY( color::html::IndianRed2 ) );
	pTable->Add( COLOR_ENTRY( color::html::IndianRed3 ) );
	pTable->Add( COLOR_ENTRY( color::html::Red ) );
	pTable->Add( COLOR_ENTRY( color::html::Red1 ) );
	pTable->Add( COLOR_ENTRY( color::html::Red2 ) );
	pTable->Add( COLOR_ENTRY( color::html::Firebrick1 ) );
	pTable->Add( COLOR_ENTRY( color::html::Firebrick2 ) );
	pTable->Add( COLOR_ENTRY( color::html::Firebrick3 ) );
	pTable->Add( COLOR_ENTRY( color::html::Pink ) );
	pTable->Add( COLOR_ENTRY( color::html::RosyBrown1 ) );
	pTable->Add( COLOR_ENTRY( color::html::RosyBrown2 ) );
	pTable->Add( COLOR_ENTRY( color::html::Pink2 ) );
	pTable->Add( COLOR_ENTRY( color::html::LightPink ) );
	pTable->Add( COLOR_ENTRY( color::html::LightPink1 ) );
	pTable->Add( COLOR_ENTRY( color::html::LightPink2 ) );
	pTable->Add( COLOR_ENTRY( color::html::Pink3 ) );
	pTable->Add( COLOR_ENTRY( color::html::RosyBrown3 ) );
	pTable->Add( COLOR_ENTRY( color::html::RosyBrown ) );
	pTable->Add( COLOR_ENTRY( color::html::LightPink3 ) );
	pTable->Add( COLOR_ENTRY( color::html::RosyBrown4 ) );
	pTable->Add( COLOR_ENTRY( color::html::LightPink4 ) );
	pTable->Add( COLOR_ENTRY( color::html::Pink4 ) );
	pTable->Add( COLOR_ENTRY( color::html::LavenderBlush4 ) );
	pTable->Add( COLOR_ENTRY( color::html::LightGoldenrod4 ) );
	pTable->Add( COLOR_ENTRY( color::html::LemonChiffon4 ) );
	pTable->Add( COLOR_ENTRY( color::html::LemonChiffon3 ) );
	pTable->Add( COLOR_ENTRY( color::html::LightGoldenrod3 ) );
	pTable->Add( COLOR_ENTRY( color::html::LightGolden2 ) );
	pTable->Add( COLOR_ENTRY( color::html::LightGoldenrod ) );
	pTable->Add( COLOR_ENTRY( color::html::LightGoldenrod1 ) );
	pTable->Add( COLOR_ENTRY( color::html::BlanchedAlmond ) );
	pTable->Add( COLOR_ENTRY( color::html::LemonChiffon2 ) );
	pTable->Add( COLOR_ENTRY( color::html::LemonChiffon ) );
	pTable->Add( COLOR_ENTRY( color::html::LightGoldenrodYellow ) );
	pTable->Add( COLOR_ENTRY( color::html::Cornsilk ) );
	pTable->Add( COLOR_ENTRY( color::html::White ) );

	return pTable;
}

CColorTable* CColorRepository::MakeTable_X11( void )
{
	CColorTable* pTable = new CColorTable( ui::X11_Colors, BaseId_HTML, color::x11::_X11_ColorCount, 14 );		// 140 colors: 10 columns x 14 rows

	pTable->Add( COLOR_ENTRY( color::x11::Black ) );
	pTable->Add( COLOR_ENTRY( color::x11::DarkSlateGray ) );
	pTable->Add( COLOR_ENTRY( color::x11::SlateGray ) );
	pTable->Add( COLOR_ENTRY( color::x11::LightSlateGray ) );
	pTable->Add( COLOR_ENTRY( color::x11::DimGray ) );
	pTable->Add( COLOR_ENTRY( color::x11::Gray ) );
	pTable->Add( COLOR_ENTRY( color::x11::DarkGray ) );
	pTable->Add( COLOR_ENTRY( color::x11::Silver ) );
	pTable->Add( COLOR_ENTRY( color::x11::LightGrey ) );
	pTable->Add( COLOR_ENTRY( color::x11::Gainsboro ) );
	pTable->Add( COLOR_ENTRY( color::x11::IndianRed ) );
	pTable->Add( COLOR_ENTRY( color::x11::LightCoral ) );
	pTable->Add( COLOR_ENTRY( color::x11::Salmon ) );
	pTable->Add( COLOR_ENTRY( color::x11::DarkSalmon ) );
	pTable->Add( COLOR_ENTRY( color::x11::LightSalmon ) );
	pTable->Add( COLOR_ENTRY( color::x11::Red ) );
	pTable->Add( COLOR_ENTRY( color::x11::Crimson ) );
	pTable->Add( COLOR_ENTRY( color::x11::FireBrick ) );
	pTable->Add( COLOR_ENTRY( color::x11::DarkRed ) );
	pTable->Add( COLOR_ENTRY( color::x11::Pink ) );
	pTable->Add( COLOR_ENTRY( color::x11::LightPink ) );
	pTable->Add( COLOR_ENTRY( color::x11::HotPink ) );
	pTable->Add( COLOR_ENTRY( color::x11::DeepPink ) );
	pTable->Add( COLOR_ENTRY( color::x11::MediumVioletRed ) );
	pTable->Add( COLOR_ENTRY( color::x11::PaleVioletRed ) );
	pTable->Add( COLOR_ENTRY( color::x11::LightSalmon2 ) );
	pTable->Add( COLOR_ENTRY( color::x11::Coral ) );
	pTable->Add( COLOR_ENTRY( color::x11::Tomato ) );
	pTable->Add( COLOR_ENTRY( color::x11::OrangeRed ) );
	pTable->Add( COLOR_ENTRY( color::x11::DarkOrange ) );
	pTable->Add( COLOR_ENTRY( color::x11::Orange ) );
	pTable->Add( COLOR_ENTRY( color::x11::Gold ) );
	pTable->Add( COLOR_ENTRY( color::x11::Yellow ) );
	pTable->Add( COLOR_ENTRY( color::x11::LightYellow ) );
	pTable->Add( COLOR_ENTRY( color::x11::LemonChiffon ) );
	pTable->Add( COLOR_ENTRY( color::x11::LightGoldenrodYellow ) );
	pTable->Add( COLOR_ENTRY( color::x11::PapayaWhip ) );
	pTable->Add( COLOR_ENTRY( color::x11::Moccasin ) );
	pTable->Add( COLOR_ENTRY( color::x11::PeachPuff ) );
	pTable->Add( COLOR_ENTRY( color::x11::PaleGoldenrod ) );
	pTable->Add( COLOR_ENTRY( color::x11::Khaki ) );
	pTable->Add( COLOR_ENTRY( color::x11::DarkKhaki ) );
	pTable->Add( COLOR_ENTRY( color::x11::Lavender ) );
	pTable->Add( COLOR_ENTRY( color::x11::Thistle ) );
	pTable->Add( COLOR_ENTRY( color::x11::Plum ) );
	pTable->Add( COLOR_ENTRY( color::x11::Violet ) );
	pTable->Add( COLOR_ENTRY( color::x11::Orchid ) );
	pTable->Add( COLOR_ENTRY( color::x11::Fuchsia ) );
	pTable->Add( COLOR_ENTRY( color::x11::Magenta ) );
	pTable->Add( COLOR_ENTRY( color::x11::MediumOrchid ) );
	pTable->Add( COLOR_ENTRY( color::x11::MediumPurple ) );
	pTable->Add( COLOR_ENTRY( color::x11::BlueViolet ) );
	pTable->Add( COLOR_ENTRY( color::x11::DarkViolet ) );
	pTable->Add( COLOR_ENTRY( color::x11::DarkOrchid ) );
	pTable->Add( COLOR_ENTRY( color::x11::DarkMagenta ) );
	pTable->Add( COLOR_ENTRY( color::x11::Purple ) );
	pTable->Add( COLOR_ENTRY( color::x11::Indigo ) );
	pTable->Add( COLOR_ENTRY( color::x11::DarkSlateBlue ) );
	pTable->Add( COLOR_ENTRY( color::x11::SlateBlue ) );
	pTable->Add( COLOR_ENTRY( color::x11::MediumSlateBlue ) );
	pTable->Add( COLOR_ENTRY( color::x11::GreenYellow ) );
	pTable->Add( COLOR_ENTRY( color::x11::Chartreuse ) );
	pTable->Add( COLOR_ENTRY( color::x11::LawnGreen ) );
	pTable->Add( COLOR_ENTRY( color::x11::Lime ) );
	pTable->Add( COLOR_ENTRY( color::x11::LimeGreen ) );
	pTable->Add( COLOR_ENTRY( color::x11::PaleGreen ) );
	pTable->Add( COLOR_ENTRY( color::x11::LightGreen ) );
	pTable->Add( COLOR_ENTRY( color::x11::MediumSpringGreen ) );
	pTable->Add( COLOR_ENTRY( color::x11::SpringGreen ) );
	pTable->Add( COLOR_ENTRY( color::x11::MediumSeaGreen ) );
	pTable->Add( COLOR_ENTRY( color::x11::SeaGreen ) );
	pTable->Add( COLOR_ENTRY( color::x11::ForestGreen ) );
	pTable->Add( COLOR_ENTRY( color::x11::Green ) );
	pTable->Add( COLOR_ENTRY( color::x11::DarkGreen ) );
	pTable->Add( COLOR_ENTRY( color::x11::YellowGreen ) );
	pTable->Add( COLOR_ENTRY( color::x11::OliveDrab ) );
	pTable->Add( COLOR_ENTRY( color::x11::Olive ) );
	pTable->Add( COLOR_ENTRY( color::x11::DarkOliveGreen ) );
	pTable->Add( COLOR_ENTRY( color::x11::MediumAquamarine ) );
	pTable->Add( COLOR_ENTRY( color::x11::DarkSeaGreen ) );
	pTable->Add( COLOR_ENTRY( color::x11::LightSeaGreen ) );
	pTable->Add( COLOR_ENTRY( color::x11::DarkCyan ) );
	pTable->Add( COLOR_ENTRY( color::x11::Teal ) );
	pTable->Add( COLOR_ENTRY( color::x11::Aqua ) );
	pTable->Add( COLOR_ENTRY( color::x11::Cyan ) );
	pTable->Add( COLOR_ENTRY( color::x11::LightCyan ) );
	pTable->Add( COLOR_ENTRY( color::x11::PaleTurquoise ) );
	pTable->Add( COLOR_ENTRY( color::x11::Aquamarine ) );
	pTable->Add( COLOR_ENTRY( color::x11::Turquoise ) );
	pTable->Add( COLOR_ENTRY( color::x11::MediumTurquoise ) );
	pTable->Add( COLOR_ENTRY( color::x11::DarkTurquoise ) );
	pTable->Add( COLOR_ENTRY( color::x11::CadetBlue ) );
	pTable->Add( COLOR_ENTRY( color::x11::SteelBlue ) );
	pTable->Add( COLOR_ENTRY( color::x11::LightSteelBlue ) );
	pTable->Add( COLOR_ENTRY( color::x11::PowderBlue ) );
	pTable->Add( COLOR_ENTRY( color::x11::LightBlue ) );
	pTable->Add( COLOR_ENTRY( color::x11::SkyBlue ) );
	pTable->Add( COLOR_ENTRY( color::x11::LightSkyBlue ) );
	pTable->Add( COLOR_ENTRY( color::x11::DeepSkyBlue ) );
	pTable->Add( COLOR_ENTRY( color::x11::DodgerBlue ) );
	pTable->Add( COLOR_ENTRY( color::x11::CornflowerBlue ) );
	pTable->Add( COLOR_ENTRY( color::x11::RoyalBlue ) );
	pTable->Add( COLOR_ENTRY( color::x11::Blue ) );
	pTable->Add( COLOR_ENTRY( color::x11::MediumBlue ) );
	pTable->Add( COLOR_ENTRY( color::x11::DarkBlue ) );
	pTable->Add( COLOR_ENTRY( color::x11::Navy ) );
	pTable->Add( COLOR_ENTRY( color::x11::MidnightBlue ) );
	pTable->Add( COLOR_ENTRY( color::x11::Cornsilk ) );
	pTable->Add( COLOR_ENTRY( color::x11::BlanchedAlmond ) );
	pTable->Add( COLOR_ENTRY( color::x11::Bisque ) );
	pTable->Add( COLOR_ENTRY( color::x11::NavajoWhite ) );
	pTable->Add( COLOR_ENTRY( color::x11::Wheat ) );
	pTable->Add( COLOR_ENTRY( color::x11::BurlyWood ) );
	pTable->Add( COLOR_ENTRY( color::x11::Tan ) );
	pTable->Add( COLOR_ENTRY( color::x11::RosyBrown ) );
	pTable->Add( COLOR_ENTRY( color::x11::SandyBrown ) );
	pTable->Add( COLOR_ENTRY( color::x11::Goldenrod ) );
	pTable->Add( COLOR_ENTRY( color::x11::DarkGoldenrod ) );
	pTable->Add( COLOR_ENTRY( color::x11::Peru ) );
	pTable->Add( COLOR_ENTRY( color::x11::Chocolate ) );
	pTable->Add( COLOR_ENTRY( color::x11::SaddleBrown ) );
	pTable->Add( COLOR_ENTRY( color::x11::Sienna ) );
	pTable->Add( COLOR_ENTRY( color::x11::Brown ) );
	pTable->Add( COLOR_ENTRY( color::x11::Maroon ) );
	pTable->Add( COLOR_ENTRY( color::x11::MistyRose ) );
	pTable->Add( COLOR_ENTRY( color::x11::LavenderBlush ) );
	pTable->Add( COLOR_ENTRY( color::x11::Linen ) );
	pTable->Add( COLOR_ENTRY( color::x11::AntiqueWhite ) );
	pTable->Add( COLOR_ENTRY( color::x11::Ivory ) );
	pTable->Add( COLOR_ENTRY( color::x11::FloralWhite ) );
	pTable->Add( COLOR_ENTRY( color::x11::OldLace ) );
	pTable->Add( COLOR_ENTRY( color::x11::Beige ) );
	pTable->Add( COLOR_ENTRY( color::x11::Seashell ) );
	pTable->Add( COLOR_ENTRY( color::x11::WhiteSmoke ) );
	pTable->Add( COLOR_ENTRY( color::x11::GhostWhite ) );
	pTable->Add( COLOR_ENTRY( color::x11::AliceBlue ) );
	pTable->Add( COLOR_ENTRY( color::x11::Azure ) );
	pTable->Add( COLOR_ENTRY( color::x11::MintCream ) );
	pTable->Add( COLOR_ENTRY( color::x11::Honeydew ) );
	pTable->Add( COLOR_ENTRY( color::x11::White ) );

	return pTable;
}

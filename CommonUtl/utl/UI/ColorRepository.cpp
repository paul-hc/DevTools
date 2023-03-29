
#include "stdafx.h"
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
	const CEnumTags& GetTags_ColorBatch( void )
	{
		static const CEnumTags s_tags( _T("System|Standard|Custom|DirectX|HTML|X11|Shades|User") );
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

CColorEntry::CColorEntry( COLORREF color, const char* pLiteral )
	: m_color( color )
	, m_name( word::ToSpacedWordBreaks( str::FromAnsi( FindScopedLiteral( pLiteral ) ).c_str() ) )
	, m_pBatch( nullptr )
{
}

const char* CColorEntry::FindScopedLiteral( const char* pScopedColorName )
{
	static const std::string s_scopeOp = "::";
	size_t posLastScope = str::FindLast<str::Case>( pScopedColorName, s_scopeOp.c_str(), s_scopeOp.length() );

	return posLastScope != std::string::npos ? ( pScopedColorName + posLastScope + s_scopeOp.length() ) : pScopedColorName;
}


// CColorBatch implementation

CColorBatch::CColorBatch( ui::ColorBatch batchType, UINT baseCmdId, size_t capacity, int layoutCount /*= 1*/ )
	: m_batchType( batchType )
	, m_baseCmdId( baseCmdId )
	, m_layoutCount( layoutCount )
{
	m_colors.reserve( capacity );
	ENSURE( m_baseCmdId != 0 );
	ENSURE( m_layoutCount != 0 );
}

CColorBatch::~CColorBatch()
{
}

const std::tstring& CColorBatch::GetBatchName( void ) const
{
	return ui::GetTags_ColorBatch().FormatUi( m_batchType );
}

void CColorBatch::Add( const CColorEntry& colorEntry )
{
	m_colors.push_back( colorEntry );
	m_colors.back().m_pBatch = this;
}

const CColorEntry* CColorBatch::FindColor( COLORREF rawColor ) const
{
	std::vector<CColorEntry>::const_iterator itFound = std::find( m_colors.begin(), m_colors.end(), rawColor );
	if ( itFound == m_colors.end() )
		return nullptr;

	return &*itFound;
}

size_t CColorBatch::FindCmdIndex( UINT cmdId ) const
{
	size_t foundIndex = cmdId - m_baseCmdId;
	return foundIndex < m_colors.size() ? foundIndex : utl::npos;
}

void CColorBatch::GetLayout( OUT size_t* pRowCount, OUT size_t* pColumnCount ) const
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

CColorBatch* CColorBatch::MakeShadesBatch( size_t shadesCount, COLORREF selColor )
{
	if ( 0 == shadesCount || ui::IsUndefinedColor( selColor ) )
		return nullptr;

	selColor = ui::EvalColor( selColor );

	CColorBatch* pShadesBatch = new CColorBatch( ui::Shades_Colors, CColorRepository::BaseId_Shades, shadesCount );

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
			pShadesBatch->Add( colorEntry );
		}

	if ( ui::HasLuminanceVariation( selColor, -percentages[ 0 ], -percentages[ 1 ] ) )		// some variation?
		for ( unsigned int i = 0; i != shadesCount; ++i )
		{
			COLORREF darker = selColor;
			ui::AdjustLuminance( darker, -percentages[ i ] );

			CColorEntry colorEntry( darker, str::Format( _T("%s %d%%"), s_shadeTags[ Lightness ].c_str(), -percentages[i] ) );
			pShadesBatch->Add( colorEntry );
		}

	if ( ui::HasSaturationVariation( selColor, -percentages[ 0 ], -percentages[ 1 ] ) )		// some variation?
		for ( unsigned int i = 0; i != shadesCount; ++i )
		{
			COLORREF desaturated = selColor;
			ui::AdjustLuminance( desaturated, -percentages[ i ] );

			CColorEntry colorEntry( desaturated, str::Format( _T("%s %d%%"), s_shadeTags[ Saturation ].c_str(), -percentages[i] ) );
			pShadesBatch->Add( colorEntry );
		}

	return pShadesBatch;
}


// CColorBatchGroup implementation

const CColorBatch* CColorBatchGroup::FindBatch( ui::ColorBatch batchType ) const
{
	for ( std::vector<CColorBatch*>::const_iterator itBatch = m_colorBatches.begin(); itBatch != m_colorBatches.end(); ++itBatch )
		if ( (*itBatch)->GetBatchType() == batchType )
			return *itBatch;

	return nullptr;
}

const CColorEntry* CColorBatchGroup::FindColorEntry( COLORREF rawColor ) const
{
	if ( !ui::IsUndefinedColor( rawColor ) )		// a repo color?
	{
		if ( ui::IsSysColor( rawColor ) )
		{	// look it up only in sys-colors batch
			const CColorBatch* pSysColors = FindSystemBatch();
			return pSysColors != nullptr ? pSysColors->FindColor( rawColor ) : nullptr;
		}

		rawColor = ui::EvalColor( rawColor );		// for the rest of the batches search for evaluated color

		for ( std::vector<CColorBatch*>::const_iterator itBatch = m_colorBatches.begin(); itBatch != m_colorBatches.end(); ++itBatch )
			if ( (*itBatch)->GetBatchType() != ui::System_Colors )		// look only in batches with real colors
				if ( const CColorEntry* pFound = ( *itBatch )->FindColor( rawColor ) )
					return pFound;
	}

	return nullptr;
}

void CColorBatchGroup::QueryMatchingColors( OUT std::vector<const CColorEntry*>& rColorEntries, COLORREF rawColor ) const
{
	if ( !ui::IsUndefinedColor( rawColor ) )				// color has entry?
	{
		if ( ui::IsSysColor( rawColor ) )
		{	// look it up only in sys-colors batch
			if ( const CColorBatch* pSysColors = FindSystemBatch() )
				if ( const CColorEntry* pFound = pSysColors->FindColor( rawColor ) )
					rColorEntries.push_back( pFound );
		}
		else
		{
			rawColor = ui::EvalColor( rawColor );			// for the rest of the batches search for evaluated color

			for ( std::vector<CColorBatch*>::const_iterator itBatch = m_colorBatches.begin(); itBatch != m_colorBatches.end(); ++itBatch )
				if ( (*itBatch)->GetBatchType() != ui::System_Colors )		// look only in batches with real colors
					if ( const CColorEntry* pFound = ( *itBatch )->FindColor( rawColor ) )
						rColorEntries.push_back( pFound );
		}
	}
}

std::tstring CColorBatchGroup::FormatColorMatch( COLORREF rawColor, bool multiple /*= true*/ ) const
{
	std::vector<const CColorEntry*> colorEntries;

	if ( multiple )
		QueryMatchingColors( colorEntries, rawColor );
	else if ( const CColorEntry* pFoundEntry = FindColorEntry( rawColor ) )
		colorEntries.push_back( pFoundEntry );

	static const TCHAR s_sep[] = _T("  ");
	func::ToQualifiedColorName toQualifiedName;
	std::tstring outText;

	if ( !colorEntries.empty() )
		if ( colorEntries.size() > 1 )
		{
			std::unordered_set<std::tstring> colorNames;
			std::transform( colorEntries.begin(), colorEntries.end(), std::inserter( colorNames, colorNames.begin() ), func::ToColorName() );

			if ( 1 == colorNames.size() )		// single shared color name?
				outText = *colorNames.begin() + s_sep + str::Enquote( str::Join( colorEntries, _T(", "), func::ToBatchName() ).c_str(), _T("("), _T(")") );
			else
				outText = str::Join( colorEntries, _T(", "), toQualifiedName );
		}
		else
			outText = toQualifiedName( colorEntries.front(), s_sep );

	stream::Tag( outText, ui::FormatColor( rawColor ), s_sep );
	return outText;
}


// CColorRepository implementation

CColorRepository::CColorRepository( void )
{
	// add color batches in ui::ColorBatch order:
	m_colorBatches.push_back( MakeBatch_System() );
	m_colorBatches.push_back( MakeBatch_Standard() );
	m_colorBatches.push_back( MakeBatch_Custom() );
	m_colorBatches.push_back( MakeBatch_DirectX() );
	m_colorBatches.push_back( MakeBatch_HTML() );
	m_colorBatches.push_back( MakeBatch_X11() );
}

const CColorRepository* CColorRepository::Instance( void )
{
	static const CColorRepository s_colorRepo;
	return &s_colorRepo;
}

void CColorRepository::Clear( void )
{
	utl::ClearOwningContainer( m_colorBatches );
}


CColorBatch* CColorRepository::MakeBatch_System( void )
{
	CColorBatch* pSysBatch = new CColorBatch( ui::System_Colors, BaseId_System, 31, 2 );		// 31 colors: 2 columns x 16 rows

	pSysBatch->Add( CColorEntry( COLOR_BACKGROUND, _T("Desktop Background") ) );
	pSysBatch->Add( CColorEntry( COLOR_WINDOW, _T("Window Background") ) );
	pSysBatch->Add( CColorEntry( COLOR_WINDOWTEXT, _T("Window Text") ) );
	pSysBatch->Add( CColorEntry( COLOR_WINDOWFRAME, _T("Window Frame") ) );
	pSysBatch->Add( CColorEntry( COLOR_BTNFACE, _T("Button Face") ) );
	pSysBatch->Add( CColorEntry( COLOR_BTNHIGHLIGHT, _T("Button Highlight") ) );
	pSysBatch->Add( CColorEntry( COLOR_BTNSHADOW, _T("Button Shadow") ) );
	pSysBatch->Add( CColorEntry( COLOR_BTNTEXT, _T("Button Text") ) );
	pSysBatch->Add( CColorEntry( COLOR_GRAYTEXT, _T("Disabled Text") ) );
	pSysBatch->Add( CColorEntry( COLOR_3DDKSHADOW, _T("3D Dark Shadow") ) );
	pSysBatch->Add( CColorEntry( COLOR_3DLIGHT, _T("3D Light Color") ) );
	pSysBatch->Add( CColorEntry( COLOR_HIGHLIGHT, _T("Selected") ) );
	pSysBatch->Add( CColorEntry( COLOR_HIGHLIGHTTEXT, _T("Selected Text") ) );
	pSysBatch->Add( CColorEntry( COLOR_CAPTIONTEXT, _T("Scroll Bar Arrow") ) );
	pSysBatch->Add( CColorEntry( COLOR_HOTLIGHT, _T("Hot-Track") ) );
	pSysBatch->Add( CColorEntry( COLOR_INFOBK, _T("Tooltip Background") ) );
	pSysBatch->Add( CColorEntry( COLOR_INFOTEXT, _T("Tooltip Text") ) );
	pSysBatch->Add( CColorEntry( COLOR_SCROLLBAR, _T("Scroll Bar Gray Area") ) );
	pSysBatch->Add( CColorEntry( COLOR_MENU, _T("Menu Background") ) );
	pSysBatch->Add( CColorEntry( COLOR_MENUBAR, _T("Flat Menu Bar") ) );
	pSysBatch->Add( CColorEntry( COLOR_MENUHILIGHT, _T("Menu Highlight") ) );
	pSysBatch->Add( CColorEntry( COLOR_MENUTEXT, _T("Menu Text") ) );
	pSysBatch->Add( CColorEntry( COLOR_ACTIVECAPTION, _T("Active Window Title Bar") ) );
	pSysBatch->Add( CColorEntry( COLOR_GRADIENTACTIVECAPTION, _T("Active Window Gradient") ) );
	pSysBatch->Add( CColorEntry( COLOR_ACTIVEBORDER, _T("Active Window Border") ) );
	pSysBatch->Add( CColorEntry( COLOR_INACTIVECAPTION, _T("Inactive Window Caption") ) );
	pSysBatch->Add( CColorEntry( COLOR_GRADIENTINACTIVECAPTION, _T("Inactive Window Gradient") ) );
	pSysBatch->Add( CColorEntry( COLOR_INACTIVEBORDER, _T("Inactive Window Border") ) );
	pSysBatch->Add( CColorEntry( COLOR_INACTIVECAPTIONTEXT, _T("Inactive Window Caption Text") ) );
	pSysBatch->Add( CColorEntry( COLOR_APPWORKSPACE, _T("MDI Background") ) );
	pSysBatch->Add( CColorEntry( COLOR_DESKTOP, _T("Desktop Background") ) );

	return pSysBatch;
}

CColorBatch* CColorRepository::MakeBatch_Standard( void )
{
	CColorBatch* pBatch = new CColorBatch( ui::Standard_Colors, BaseId_Standard, color::_Standard_ColorCount, 8 );		// 40 colors: 8 columns x 6 rows

	pBatch->Add( COLOR_ENTRY( color::Black ) );
	pBatch->Add( COLOR_ENTRY( color::DarkRed ) );
	pBatch->Add( COLOR_ENTRY( color::Red ) );
	pBatch->Add( COLOR_ENTRY( color::Magenta ) );
	pBatch->Add( COLOR_ENTRY( color::Rose ) );
	pBatch->Add( COLOR_ENTRY( color::Brown ) );
	pBatch->Add( COLOR_ENTRY( color::Orange ) );
	pBatch->Add( COLOR_ENTRY( color::LightOrange ) );
	pBatch->Add( COLOR_ENTRY( color::Gold ) );
	pBatch->Add( COLOR_ENTRY( color::Tan ) );
	pBatch->Add( COLOR_ENTRY( color::OliveGreen ) );
	pBatch->Add( COLOR_ENTRY( color::DarkYellow ) );
	pBatch->Add( COLOR_ENTRY( color::Lime ) );
	pBatch->Add( COLOR_ENTRY( color::Yellow ) );
	pBatch->Add( COLOR_ENTRY( color::LightYellow ) );
	pBatch->Add( COLOR_ENTRY( color::DarkGreen ) );
	pBatch->Add( COLOR_ENTRY( color::Green ) );
	pBatch->Add( COLOR_ENTRY( color::SeaGreen ) );
	pBatch->Add( COLOR_ENTRY( color::BrightGreen ) );
	pBatch->Add( COLOR_ENTRY( color::LightGreen ) );
	pBatch->Add( COLOR_ENTRY( color::DarkTeal ) );
	pBatch->Add( COLOR_ENTRY( color::Teal ) );
	pBatch->Add( COLOR_ENTRY( color::Aqua ) );
	pBatch->Add( COLOR_ENTRY( color::Turquoise ) );
	pBatch->Add( COLOR_ENTRY( color::LightTurquoise ) );
	pBatch->Add( COLOR_ENTRY( color::DarkBlue ) );
	pBatch->Add( COLOR_ENTRY( color::Blue ) );
	pBatch->Add( COLOR_ENTRY( color::LightBlue ) );
	pBatch->Add( COLOR_ENTRY( color::SkyBlue ) );
	pBatch->Add( COLOR_ENTRY( color::PaleBlue ) );
	pBatch->Add( COLOR_ENTRY( color::Indigo ) );
	pBatch->Add( COLOR_ENTRY( color::BlueGray ) );
	pBatch->Add( COLOR_ENTRY( color::Violet ) );
	pBatch->Add( COLOR_ENTRY( color::Plum ) );
	pBatch->Add( COLOR_ENTRY( color::Lavender ) );
	pBatch->Add( CColorEntry( color::Grey80, "Grey 80%" ) );
	pBatch->Add( CColorEntry( color::Grey50, "Grey 50%" ) );
	pBatch->Add( CColorEntry( color::Grey40, "Grey 40%" ) );
	pBatch->Add( CColorEntry( color::Grey25, "Grey 25%" ) );
	pBatch->Add( COLOR_ENTRY( color::White ) );

	return pBatch;
}

CColorBatch* CColorRepository::MakeBatch_Custom( void )
{
	CColorBatch* pBatch = new CColorBatch( ui::Custom_Colors, BaseId_Custom, color::_Custom_ColorCount, 4 );		// 23 colors: 4 columns x 6 rows

	pBatch->Add( COLOR_ENTRY( color::VeryDarkGrey ) );
	pBatch->Add( COLOR_ENTRY( color::DarkGrey ) );
	pBatch->Add( COLOR_ENTRY( color::LightGrey ) );
	pBatch->Add( COLOR_ENTRY( color::Grey60 ) );
	pBatch->Add( COLOR_ENTRY( color::Cyan ) );
	pBatch->Add( COLOR_ENTRY( color::DarkMagenta ) );
	pBatch->Add( COLOR_ENTRY( color::LightGreenish ) );
	pBatch->Add( COLOR_ENTRY( color::SpringGreen ) );
	pBatch->Add( COLOR_ENTRY( color::NeonGreen ) );
	pBatch->Add( COLOR_ENTRY( color::AzureBlue ) );
	pBatch->Add( COLOR_ENTRY( color::BlueWindows10 ) );
	pBatch->Add( COLOR_ENTRY( color::BlueTextWin10 ) );
	pBatch->Add( COLOR_ENTRY( color::ScarletRed ) );
	pBatch->Add( COLOR_ENTRY( color::Salmon ) );
	pBatch->Add( COLOR_ENTRY( color::Pink ) );
	pBatch->Add( COLOR_ENTRY( color::PastelPink ) );
	pBatch->Add( COLOR_ENTRY( color::LightPastelPink ) );
	pBatch->Add( COLOR_ENTRY( color::ToolStripPink ) );
	pBatch->Add( COLOR_ENTRY( color::TranspPink ) );
	pBatch->Add( COLOR_ENTRY( color::SolidOrange ) );
	pBatch->Add( COLOR_ENTRY( color::Amber ) );
	pBatch->Add( COLOR_ENTRY( color::PaleYellow ) );
	pBatch->Add( COLOR_ENTRY( color::GhostWhite ) );

	return pBatch;
}

CColorBatch* CColorRepository::MakeBatch_DirectX( void )
{
	CColorBatch* pBatch = new CColorBatch( ui::DirectX_Colors, BaseId_DirectX, color::directx::_DirectX_ColorCount, 10 );		// 140 colors: 10 columns x 14 rows

	pBatch->Add( COLOR_ENTRY( color::directx::AliceBlue ) );
	pBatch->Add( COLOR_ENTRY( color::directx::AntiqueWhite ) );
	pBatch->Add( COLOR_ENTRY( color::directx::Aqua ) );
	pBatch->Add( COLOR_ENTRY( color::directx::Aquamarine ) );
	pBatch->Add( COLOR_ENTRY( color::directx::Azure ) );
	pBatch->Add( COLOR_ENTRY( color::directx::Beige ) );
	pBatch->Add( COLOR_ENTRY( color::directx::Bisque ) );
	pBatch->Add( COLOR_ENTRY( color::directx::Black ) );
	pBatch->Add( COLOR_ENTRY( color::directx::BlanchedAlmond ) );
	pBatch->Add( COLOR_ENTRY( color::directx::Blue ) );
	pBatch->Add( COLOR_ENTRY( color::directx::BlueViolet ) );
	pBatch->Add( COLOR_ENTRY( color::directx::Brown ) );
	pBatch->Add( COLOR_ENTRY( color::directx::BurlyWood ) );
	pBatch->Add( COLOR_ENTRY( color::directx::CadetBlue ) );
	pBatch->Add( COLOR_ENTRY( color::directx::Chartreuse ) );
	pBatch->Add( COLOR_ENTRY( color::directx::Chocolate ) );
	pBatch->Add( COLOR_ENTRY( color::directx::Coral ) );
	pBatch->Add( COLOR_ENTRY( color::directx::CornflowerBlue ) );
	pBatch->Add( COLOR_ENTRY( color::directx::Cornsilk ) );
	pBatch->Add( COLOR_ENTRY( color::directx::Crimson ) );
	pBatch->Add( COLOR_ENTRY( color::directx::Cyan ) );
	pBatch->Add( COLOR_ENTRY( color::directx::DarkBlue ) );
	pBatch->Add( COLOR_ENTRY( color::directx::DarkCyan ) );
	pBatch->Add( COLOR_ENTRY( color::directx::DarkGoldenrod ) );
	pBatch->Add( COLOR_ENTRY( color::directx::DarkGray ) );
	pBatch->Add( COLOR_ENTRY( color::directx::DarkGreen ) );
	pBatch->Add( COLOR_ENTRY( color::directx::DarkKhaki ) );
	pBatch->Add( COLOR_ENTRY( color::directx::DarkMagenta ) );
	pBatch->Add( COLOR_ENTRY( color::directx::DarkOliveGreen ) );
	pBatch->Add( COLOR_ENTRY( color::directx::DarkOrange ) );
	pBatch->Add( COLOR_ENTRY( color::directx::DarkOrchid ) );
	pBatch->Add( COLOR_ENTRY( color::directx::DarkRed ) );
	pBatch->Add( COLOR_ENTRY( color::directx::DarkSalmon ) );
	pBatch->Add( COLOR_ENTRY( color::directx::DarkSeaGreen ) );
	pBatch->Add( COLOR_ENTRY( color::directx::DarkSlateBlue ) );
	pBatch->Add( COLOR_ENTRY( color::directx::DarkSlateGray ) );
	pBatch->Add( COLOR_ENTRY( color::directx::DarkTurquoise ) );
	pBatch->Add( COLOR_ENTRY( color::directx::DarkViolet ) );
	pBatch->Add( COLOR_ENTRY( color::directx::DeepPink ) );
	pBatch->Add( COLOR_ENTRY( color::directx::DeepSkyBlue ) );
	pBatch->Add( COLOR_ENTRY( color::directx::DimGray ) );
	pBatch->Add( COLOR_ENTRY( color::directx::DodgerBlue ) );
	pBatch->Add( COLOR_ENTRY( color::directx::Firebrick ) );
	pBatch->Add( COLOR_ENTRY( color::directx::FloralWhite ) );
	pBatch->Add( COLOR_ENTRY( color::directx::ForestGreen ) );
	pBatch->Add( COLOR_ENTRY( color::directx::Fuchsia ) );
	pBatch->Add( COLOR_ENTRY( color::directx::Gainsboro ) );
	pBatch->Add( COLOR_ENTRY( color::directx::GhostWhite ) );
	pBatch->Add( COLOR_ENTRY( color::directx::Gold ) );
	pBatch->Add( COLOR_ENTRY( color::directx::Goldenrod ) );
	pBatch->Add( COLOR_ENTRY( color::directx::Gray ) );
	pBatch->Add( COLOR_ENTRY( color::directx::Green ) );
	pBatch->Add( COLOR_ENTRY( color::directx::GreenYellow ) );
	pBatch->Add( COLOR_ENTRY( color::directx::Honeydew ) );
	pBatch->Add( COLOR_ENTRY( color::directx::HotPink ) );
	pBatch->Add( COLOR_ENTRY( color::directx::IndianRed ) );
	pBatch->Add( COLOR_ENTRY( color::directx::Indigo ) );
	pBatch->Add( COLOR_ENTRY( color::directx::Ivory ) );
	pBatch->Add( COLOR_ENTRY( color::directx::Khaki ) );
	pBatch->Add( COLOR_ENTRY( color::directx::Lavender ) );
	pBatch->Add( COLOR_ENTRY( color::directx::LavenderBlush ) );
	pBatch->Add( COLOR_ENTRY( color::directx::LawnGreen ) );
	pBatch->Add( COLOR_ENTRY( color::directx::LemonChiffon ) );
	pBatch->Add( COLOR_ENTRY( color::directx::LightBlue ) );
	pBatch->Add( COLOR_ENTRY( color::directx::LightCoral ) );
	pBatch->Add( COLOR_ENTRY( color::directx::LightCyan ) );
	pBatch->Add( COLOR_ENTRY( color::directx::LightGoldenrodYellow ) );
	pBatch->Add( COLOR_ENTRY( color::directx::LightGreen ) );
	pBatch->Add( COLOR_ENTRY( color::directx::LightGray ) );
	pBatch->Add( COLOR_ENTRY( color::directx::LightPink ) );
	pBatch->Add( COLOR_ENTRY( color::directx::LightSalmon ) );
	pBatch->Add( COLOR_ENTRY( color::directx::LightSeaGreen ) );
	pBatch->Add( COLOR_ENTRY( color::directx::LightSkyBlue ) );
	pBatch->Add( COLOR_ENTRY( color::directx::LightSlateGray ) );
	pBatch->Add( COLOR_ENTRY( color::directx::LightSteelBlue ) );
	pBatch->Add( COLOR_ENTRY( color::directx::LightYellow ) );
	pBatch->Add( COLOR_ENTRY( color::directx::Lime ) );
	pBatch->Add( COLOR_ENTRY( color::directx::LimeGreen ) );
	pBatch->Add( COLOR_ENTRY( color::directx::Linen ) );
	pBatch->Add( COLOR_ENTRY( color::directx::Magenta ) );
	pBatch->Add( COLOR_ENTRY( color::directx::Maroon ) );
	pBatch->Add( COLOR_ENTRY( color::directx::MediumAquamarine ) );
	pBatch->Add( COLOR_ENTRY( color::directx::MediumBlue ) );
	pBatch->Add( COLOR_ENTRY( color::directx::MediumOrchid ) );
	pBatch->Add( COLOR_ENTRY( color::directx::MediumPurple ) );
	pBatch->Add( COLOR_ENTRY( color::directx::MediumSeaGreen ) );
	pBatch->Add( COLOR_ENTRY( color::directx::MediumSlateBlue ) );
	pBatch->Add( COLOR_ENTRY( color::directx::MediumSpringGreen ) );
	pBatch->Add( COLOR_ENTRY( color::directx::MediumTurquoise ) );
	pBatch->Add( COLOR_ENTRY( color::directx::MediumVioletRed ) );
	pBatch->Add( COLOR_ENTRY( color::directx::MidnightBlue ) );
	pBatch->Add( COLOR_ENTRY( color::directx::MintCream ) );
	pBatch->Add( COLOR_ENTRY( color::directx::MistyRose ) );
	pBatch->Add( COLOR_ENTRY( color::directx::Moccasin ) );
	pBatch->Add( COLOR_ENTRY( color::directx::NavajoWhite ) );
	pBatch->Add( COLOR_ENTRY( color::directx::Navy ) );
	pBatch->Add( COLOR_ENTRY( color::directx::OldLace ) );
	pBatch->Add( COLOR_ENTRY( color::directx::Olive ) );
	pBatch->Add( COLOR_ENTRY( color::directx::OliveDrab ) );
	pBatch->Add( COLOR_ENTRY( color::directx::Orange ) );
	pBatch->Add( COLOR_ENTRY( color::directx::OrangeRed ) );
	pBatch->Add( COLOR_ENTRY( color::directx::Orchid ) );
	pBatch->Add( COLOR_ENTRY( color::directx::PaleGoldenrod ) );
	pBatch->Add( COLOR_ENTRY( color::directx::PaleGreen ) );
	pBatch->Add( COLOR_ENTRY( color::directx::PaleTurquoise ) );
	pBatch->Add( COLOR_ENTRY( color::directx::PaleVioletRed ) );
	pBatch->Add( COLOR_ENTRY( color::directx::PapayaWhip ) );
	pBatch->Add( COLOR_ENTRY( color::directx::PeachPuff ) );
	pBatch->Add( COLOR_ENTRY( color::directx::Peru ) );
	pBatch->Add( COLOR_ENTRY( color::directx::Pink ) );
	pBatch->Add( COLOR_ENTRY( color::directx::Plum ) );
	pBatch->Add( COLOR_ENTRY( color::directx::PowderBlue ) );
	pBatch->Add( COLOR_ENTRY( color::directx::Purple ) );
	pBatch->Add( COLOR_ENTRY( color::directx::Red ) );
	pBatch->Add( COLOR_ENTRY( color::directx::RosyBrown ) );
	pBatch->Add( COLOR_ENTRY( color::directx::RoyalBlue ) );
	pBatch->Add( COLOR_ENTRY( color::directx::SaddleBrown ) );
	pBatch->Add( COLOR_ENTRY( color::directx::Salmon ) );
	pBatch->Add( COLOR_ENTRY( color::directx::SandyBrown ) );
	pBatch->Add( COLOR_ENTRY( color::directx::SeaGreen ) );
	pBatch->Add( COLOR_ENTRY( color::directx::SeaShell ) );
	pBatch->Add( COLOR_ENTRY( color::directx::Sienna ) );
	pBatch->Add( COLOR_ENTRY( color::directx::Silver ) );
	pBatch->Add( COLOR_ENTRY( color::directx::SkyBlue ) );
	pBatch->Add( COLOR_ENTRY( color::directx::SlateBlue ) );
	pBatch->Add( COLOR_ENTRY( color::directx::SlateGray ) );
	pBatch->Add( COLOR_ENTRY( color::directx::Snow ) );
	pBatch->Add( COLOR_ENTRY( color::directx::SpringGreen ) );
	pBatch->Add( COLOR_ENTRY( color::directx::SteelBlue ) );
	pBatch->Add( COLOR_ENTRY( color::directx::Tan ) );
	pBatch->Add( COLOR_ENTRY( color::directx::Teal ) );
	pBatch->Add( COLOR_ENTRY( color::directx::Thistle ) );
	pBatch->Add( COLOR_ENTRY( color::directx::Tomato ) );
	pBatch->Add( COLOR_ENTRY( color::directx::Turquoise ) );
	pBatch->Add( COLOR_ENTRY( color::directx::Violet ) );
	pBatch->Add( COLOR_ENTRY( color::directx::Wheat ) );
	pBatch->Add( COLOR_ENTRY( color::directx::White ) );
	pBatch->Add( COLOR_ENTRY( color::directx::WhiteSmoke ) );
	pBatch->Add( COLOR_ENTRY( color::directx::Yellow ) );
	pBatch->Add( COLOR_ENTRY( color::directx::YellowGreen ) );

	return pBatch;
}

CColorBatch* CColorRepository::MakeBatch_HTML( void )
{
	CColorBatch* pBatch = new CColorBatch( ui::HTML_Colors, BaseId_HTML, color::html::_Html_ColorCount, 1 );		// 300 colors: 10 columns x 30 rows

	pBatch->Add( COLOR_ENTRY( color::html::Black ) );
	pBatch->Add( COLOR_ENTRY( color::html::Gray0 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Gray18 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Gray21 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Gray23 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Gray24 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Gray25 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Gray26 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Gray27 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Gray28 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Gray29 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Gray30 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Gray31 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Gray32 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Gray34 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Gray35 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Gray36 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Gray37 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Gray38 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Gray39 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Gray40 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Gray41 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Gray42 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Gray43 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Gray44 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Gray45 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Gray46 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Gray47 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Gray48 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Gray49 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Gray50 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Gray ) );
	pBatch->Add( COLOR_ENTRY( color::html::SlateGray4 ) );
	pBatch->Add( COLOR_ENTRY( color::html::SlateGray ) );
	pBatch->Add( COLOR_ENTRY( color::html::LightSteelBlue4 ) );
	pBatch->Add( COLOR_ENTRY( color::html::LightSlateGray ) );
	pBatch->Add( COLOR_ENTRY( color::html::CadetBlue4 ) );
	pBatch->Add( COLOR_ENTRY( color::html::DarkSlateGray4 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Thistle4 ) );
	pBatch->Add( COLOR_ENTRY( color::html::MediumSlateBlue ) );
	pBatch->Add( COLOR_ENTRY( color::html::MediumPurple4 ) );
	pBatch->Add( COLOR_ENTRY( color::html::MidnightBlue ) );
	pBatch->Add( COLOR_ENTRY( color::html::DarkSlateBlue ) );
	pBatch->Add( COLOR_ENTRY( color::html::DarkSlateGray ) );
	pBatch->Add( COLOR_ENTRY( color::html::DimGray ) );
	pBatch->Add( COLOR_ENTRY( color::html::CornflowerBlue ) );
	pBatch->Add( COLOR_ENTRY( color::html::RoyalBlue4 ) );
	pBatch->Add( COLOR_ENTRY( color::html::SlateBlue4 ) );
	pBatch->Add( COLOR_ENTRY( color::html::RoyalBlue ) );
	pBatch->Add( COLOR_ENTRY( color::html::RoyalBlue1 ) );
	pBatch->Add( COLOR_ENTRY( color::html::RoyalBlue2 ) );
	pBatch->Add( COLOR_ENTRY( color::html::RoyalBlue3 ) );
	pBatch->Add( COLOR_ENTRY( color::html::DeepSkyBlue ) );
	pBatch->Add( COLOR_ENTRY( color::html::DeepSkyBlue2 ) );
	pBatch->Add( COLOR_ENTRY( color::html::SlateBlue ) );
	pBatch->Add( COLOR_ENTRY( color::html::DeepSkyBlue3 ) );
	pBatch->Add( COLOR_ENTRY( color::html::DeepSkyBlue4 ) );
	pBatch->Add( COLOR_ENTRY( color::html::DodgerBlue ) );
	pBatch->Add( COLOR_ENTRY( color::html::DodgerBlue2 ) );
	pBatch->Add( COLOR_ENTRY( color::html::DodgerBlue3 ) );
	pBatch->Add( COLOR_ENTRY( color::html::DodgerBlue4 ) );
	pBatch->Add( COLOR_ENTRY( color::html::SteelBlue4 ) );
	pBatch->Add( COLOR_ENTRY( color::html::SteelBlue ) );
	pBatch->Add( COLOR_ENTRY( color::html::SlateBlue2 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Violet ) );
	pBatch->Add( COLOR_ENTRY( color::html::MediumPurple3 ) );
	pBatch->Add( COLOR_ENTRY( color::html::MediumPurple ) );
	pBatch->Add( COLOR_ENTRY( color::html::MediumPurple2 ) );
	pBatch->Add( COLOR_ENTRY( color::html::MediumPurple1 ) );
	pBatch->Add( COLOR_ENTRY( color::html::LightSteelBlue ) );
	pBatch->Add( COLOR_ENTRY( color::html::SteelBlue3 ) );
	pBatch->Add( COLOR_ENTRY( color::html::SteelBlue2 ) );
	pBatch->Add( COLOR_ENTRY( color::html::SteelBlue1 ) );
	pBatch->Add( COLOR_ENTRY( color::html::SkyBlue3 ) );
	pBatch->Add( COLOR_ENTRY( color::html::SkyBlue4 ) );
	pBatch->Add( COLOR_ENTRY( color::html::SlateBlue3 ) );
	pBatch->Add( COLOR_ENTRY( color::html::SlateBlue5 ) );
	pBatch->Add( COLOR_ENTRY( color::html::SlateGray3 ) );
	pBatch->Add( COLOR_ENTRY( color::html::VioletRed ) );
	pBatch->Add( COLOR_ENTRY( color::html::VioletRed1 ) );
	pBatch->Add( COLOR_ENTRY( color::html::VioletRed2 ) );
	pBatch->Add( COLOR_ENTRY( color::html::DeepPink ) );
	pBatch->Add( COLOR_ENTRY( color::html::DeepPink2 ) );
	pBatch->Add( COLOR_ENTRY( color::html::DeepPink3 ) );
	pBatch->Add( COLOR_ENTRY( color::html::DeepPink4 ) );
	pBatch->Add( COLOR_ENTRY( color::html::MediumVioletRed ) );
	pBatch->Add( COLOR_ENTRY( color::html::VioletRed3 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Firebrick ) );
	pBatch->Add( COLOR_ENTRY( color::html::VioletRed4 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Maroon4 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Maroon ) );
	pBatch->Add( COLOR_ENTRY( color::html::Maroon3 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Maroon2 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Maroon1 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Magenta ) );
	pBatch->Add( COLOR_ENTRY( color::html::Magenta1 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Magenta2 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Magenta3 ) );
	pBatch->Add( COLOR_ENTRY( color::html::MediumOrchid ) );
	pBatch->Add( COLOR_ENTRY( color::html::MediumOrchid1 ) );
	pBatch->Add( COLOR_ENTRY( color::html::MediumOrchid2 ) );
	pBatch->Add( COLOR_ENTRY( color::html::MediumOrchid3 ) );
	pBatch->Add( COLOR_ENTRY( color::html::MediumOrchid4 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Purple ) );
	pBatch->Add( COLOR_ENTRY( color::html::Purple1 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Purple2 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Purple3 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Purple4 ) );
	pBatch->Add( COLOR_ENTRY( color::html::DarkOrchid4 ) );
	pBatch->Add( COLOR_ENTRY( color::html::DarkOrchid ) );
	pBatch->Add( COLOR_ENTRY( color::html::DarkViolet ) );
	pBatch->Add( COLOR_ENTRY( color::html::DarkOrchid3 ) );
	pBatch->Add( COLOR_ENTRY( color::html::DarkOrchid2 ) );
	pBatch->Add( COLOR_ENTRY( color::html::DarkOrchid1 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Plum4 ) );
	pBatch->Add( COLOR_ENTRY( color::html::PaleVioletRed ) );
	pBatch->Add( COLOR_ENTRY( color::html::PaleVioletRed1 ) );
	pBatch->Add( COLOR_ENTRY( color::html::PaleVioletRed2 ) );
	pBatch->Add( COLOR_ENTRY( color::html::PaleVioletRed3 ) );
	pBatch->Add( COLOR_ENTRY( color::html::PaleVioletRed4 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Plum ) );
	pBatch->Add( COLOR_ENTRY( color::html::Plum1 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Plum2 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Plum3 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Thistle ) );
	pBatch->Add( COLOR_ENTRY( color::html::Thistle3 ) );
	pBatch->Add( COLOR_ENTRY( color::html::LavenderBlush2 ) );
	pBatch->Add( COLOR_ENTRY( color::html::LavenderBlush3 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Thistle2 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Thistle1 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Lavender ) );
	pBatch->Add( COLOR_ENTRY( color::html::LavenderBlush ) );
	pBatch->Add( COLOR_ENTRY( color::html::LightSteelBlue1 ) );
	pBatch->Add( COLOR_ENTRY( color::html::LightBlue ) );
	pBatch->Add( COLOR_ENTRY( color::html::LightBlue1 ) );
	pBatch->Add( COLOR_ENTRY( color::html::LightCyan ) );
	pBatch->Add( COLOR_ENTRY( color::html::SlateGray1 ) );
	pBatch->Add( COLOR_ENTRY( color::html::SlateGray2 ) );
	pBatch->Add( COLOR_ENTRY( color::html::LightSteelBlue2 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Turquoise1 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Cyan ) );
	pBatch->Add( COLOR_ENTRY( color::html::Cyan1 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Cyan2 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Turquoise2 ) );
	pBatch->Add( COLOR_ENTRY( color::html::MediumTurquoise ) );
	pBatch->Add( COLOR_ENTRY( color::html::Turquoise ) );
	pBatch->Add( COLOR_ENTRY( color::html::DarkSlateGray1 ) );
	pBatch->Add( COLOR_ENTRY( color::html::DarkSlateGray2 ) );
	pBatch->Add( COLOR_ENTRY( color::html::DarkSlateGray3 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Cyan3 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Turquoise3 ) );
	pBatch->Add( COLOR_ENTRY( color::html::CadetBlue3 ) );
	pBatch->Add( COLOR_ENTRY( color::html::PaleTurquoise3 ) );
	pBatch->Add( COLOR_ENTRY( color::html::LightBlue2 ) );
	pBatch->Add( COLOR_ENTRY( color::html::DarkTurquoise ) );
	pBatch->Add( COLOR_ENTRY( color::html::Cyan4 ) );
	pBatch->Add( COLOR_ENTRY( color::html::LightSeaGreen ) );
	pBatch->Add( COLOR_ENTRY( color::html::LightSkyBlue ) );
	pBatch->Add( COLOR_ENTRY( color::html::LightSkyBlue2 ) );
	pBatch->Add( COLOR_ENTRY( color::html::LightSkyBlue3 ) );
	pBatch->Add( COLOR_ENTRY( color::html::SkyBlue5 ) );
	pBatch->Add( COLOR_ENTRY( color::html::SkyBlue6 ) );
	pBatch->Add( COLOR_ENTRY( color::html::LightSkyBlue4 ) );
	pBatch->Add( COLOR_ENTRY( color::html::SkyBlue ) );
	pBatch->Add( COLOR_ENTRY( color::html::LightSlateBlue ) );
	pBatch->Add( COLOR_ENTRY( color::html::LightCyan2 ) );
	pBatch->Add( COLOR_ENTRY( color::html::LightCyan3 ) );
	pBatch->Add( COLOR_ENTRY( color::html::LightCyan4 ) );
	pBatch->Add( COLOR_ENTRY( color::html::LightBlue3 ) );
	pBatch->Add( COLOR_ENTRY( color::html::LightBlue4 ) );
	pBatch->Add( COLOR_ENTRY( color::html::PaleTurquoise4 ) );
	pBatch->Add( COLOR_ENTRY( color::html::DarkSeaGreen4 ) );
	pBatch->Add( COLOR_ENTRY( color::html::MediumAquamarine ) );
	pBatch->Add( COLOR_ENTRY( color::html::MediumSeaGreen ) );
	pBatch->Add( COLOR_ENTRY( color::html::SeaGreen ) );
	pBatch->Add( COLOR_ENTRY( color::html::DarkGreen ) );
	pBatch->Add( COLOR_ENTRY( color::html::SeaGreen4 ) );
	pBatch->Add( COLOR_ENTRY( color::html::ForestGreen ) );
	pBatch->Add( COLOR_ENTRY( color::html::MediumForestGreen ) );
	pBatch->Add( COLOR_ENTRY( color::html::SpringGreen4 ) );
	pBatch->Add( COLOR_ENTRY( color::html::DarkOliveGreen4 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Chartreuse4 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Green4 ) );
	pBatch->Add( COLOR_ENTRY( color::html::MediumSpringGreen ) );
	pBatch->Add( COLOR_ENTRY( color::html::SpringGreen ) );
	pBatch->Add( COLOR_ENTRY( color::html::LimeGreen ) );
	pBatch->Add( COLOR_ENTRY( color::html::SpringGreen3 ) );
	pBatch->Add( COLOR_ENTRY( color::html::DarkSeaGreen ) );
	pBatch->Add( COLOR_ENTRY( color::html::DarkSeaGreen3 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Green3 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Chartreuse3 ) );
	pBatch->Add( COLOR_ENTRY( color::html::YellowGreen ) );
	pBatch->Add( COLOR_ENTRY( color::html::SpringGreen5 ) );
	pBatch->Add( COLOR_ENTRY( color::html::SeaGreen3 ) );
	pBatch->Add( COLOR_ENTRY( color::html::SpringGreen2 ) );
	pBatch->Add( COLOR_ENTRY( color::html::SpringGreen1 ) );
	pBatch->Add( COLOR_ENTRY( color::html::SeaGreen2 ) );
	pBatch->Add( COLOR_ENTRY( color::html::SeaGreen1 ) );
	pBatch->Add( COLOR_ENTRY( color::html::DarkSeaGreen2 ) );
	pBatch->Add( COLOR_ENTRY( color::html::DarkSeaGreen1 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Green ) );
	pBatch->Add( COLOR_ENTRY( color::html::LawnGreen ) );
	pBatch->Add( COLOR_ENTRY( color::html::Green1 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Green2 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Chartreuse2 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Chartreuse ) );
	pBatch->Add( COLOR_ENTRY( color::html::GreenYellow ) );
	pBatch->Add( COLOR_ENTRY( color::html::DarkOliveGreen1 ) );
	pBatch->Add( COLOR_ENTRY( color::html::DarkOliveGreen2 ) );
	pBatch->Add( COLOR_ENTRY( color::html::DarkOliveGreen3 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Yellow ) );
	pBatch->Add( COLOR_ENTRY( color::html::Yellow1 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Khaki1 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Khaki2 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Goldenrod ) );
	pBatch->Add( COLOR_ENTRY( color::html::Gold2 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Gold1 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Goldenrod1 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Goldenrod2 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Gold ) );
	pBatch->Add( COLOR_ENTRY( color::html::Gold3 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Goldenrod3 ) );
	pBatch->Add( COLOR_ENTRY( color::html::DarkGoldenrod ) );
	pBatch->Add( COLOR_ENTRY( color::html::Khaki ) );
	pBatch->Add( COLOR_ENTRY( color::html::Khaki3 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Khaki4 ) );
	pBatch->Add( COLOR_ENTRY( color::html::DarkGoldenrod1 ) );
	pBatch->Add( COLOR_ENTRY( color::html::DarkGoldenrod2 ) );
	pBatch->Add( COLOR_ENTRY( color::html::DarkGoldenrod3 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Sienna1 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Sienna2 ) );
	pBatch->Add( COLOR_ENTRY( color::html::DarkOrange ) );
	pBatch->Add( COLOR_ENTRY( color::html::DarkOrange1 ) );
	pBatch->Add( COLOR_ENTRY( color::html::DarkOrange2 ) );
	pBatch->Add( COLOR_ENTRY( color::html::DarkOrange3 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Sienna3 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Sienna ) );
	pBatch->Add( COLOR_ENTRY( color::html::Sienna4 ) );
	pBatch->Add( COLOR_ENTRY( color::html::IndianRed4 ) );
	pBatch->Add( COLOR_ENTRY( color::html::DarkOrange4 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Salmon4 ) );
	pBatch->Add( COLOR_ENTRY( color::html::DarkGoldenrod4 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Gold4 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Goldenrod4 ) );
	pBatch->Add( COLOR_ENTRY( color::html::LightSalmon4 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Chocolate ) );
	pBatch->Add( COLOR_ENTRY( color::html::Coral3 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Coral2 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Coral ) );
	pBatch->Add( COLOR_ENTRY( color::html::DarkSalmon ) );
	pBatch->Add( COLOR_ENTRY( color::html::Salmon1 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Salmon2 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Salmon3 ) );
	pBatch->Add( COLOR_ENTRY( color::html::LightSalmon3 ) );
	pBatch->Add( COLOR_ENTRY( color::html::LightSalmon2 ) );
	pBatch->Add( COLOR_ENTRY( color::html::LightSalmon ) );
	pBatch->Add( COLOR_ENTRY( color::html::SandyBrown ) );
	pBatch->Add( COLOR_ENTRY( color::html::HotPink ) );
	pBatch->Add( COLOR_ENTRY( color::html::HotPink1 ) );
	pBatch->Add( COLOR_ENTRY( color::html::HotPink2 ) );
	pBatch->Add( COLOR_ENTRY( color::html::HotPink3 ) );
	pBatch->Add( COLOR_ENTRY( color::html::HotPink4 ) );
	pBatch->Add( COLOR_ENTRY( color::html::LightCoral ) );
	pBatch->Add( COLOR_ENTRY( color::html::IndianRed1 ) );
	pBatch->Add( COLOR_ENTRY( color::html::IndianRed2 ) );
	pBatch->Add( COLOR_ENTRY( color::html::IndianRed3 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Red ) );
	pBatch->Add( COLOR_ENTRY( color::html::Red1 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Red2 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Firebrick1 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Firebrick2 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Firebrick3 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Pink ) );
	pBatch->Add( COLOR_ENTRY( color::html::RosyBrown1 ) );
	pBatch->Add( COLOR_ENTRY( color::html::RosyBrown2 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Pink2 ) );
	pBatch->Add( COLOR_ENTRY( color::html::LightPink ) );
	pBatch->Add( COLOR_ENTRY( color::html::LightPink1 ) );
	pBatch->Add( COLOR_ENTRY( color::html::LightPink2 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Pink3 ) );
	pBatch->Add( COLOR_ENTRY( color::html::RosyBrown3 ) );
	pBatch->Add( COLOR_ENTRY( color::html::RosyBrown ) );
	pBatch->Add( COLOR_ENTRY( color::html::LightPink3 ) );
	pBatch->Add( COLOR_ENTRY( color::html::RosyBrown4 ) );
	pBatch->Add( COLOR_ENTRY( color::html::LightPink4 ) );
	pBatch->Add( COLOR_ENTRY( color::html::Pink4 ) );
	pBatch->Add( COLOR_ENTRY( color::html::LavenderBlush4 ) );
	pBatch->Add( COLOR_ENTRY( color::html::LightGoldenrod4 ) );
	pBatch->Add( COLOR_ENTRY( color::html::LemonChiffon4 ) );
	pBatch->Add( COLOR_ENTRY( color::html::LemonChiffon3 ) );
	pBatch->Add( COLOR_ENTRY( color::html::LightGoldenrod3 ) );
	pBatch->Add( COLOR_ENTRY( color::html::LightGolden2 ) );
	pBatch->Add( COLOR_ENTRY( color::html::LightGoldenrod ) );
	pBatch->Add( COLOR_ENTRY( color::html::LightGoldenrod1 ) );
	pBatch->Add( COLOR_ENTRY( color::html::BlanchedAlmond ) );
	pBatch->Add( COLOR_ENTRY( color::html::LemonChiffon2 ) );
	pBatch->Add( COLOR_ENTRY( color::html::LemonChiffon ) );
	pBatch->Add( COLOR_ENTRY( color::html::LightGoldenrodYellow ) );
	pBatch->Add( COLOR_ENTRY( color::html::Cornsilk ) );
	pBatch->Add( COLOR_ENTRY( color::html::White ) );

	return pBatch;
}

CColorBatch* CColorRepository::MakeBatch_X11( void )
{
	CColorBatch* pBatch = new CColorBatch( ui::X11_Colors, BaseId_HTML, color::x11::_X11_ColorCount, 1 );		// 140 colors: 10 columns x 14 rows

	pBatch->Add( COLOR_ENTRY( color::x11::Black ) );
	pBatch->Add( COLOR_ENTRY( color::x11::DarkSlateGray ) );
	pBatch->Add( COLOR_ENTRY( color::x11::SlateGray ) );
	pBatch->Add( COLOR_ENTRY( color::x11::LightSlateGray ) );
	pBatch->Add( COLOR_ENTRY( color::x11::DimGray ) );
	pBatch->Add( COLOR_ENTRY( color::x11::Gray ) );
	pBatch->Add( COLOR_ENTRY( color::x11::DarkGray ) );
	pBatch->Add( COLOR_ENTRY( color::x11::Silver ) );
	pBatch->Add( COLOR_ENTRY( color::x11::LightGrey ) );
	pBatch->Add( COLOR_ENTRY( color::x11::Gainsboro ) );
	pBatch->Add( COLOR_ENTRY( color::x11::IndianRed ) );
	pBatch->Add( COLOR_ENTRY( color::x11::LightCoral ) );
	pBatch->Add( COLOR_ENTRY( color::x11::Salmon ) );
	pBatch->Add( COLOR_ENTRY( color::x11::DarkSalmon ) );
	pBatch->Add( COLOR_ENTRY( color::x11::LightSalmon ) );
	pBatch->Add( COLOR_ENTRY( color::x11::Red ) );
	pBatch->Add( COLOR_ENTRY( color::x11::Crimson ) );
	pBatch->Add( COLOR_ENTRY( color::x11::FireBrick ) );
	pBatch->Add( COLOR_ENTRY( color::x11::DarkRed ) );
	pBatch->Add( COLOR_ENTRY( color::x11::Pink ) );
	pBatch->Add( COLOR_ENTRY( color::x11::LightPink ) );
	pBatch->Add( COLOR_ENTRY( color::x11::HotPink ) );
	pBatch->Add( COLOR_ENTRY( color::x11::DeepPink ) );
	pBatch->Add( COLOR_ENTRY( color::x11::MediumVioletRed ) );
	pBatch->Add( COLOR_ENTRY( color::x11::PaleVioletRed ) );
	pBatch->Add( COLOR_ENTRY( color::x11::LightSalmon2 ) );
	pBatch->Add( COLOR_ENTRY( color::x11::Coral ) );
	pBatch->Add( COLOR_ENTRY( color::x11::Tomato ) );
	pBatch->Add( COLOR_ENTRY( color::x11::OrangeRed ) );
	pBatch->Add( COLOR_ENTRY( color::x11::DarkOrange ) );
	pBatch->Add( COLOR_ENTRY( color::x11::Orange ) );
	pBatch->Add( COLOR_ENTRY( color::x11::Gold ) );
	pBatch->Add( COLOR_ENTRY( color::x11::Yellow ) );
	pBatch->Add( COLOR_ENTRY( color::x11::LightYellow ) );
	pBatch->Add( COLOR_ENTRY( color::x11::LemonChiffon ) );
	pBatch->Add( COLOR_ENTRY( color::x11::LightGoldenrodYellow ) );
	pBatch->Add( COLOR_ENTRY( color::x11::PapayaWhip ) );
	pBatch->Add( COLOR_ENTRY( color::x11::Moccasin ) );
	pBatch->Add( COLOR_ENTRY( color::x11::PeachPuff ) );
	pBatch->Add( COLOR_ENTRY( color::x11::PaleGoldenrod ) );
	pBatch->Add( COLOR_ENTRY( color::x11::Khaki ) );
	pBatch->Add( COLOR_ENTRY( color::x11::DarkKhaki ) );
	pBatch->Add( COLOR_ENTRY( color::x11::Lavender ) );
	pBatch->Add( COLOR_ENTRY( color::x11::Thistle ) );
	pBatch->Add( COLOR_ENTRY( color::x11::Plum ) );
	pBatch->Add( COLOR_ENTRY( color::x11::Violet ) );
	pBatch->Add( COLOR_ENTRY( color::x11::Orchid ) );
	pBatch->Add( COLOR_ENTRY( color::x11::Fuchsia ) );
	pBatch->Add( COLOR_ENTRY( color::x11::Magenta ) );
	pBatch->Add( COLOR_ENTRY( color::x11::MediumOrchid ) );
	pBatch->Add( COLOR_ENTRY( color::x11::MediumPurple ) );
	pBatch->Add( COLOR_ENTRY( color::x11::BlueViolet ) );
	pBatch->Add( COLOR_ENTRY( color::x11::DarkViolet ) );
	pBatch->Add( COLOR_ENTRY( color::x11::DarkOrchid ) );
	pBatch->Add( COLOR_ENTRY( color::x11::DarkMagenta ) );
	pBatch->Add( COLOR_ENTRY( color::x11::Purple ) );
	pBatch->Add( COLOR_ENTRY( color::x11::Indigo ) );
	pBatch->Add( COLOR_ENTRY( color::x11::DarkSlateBlue ) );
	pBatch->Add( COLOR_ENTRY( color::x11::SlateBlue ) );
	pBatch->Add( COLOR_ENTRY( color::x11::MediumSlateBlue ) );
	pBatch->Add( COLOR_ENTRY( color::x11::GreenYellow ) );
	pBatch->Add( COLOR_ENTRY( color::x11::Chartreuse ) );
	pBatch->Add( COLOR_ENTRY( color::x11::LawnGreen ) );
	pBatch->Add( COLOR_ENTRY( color::x11::Lime ) );
	pBatch->Add( COLOR_ENTRY( color::x11::LimeGreen ) );
	pBatch->Add( COLOR_ENTRY( color::x11::PaleGreen ) );
	pBatch->Add( COLOR_ENTRY( color::x11::LightGreen ) );
	pBatch->Add( COLOR_ENTRY( color::x11::MediumSpringGreen ) );
	pBatch->Add( COLOR_ENTRY( color::x11::SpringGreen ) );
	pBatch->Add( COLOR_ENTRY( color::x11::MediumSeaGreen ) );
	pBatch->Add( COLOR_ENTRY( color::x11::SeaGreen ) );
	pBatch->Add( COLOR_ENTRY( color::x11::ForestGreen ) );
	pBatch->Add( COLOR_ENTRY( color::x11::Green ) );
	pBatch->Add( COLOR_ENTRY( color::x11::DarkGreen ) );
	pBatch->Add( COLOR_ENTRY( color::x11::YellowGreen ) );
	pBatch->Add( COLOR_ENTRY( color::x11::OliveDrab ) );
	pBatch->Add( COLOR_ENTRY( color::x11::Olive ) );
	pBatch->Add( COLOR_ENTRY( color::x11::DarkOliveGreen ) );
	pBatch->Add( COLOR_ENTRY( color::x11::MediumAquamarine ) );
	pBatch->Add( COLOR_ENTRY( color::x11::DarkSeaGreen ) );
	pBatch->Add( COLOR_ENTRY( color::x11::LightSeaGreen ) );
	pBatch->Add( COLOR_ENTRY( color::x11::DarkCyan ) );
	pBatch->Add( COLOR_ENTRY( color::x11::Teal ) );
	pBatch->Add( COLOR_ENTRY( color::x11::Aqua ) );
	pBatch->Add( COLOR_ENTRY( color::x11::Cyan ) );
	pBatch->Add( COLOR_ENTRY( color::x11::LightCyan ) );
	pBatch->Add( COLOR_ENTRY( color::x11::PaleTurquoise ) );
	pBatch->Add( COLOR_ENTRY( color::x11::Aquamarine ) );
	pBatch->Add( COLOR_ENTRY( color::x11::Turquoise ) );
	pBatch->Add( COLOR_ENTRY( color::x11::MediumTurquoise ) );
	pBatch->Add( COLOR_ENTRY( color::x11::DarkTurquoise ) );
	pBatch->Add( COLOR_ENTRY( color::x11::CadetBlue ) );
	pBatch->Add( COLOR_ENTRY( color::x11::SteelBlue ) );
	pBatch->Add( COLOR_ENTRY( color::x11::LightSteelBlue ) );
	pBatch->Add( COLOR_ENTRY( color::x11::PowderBlue ) );
	pBatch->Add( COLOR_ENTRY( color::x11::LightBlue ) );
	pBatch->Add( COLOR_ENTRY( color::x11::SkyBlue ) );
	pBatch->Add( COLOR_ENTRY( color::x11::LightSkyBlue ) );
	pBatch->Add( COLOR_ENTRY( color::x11::DeepSkyBlue ) );
	pBatch->Add( COLOR_ENTRY( color::x11::DodgerBlue ) );
	pBatch->Add( COLOR_ENTRY( color::x11::CornflowerBlue ) );
	pBatch->Add( COLOR_ENTRY( color::x11::RoyalBlue ) );
	pBatch->Add( COLOR_ENTRY( color::x11::Blue ) );
	pBatch->Add( COLOR_ENTRY( color::x11::MediumBlue ) );
	pBatch->Add( COLOR_ENTRY( color::x11::DarkBlue ) );
	pBatch->Add( COLOR_ENTRY( color::x11::Navy ) );
	pBatch->Add( COLOR_ENTRY( color::x11::MidnightBlue ) );
	pBatch->Add( COLOR_ENTRY( color::x11::Cornsilk ) );
	pBatch->Add( COLOR_ENTRY( color::x11::BlanchedAlmond ) );
	pBatch->Add( COLOR_ENTRY( color::x11::Bisque ) );
	pBatch->Add( COLOR_ENTRY( color::x11::NavajoWhite ) );
	pBatch->Add( COLOR_ENTRY( color::x11::Wheat ) );
	pBatch->Add( COLOR_ENTRY( color::x11::BurlyWood ) );
	pBatch->Add( COLOR_ENTRY( color::x11::Tan ) );
	pBatch->Add( COLOR_ENTRY( color::x11::RosyBrown ) );
	pBatch->Add( COLOR_ENTRY( color::x11::SandyBrown ) );
	pBatch->Add( COLOR_ENTRY( color::x11::Goldenrod ) );
	pBatch->Add( COLOR_ENTRY( color::x11::DarkGoldenrod ) );
	pBatch->Add( COLOR_ENTRY( color::x11::Peru ) );
	pBatch->Add( COLOR_ENTRY( color::x11::Chocolate ) );
	pBatch->Add( COLOR_ENTRY( color::x11::SaddleBrown ) );
	pBatch->Add( COLOR_ENTRY( color::x11::Sienna ) );
	pBatch->Add( COLOR_ENTRY( color::x11::Brown ) );
	pBatch->Add( COLOR_ENTRY( color::x11::Maroon ) );
	pBatch->Add( COLOR_ENTRY( color::x11::MistyRose ) );
	pBatch->Add( COLOR_ENTRY( color::x11::LavenderBlush ) );
	pBatch->Add( COLOR_ENTRY( color::x11::Linen ) );
	pBatch->Add( COLOR_ENTRY( color::x11::AntiqueWhite ) );
	pBatch->Add( COLOR_ENTRY( color::x11::Ivory ) );
	pBatch->Add( COLOR_ENTRY( color::x11::FloralWhite ) );
	pBatch->Add( COLOR_ENTRY( color::x11::OldLace ) );
	pBatch->Add( COLOR_ENTRY( color::x11::Beige ) );
	pBatch->Add( COLOR_ENTRY( color::x11::Seashell ) );
	pBatch->Add( COLOR_ENTRY( color::x11::WhiteSmoke ) );
	pBatch->Add( COLOR_ENTRY( color::x11::GhostWhite ) );
	pBatch->Add( COLOR_ENTRY( color::x11::AliceBlue ) );
	pBatch->Add( COLOR_ENTRY( color::x11::Azure ) );
	pBatch->Add( COLOR_ENTRY( color::x11::MintCream ) );
	pBatch->Add( COLOR_ENTRY( color::x11::Honeydew ) );
	pBatch->Add( COLOR_ENTRY( color::x11::White ) );

	return pBatch;
}

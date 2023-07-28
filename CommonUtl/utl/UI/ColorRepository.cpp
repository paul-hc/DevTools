
#include "pch.h"
#include "ColorRepository.h"
#include "Color.h"
#include "PopupMenus_fwd.h"
#include "resource.h"
#include "utl/Algorithms.h"
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
		static const CEnumTags s_tags( _T("\
Standard Colors|Office 2003 Colors|Office 2007 Colors|DirectX (X11) Colors|HTML Colors|\
Windows (System) Colors|\
Halftone: 16 Colors|Halftone: 20 Colors|Halftone: 256 Colors|\
Color Shades|User Custom Colors|\
Development Colors"
) );
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
	, m_name( word::ToSpacedWordBreaks( str::FromAnsi( pLiteral ).c_str() ) )
	, m_pParentTable( nullptr )
{
}

const char* CColorEntry::FindScopedLiteral( const char* pScopedColorName )
{
	static const std::string s_scopeOp = "::";
	size_t posLastScope = str::FindLast<str::Case>( pScopedColorName, s_scopeOp.c_str(), s_scopeOp.length() );

	return posLastScope != std::string::npos ? ( pScopedColorName + posLastScope + s_scopeOp.length() ) : pScopedColorName;
}

std::tstring CColorEntry::FormatColor( const TCHAR* pFieldSep /*= s_fieldSep*/, bool suffixTableName /*= true*/ ) const
{
	std::tstring colorName = m_name;

	if ( suffixTableName && m_pParentTable != nullptr && !m_pParentTable->IsHalftoneTable() )
		stream::Tag( colorName, str::Enquote( m_pParentTable->GetTableName().c_str() ), _T(" ") );

	if ( pFieldSep != nullptr )			// requesting full color name?
		stream::Tag( colorName, ui::FormatColor( m_color, pFieldSep ), pFieldSep );

	return colorName;
}


// CColorTable implementation

CColorTable::CColorTable( ui::StdColorTable tableType, size_t capacity, int layoutCount /*= 0*/ )
	: m_tableType( tableType )
	, m_pParentStore( nullptr )
	, m_tableItemId( 0 )
{
	Reset( capacity, layoutCount );
}

CColorTable::~CColorTable()
{
}

void CColorTable::Reset( size_t capacity, int layoutCount )
{
	m_colors.reserve( capacity );
	m_layoutCount = layoutCount;
}

void CColorTable::Clear( void )
{
	m_colors.clear();
	m_layoutCount = 0;
}

void CColorTable::CopyFrom( const CColorTable& srcTable )
{
	ASSERT( &srcTable != this );

	m_colors = srcTable.m_colors;
	m_layoutCount = srcTable.m_layoutCount;
}

void CColorTable::StoreTableInfo( const CColorStore* pParentStore, UINT tableItemId )
{
	ASSERT_NULL( m_pParentStore );
	m_pParentStore = pParentStore;
	m_tableItemId = tableItemId;
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

int CColorTable::GetColumnCount( void ) const
{
	if ( m_layoutCount >= 0 )		// column layout?
		return m_layoutCount;
	else
		return static_cast<int>( m_colors.size() ) / -m_layoutCount;
}

bool CColorTable::GetLayout( OUT size_t* pRowCount, OUT size_t* pColumnCount ) const
{
	if ( m_layoutCount > 0 )		// column layout?
	{
		utl::AssignPtr( pRowCount, m_colors.size() / m_layoutCount );
		utl::AssignPtr( pColumnCount, static_cast<size_t>( m_layoutCount ) );
	}
	else if ( m_layoutCount < 0 )	// row layout?
	{
		utl::AssignPtr( pRowCount, static_cast<size_t>( -m_layoutCount ) );
		utl::AssignPtr( pColumnCount, m_colors.size() / -m_layoutCount );
	}
	else
		return false;

	return true;
}

void CColorTable::QueryMfcColors( ui::TMFCColorArray& rColorArray ) const
{
	rColorArray.SetSize( m_colors.size() );

	for ( size_t i = 0; i != m_colors.size(); ++i )
		rColorArray[i] = m_colors[i].EvalColor();
}

void CColorTable::QueryMfcColors( ui::TMFCColorList& rColorList ) const
{
	for ( std::vector<CColorEntry>::const_iterator itColorEntry = m_colors.begin(); itColorEntry != m_colors.end(); ++itColorEntry )
		rColorList.AddTail( itColorEntry->EvalColor() );
}

void CColorTable::SetupMfcColors( const ui::TMFCColorArray& customColors, int columnCount )
{
	const CColorRepository* pColorRepo = CColorRepository::Instance();		// to borrow color names

	Clear();
	Reset( customColors.GetSize(), columnCount );

	for ( INT_PTR i = 0; i != customColors.GetSize(); ++i )
	{
		m_colors.push_back( CColorEntry() );

		CColorEntry* pColorEntry = &m_colors.back();

		pColorEntry->m_color = customColors[ i ];

		if ( const CColorEntry* pRepoColor = pColorRepo->FindColorEntry( pColorEntry->m_color ) )
			pColorEntry->m_name = pRepoColor->FormatColor( nullptr );		// borrow the qualified color name from the repo
		else
			pColorEntry->m_name.clear();
	}
}

namespace func
{
	template< typename ValueT >
	struct AddSequence
	{
		AddSequence( ValueT startValue, ValueT stepBy ) : m_startValue( startValue ), m_stepBy( stepBy ), m_pos( 0 ) {}

		ValueT operator()( void )
		{
			return m_startValue + m_stepBy * static_cast<ValueT>( m_pos++ );
		}
	public:
		ValueT m_startValue;
		ValueT m_stepBy;
		size_t m_pos;
	};
}

size_t CColorTable::SetupShadesTable( COLORREF selColor, size_t columnCount )
{
	REQUIRE( ui::Shades_Colors == m_tableType );

	Clear();
	Reset( columnCount * 3, static_cast<int>( columnCount ) );

	if ( 0 == columnCount || !ui::IsRealColor( selColor ) )
		return 0;

	selColor = ui::EvalColor( selColor );

	static const Range<ui::TPercent> s_pctRange( 10, 100 );			// [10% to 100%]
	const ui::TPercent pctStep = s_pctRange.GetSpan<ui::TPercent>() / ( (int)columnCount - 1 );
	std::vector<ui::TPercent> percentages;

	std::generate_n( std::back_inserter( percentages ), columnCount, func::AddSequence<ui::TPercent>( s_pctRange.m_start, pctStep ) );

	static const TCHAR* s_pShadeTags[] = { _T("Lighter"), _T("Darker"), _T("Desaturated") };
	enum ShadeTag { Lighter, Darker, Desaturated };

	if ( ui::HasLuminanceVariation( selColor, percentages[ 0 ], percentages[ 1 ] ) )		// Lighter Shades: have some step variation?
		for ( size_t i = 0; i != percentages.size(); ++i )
		{
			COLORREF lighter = selColor;
			ui::AdjustLuminance( lighter, percentages[ i ] );

			CColorEntry colorEntry( lighter, str::Format( _T("%s %d%%"), s_pShadeTags[ Lighter ], percentages[i] ) );
			Add( colorEntry );
		}

	if ( ui::HasLuminanceVariation( selColor, -percentages[ 0 ], -percentages[ 1 ] ) )		// Darker Shades: have some step variation?
		for ( size_t i = 0; i != columnCount; ++i )
		{
			COLORREF darker = selColor;
			ui::AdjustLuminance( darker, -percentages[ i ] );

			CColorEntry colorEntry( darker, str::Format( _T("%s %d%%"), s_pShadeTags[ Darker ], percentages[i] ) );
			Add( colorEntry );
		}

	if ( ui::HasSaturationVariation( selColor, -percentages[ 0 ], -percentages[ 1 ] ) )		// Desaturated Shades: have some step variation?
		for ( size_t i = 0; i != columnCount; ++i )
		{
			COLORREF desaturated = selColor;
			ui::AdjustSaturation( desaturated, -percentages[ i ] );

			CColorEntry colorEntry( desaturated, str::Format( _T("%s %d%%"), s_pShadeTags[ Desaturated ], percentages[i] ) );
			Add( colorEntry );
		}

	return m_colors.size();
}


// CColorStore implementation

void CColorStore::Clear( void )
{
	utl::ClearOwningContainer( m_colorTables );
}

void CColorStore::AddTable( CColorTable* pNewTable, UINT tableItemId )
{
	ASSERT_PTR( pNewTable );

	utl::PushUnique( m_colorTables, pNewTable );
	pNewTable->StoreTableInfo( this, tableItemId );
}

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
			const CColorTable* pSysColors = LookupSystemTable();
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
			if ( const CColorTable* pSysColors = LookupSystemTable() )
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

const CColorTable* CColorStore::LookupSystemTable( void )
{
	return CColorRepository::Instance()->GetSystemColorTable();
}


// CScratchColorStore implementation

CScratchColorStore::CScratchColorStore( void )
	: CColorStore()
	, m_pShadesTable( new CColorTable( ui::Shades_Colors, 0, 0 ) )
	, m_pUserCustomTable( new CColorTable( ui::UserCustom_Colors, 0, 0 ) )
{
	AddTable( m_pShadesTable, ID_SHADES_COLOR_SET );
	AddTable( m_pUserCustomTable, ID_USER_CUSTOM_COLOR_SET );
}


// CHalftoneRepository implementation

CHalftoneRepository::CHalftoneRepository( void )
{
	// add color tables in ui::StdColorTable order:
	AddTable( MakeHalftoneTable( ui::Halftone16_Colors, 16, 8 ), ID_HALFTONE_TABLE_16 );
	AddTable( MakeHalftoneTable( ui::Halftone20_Colors, 20, 0 ), ID_HALFTONE_TABLE_20 );
	AddTable( MakeHalftoneTable( ui::Halftone256_Colors, 256, 16 ), ID_HALFTONE_TABLE_256 );
}

const CHalftoneRepository* CHalftoneRepository::Instance( void )
{
	static const CHalftoneRepository s_halftoneRepo;
	return &s_halftoneRepo;
}

CColorTable* CHalftoneRepository::MakeHalftoneTable( ui::StdColorTable halftoneType, size_t halftoneSize, unsigned int columnCount )
{
	return SetupHalftoneTable( new CColorTable( halftoneType, halftoneSize, columnCount ), halftoneSize );
}

CColorTable* CHalftoneRepository::SetupHalftoneTable( CColorTable* pHalftoneTable, size_t halftoneSize )
{
	ASSERT_PTR( pHalftoneTable );
	const CColorRepository* pColorRepo = CColorRepository::Instance();		// to borrow color names

	std::vector<COLORREF> halftoneColors;
	ui::MakeHalftoneColorTable( halftoneColors, halftoneSize );

	for ( std::vector<COLORREF>::const_iterator itColor = halftoneColors.begin(); itColor != halftoneColors.end(); ++itColor )
	{
		std::tstring colorName;

		if ( const CColorEntry* pRepoColor = pColorRepo->FindColorEntry( *itColor ) )
			colorName = pRepoColor->FormatColor( nullptr );		// borrow the qualified color name from the repo

		pHalftoneTable->Add( CColorEntry( *itColor, colorName ) );
	}

	return pHalftoneTable;
}


// CColorRepository implementation

CColorRepository::CColorRepository( void )
	: CColorStore()
	, m_pWindowsSystemTable( MakeTable_WindowsSystem() )
{
	UINT tableItemId = ID_REPO_COLOR_TABLE_MIN;

	// add color tables in ui::StdColorTable order:
	AddTable( MakeTable_Standard(), tableItemId++ );
	AddTable( MakeTable_Office2003(), tableItemId++ );
	AddTable( MakeTable_Office2007(), tableItemId++ );
	AddTable( MakeTable_DirectX(), tableItemId++ );
	AddTable( MakeTable_HTML(), tableItemId++ );
	//AddTable( MakeTable_Dev(), tableItemId++ );
	AddTable( m_pWindowsSystemTable, tableItemId++ );
}

const CColorRepository* CColorRepository::Instance( void )
{
	static const CColorRepository s_colorRepo;
	return &s_colorRepo;
}


CColorTable* CColorRepository::MakeTable_WindowsSystem( void )
{
	CColorTable* pSysTable = new CColorTable( ui::WindowsSys_Colors, 31, 8 );		// 31 colors: 2 columns x 16 rows

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
	CColorTable* pTable = new CColorTable( ui::Standard_Colors, color::_Standard_ColorCount, 8 );		// 40 colors: 8 columns x 6 rows

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
	pTable->Add( CColorEntry( color::Gray80, "Gray 80%" ) );
	pTable->Add( CColorEntry( color::Gray50, "Gray 50%" ) );
	pTable->Add( CColorEntry( color::Gray40, "Gray 40%" ) );
	pTable->Add( CColorEntry( color::Gray25, "Gray 25%" ) );
	pTable->Add( COLOR_ENTRY( color::White ) );

	return pTable;
}

CColorTable* CColorRepository::MakeTable_Office2003( void )
{
	CColorTable* pTable = new CColorTable( ui::Office2003_Colors, color::Office2003::_Office2003_ColorCount, 8 );		// 40 colors: 8 columns x 5 rows

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

CColorTable* CColorRepository::MakeTable_Office2007( void )
{
	CColorTable* pTable = new CColorTable( ui::Office2007_Colors, color::Office2007::_Office2007_ColorCount, 10 );		// 60 colors: 10 columns x 6 rows

	pTable->Add( CColorEntry( color::Office2007::WhiteBackground1			, _T("White, Background 1") ) );
	pTable->Add( CColorEntry( color::Office2007::BlackText1					, _T("Black, Text 1") ) );
	pTable->Add( CColorEntry( color::Office2007::TanBackground2				, _T("Tan, Background 2") ) );
	pTable->Add( CColorEntry( color::Office2007::DarkBlueText2				, _T("Dark Blue, Text 2") ) );
	pTable->Add( CColorEntry( color::Office2007::BlueAccent1				, _T("Blue, Accent 1") ) );
	pTable->Add( CColorEntry( color::Office2007::RedAccent2					, _T("Red, Accent 2") ) );
	pTable->Add( CColorEntry( color::Office2007::OliveGreenAccent3			, _T("Olive Green, Accent 3") ) );
	pTable->Add( CColorEntry( color::Office2007::PurpleAccent4				, _T("Purple, Accent 4") ) );
	pTable->Add( CColorEntry( color::Office2007::AquaAccent5				, _T("Aqua, Accent 5") ) );
	pTable->Add( CColorEntry( color::Office2007::OrangeAccent6				, _T("Orange, Accent 6") ) );

	pTable->Add( CColorEntry( color::Office2007::WhiteBackground1Darker5	, _T("White, Background 1, Darker 5%") ) );
	pTable->Add( CColorEntry( color::Office2007::BlackText1Lighter50		, _T("Black, Text 1, Lighter 50%") ) );
	pTable->Add( CColorEntry( color::Office2007::TanBackground2Darker10		, _T("Tan, Background 2, Darker 10%") ) );
	pTable->Add( CColorEntry( color::Office2007::DarkBlueText2Lighter80		, _T("Dark Blue, Text 2, Lighter 80%") ) );
	pTable->Add( CColorEntry( color::Office2007::BlueAccent1Lighter80		, _T("Blue, Accent 1, Lighter 80%") ) );
	pTable->Add( CColorEntry( color::Office2007::RedAccent2Lighter80		, _T("Red, Accent 2, Lighter 80%") ) );
	pTable->Add( CColorEntry( color::Office2007::OliveGreenAccent3Lighter80	, _T("Olive Green, Accent 3, Lighter 80%") ) );
	pTable->Add( CColorEntry( color::Office2007::PurpleAccent4Lighter80		, _T("Purple, Accent 4, Lighter 80%") ) );
	pTable->Add( CColorEntry( color::Office2007::AquaAccent5Lighter80		, _T("Aqua, Accent 5, Lighter 80%") ) );
	pTable->Add( CColorEntry( color::Office2007::OrangeAccent6Lighter80		, _T("Orange, Accent 6, Lighter 80%") ) );

	pTable->Add( CColorEntry( color::Office2007::WhiteBackground1Darker15	, _T("White, Background 1, Darker 15%") ) );
	pTable->Add( CColorEntry( color::Office2007::BlackText1Lighter35		, _T("Black, Text 1, Lighter 35%") ) );
	pTable->Add( CColorEntry( color::Office2007::TanBackground2Darker25		, _T("Tan, Background 2, Darker 25%") ) );
	pTable->Add( CColorEntry( color::Office2007::DarkBlueText2Lighter60		, _T("Dark Blue, Text 2, Lighter 60%") ) );
	pTable->Add( CColorEntry( color::Office2007::BlueAccent1Lighter60		, _T("Blue, Accent 1, Lighter 60%") ) );
	pTable->Add( CColorEntry( color::Office2007::RedAccent2Lighter60		, _T("Red, Accent 2, Lighter 60%") ) );
	pTable->Add( CColorEntry( color::Office2007::OliveGreenAccent3Lighter60	, _T("Olive Green, Accent 3, Lighter 60%") ) );
	pTable->Add( CColorEntry( color::Office2007::PurpleAccent4Lighter60		, _T("Purple, Accent 4, Lighter 60%") ) );
	pTable->Add( CColorEntry( color::Office2007::AquaAccent5Lighter60		, _T("Aqua, Accent 5, Lighter 60%") ) );
	pTable->Add( CColorEntry( color::Office2007::OrangeAccent6Lighter60		, _T("Orange, Accent 6, Lighter 60%") ) );

	pTable->Add( CColorEntry( color::Office2007::WhiteBackground1Darker25	, _T("White, Background 1, Darker 25%") ) );
	pTable->Add( CColorEntry( color::Office2007::BlackText1Lighter25		, _T("Black, Text 1, Lighter 25%") ) );
	pTable->Add( CColorEntry( color::Office2007::TanBackground2Darker35		, _T("Tan, Background 2, Darker 35%") ) );
	pTable->Add( CColorEntry( color::Office2007::DarkBlueText2Lighter40		, _T("Dark Blue, Text 2, Lighter 40%") ) );
	pTable->Add( CColorEntry( color::Office2007::BlueAccent1Lighter40		, _T("Blue, Accent 1, Lighter 40%") ) );
	pTable->Add( CColorEntry( color::Office2007::RedAccent2Lighter40		, _T("Red, Accent 2, Lighter 40%") ) );
	pTable->Add( CColorEntry( color::Office2007::OliveGreenAccent3Lighter40	, _T("Olive Green, Accent 3, Lighter 40%") ) );
	pTable->Add( CColorEntry( color::Office2007::PurpleAccent4Lighter40		, _T("Purple, Accent 4, Lighter 40%") ) );
	pTable->Add( CColorEntry( color::Office2007::AquaAccent5Lighter40		, _T("Aqua, Accent 5, Lighter 40%") ) );
	pTable->Add( CColorEntry( color::Office2007::OrangeAccent6Lighter40		, _T("Orange, Accent 6, Lighter 40%") ) );

	pTable->Add( CColorEntry( color::Office2007::WhiteBackground1Darker35	, _T("White, Background 1, Darker 35%") ) );
	pTable->Add( CColorEntry( color::Office2007::BlackText1Lighter15		, _T("Black, Text 1, Lighter 15%") ) );
	pTable->Add( CColorEntry( color::Office2007::TanBackground2Darker50		, _T("Tan, Background 2, Darker 50%") ) );
	pTable->Add( CColorEntry( color::Office2007::DarkBlueText2Darker25		, _T("Dark Blue, Text 2, Darker 25%") ) );
	pTable->Add( CColorEntry( color::Office2007::BlueAccent1Darker25		, _T("Blue, Accent 1, Darker 25%") ) );
	pTable->Add( CColorEntry( color::Office2007::RedAccent2Darker25			, _T("Red, Accent 2, Darker 25%") ) );
	pTable->Add( CColorEntry( color::Office2007::OliveGreenAccent3Darker25	, _T("Olive Green, Accent 3, Darker 25%") ) );
	pTable->Add( CColorEntry( color::Office2007::PurpleAccent4Darker25		, _T("Purple, Accent 4, Darker 25%") ) );
	pTable->Add( CColorEntry( color::Office2007::AquaAccent5Darker25		, _T("Aqua, Accent 5, Darker 25%") ) );
	pTable->Add( CColorEntry( color::Office2007::OrangeAccent6Darker25		, _T("Orange, Accent 6, Darker 25%") ) );

	pTable->Add( CColorEntry( color::Office2007::WhiteBackground1Darker50	, _T("White, Background 1, Darker 50%") ) );
	pTable->Add( CColorEntry( color::Office2007::BlackText1Lighter5			, _T("Black, Text 1, Lighter 5%") ) );
	pTable->Add( CColorEntry( color::Office2007::TanBackground2Darker90		, _T("Tan, Background 2, Darker 90%") ) );
	pTable->Add( CColorEntry( color::Office2007::DarkBlueText2Darker50		, _T("Dark Blue, Text 2, Darker 50%") ) );
	pTable->Add( CColorEntry( color::Office2007::BlueAccent1Darker50		, _T("Blue, Accent 1, Darker 50%") ) );
	pTable->Add( CColorEntry( color::Office2007::RedAccent2Darker50			, _T("Red, Accent 2, Darker 50%") ) );
	pTable->Add( CColorEntry( color::Office2007::OliveGreenAccent3Darker50	, _T("Olive Green, Accent 3, Darker 50%") ) );
	pTable->Add( CColorEntry( color::Office2007::PurpleAccent4Darker50		, _T("Purple, Accent 4, Darker 50%") ) );
	pTable->Add( CColorEntry( color::Office2007::AquaAccent5Darker50		, _T("Aqua, Accent 5, Darker 50%") ) );
	pTable->Add( CColorEntry( color::Office2007::OrangeAccent6Darker50		, _T("Orange, Accent 6, Darker 50%") ) );

	return pTable;
}

CColorTable* CColorRepository::MakeTable_DirectX( void )
{
	CColorTable* pTable = new CColorTable( ui::DirectX_Colors, color::directx::_DirectX_ColorCount, 14 );		// 140 colors: 10 columns x 14 rows

	pTable->Add( COLOR_ENTRY( color::directx::Black ) );
	pTable->Add( COLOR_ENTRY( color::directx::DarkSlateGray ) );
	pTable->Add( COLOR_ENTRY( color::directx::SlateGray ) );
	pTable->Add( COLOR_ENTRY( color::directx::LightSlateGray ) );
	pTable->Add( COLOR_ENTRY( color::directx::DimGray ) );
	pTable->Add( COLOR_ENTRY( color::directx::Gray ) );
	pTable->Add( COLOR_ENTRY( color::directx::DarkGray ) );
	pTable->Add( COLOR_ENTRY( color::directx::Silver ) );
	pTable->Add( COLOR_ENTRY( color::directx::LightGray ) );
	pTable->Add( COLOR_ENTRY( color::directx::Gainsboro ) );
	pTable->Add( COLOR_ENTRY( color::directx::IndianRed ) );
	pTable->Add( COLOR_ENTRY( color::directx::LightCoral ) );
	pTable->Add( COLOR_ENTRY( color::directx::Salmon ) );
	pTable->Add( COLOR_ENTRY( color::directx::DarkSalmon ) );
	pTable->Add( COLOR_ENTRY( color::directx::LightSalmon ) );
	pTable->Add( COLOR_ENTRY( color::directx::Red ) );
	pTable->Add( COLOR_ENTRY( color::directx::Crimson ) );
	pTable->Add( COLOR_ENTRY( color::directx::Firebrick ) );
	pTable->Add( COLOR_ENTRY( color::directx::DarkRed ) );
	pTable->Add( COLOR_ENTRY( color::directx::Pink ) );
	pTable->Add( COLOR_ENTRY( color::directx::LightPink ) );
	pTable->Add( COLOR_ENTRY( color::directx::HotPink ) );
	pTable->Add( COLOR_ENTRY( color::directx::DeepPink ) );
	pTable->Add( COLOR_ENTRY( color::directx::MediumVioletRed ) );
	pTable->Add( COLOR_ENTRY( color::directx::PaleVioletRed ) );
	pTable->Add( COLOR_ENTRY( color::directx::LightSalmon2 ) );
	pTable->Add( COLOR_ENTRY( color::directx::Coral ) );
	pTable->Add( COLOR_ENTRY( color::directx::Tomato ) );
	pTable->Add( COLOR_ENTRY( color::directx::OrangeRed ) );
	pTable->Add( COLOR_ENTRY( color::directx::DarkOrange ) );
	pTable->Add( COLOR_ENTRY( color::directx::Orange ) );
	pTable->Add( COLOR_ENTRY( color::directx::Gold ) );
	pTable->Add( COLOR_ENTRY( color::directx::Yellow ) );
	pTable->Add( COLOR_ENTRY( color::directx::LightYellow ) );
	pTable->Add( COLOR_ENTRY( color::directx::LemonChiffon ) );
	pTable->Add( COLOR_ENTRY( color::directx::LightGoldenrodYellow ) );
	pTable->Add( COLOR_ENTRY( color::directx::PapayaWhip ) );
	pTable->Add( COLOR_ENTRY( color::directx::Moccasin ) );
	pTable->Add( COLOR_ENTRY( color::directx::PeachPuff ) );
	pTable->Add( COLOR_ENTRY( color::directx::PaleGoldenrod ) );
	pTable->Add( COLOR_ENTRY( color::directx::Khaki ) );
	pTable->Add( COLOR_ENTRY( color::directx::DarkKhaki ) );
	pTable->Add( COLOR_ENTRY( color::directx::Lavender ) );
	pTable->Add( COLOR_ENTRY( color::directx::Thistle ) );
	pTable->Add( COLOR_ENTRY( color::directx::Plum ) );
	pTable->Add( COLOR_ENTRY( color::directx::Violet ) );
	pTable->Add( COLOR_ENTRY( color::directx::Orchid ) );
	pTable->Add( COLOR_ENTRY( color::directx::Fuchsia ) );
	pTable->Add( COLOR_ENTRY( color::directx::Magenta ) );
	pTable->Add( COLOR_ENTRY( color::directx::MediumOrchid ) );
	pTable->Add( COLOR_ENTRY( color::directx::MediumPurple ) );
	pTable->Add( COLOR_ENTRY( color::directx::BlueViolet ) );
	pTable->Add( COLOR_ENTRY( color::directx::DarkViolet ) );
	pTable->Add( COLOR_ENTRY( color::directx::DarkOrchid ) );
	pTable->Add( COLOR_ENTRY( color::directx::DarkMagenta ) );
	pTable->Add( COLOR_ENTRY( color::directx::Purple ) );
	pTable->Add( COLOR_ENTRY( color::directx::Indigo ) );
	pTable->Add( COLOR_ENTRY( color::directx::DarkSlateBlue ) );
	pTable->Add( COLOR_ENTRY( color::directx::SlateBlue ) );
	pTable->Add( COLOR_ENTRY( color::directx::MediumSlateBlue ) );
	pTable->Add( COLOR_ENTRY( color::directx::GreenYellow ) );
	pTable->Add( COLOR_ENTRY( color::directx::Chartreuse ) );
	pTable->Add( COLOR_ENTRY( color::directx::LawnGreen ) );
	pTable->Add( COLOR_ENTRY( color::directx::Lime ) );
	pTable->Add( COLOR_ENTRY( color::directx::LimeGreen ) );
	pTable->Add( COLOR_ENTRY( color::directx::PaleGreen ) );
	pTable->Add( COLOR_ENTRY( color::directx::LightGreen ) );
	pTable->Add( COLOR_ENTRY( color::directx::MediumSpringGreen ) );
	pTable->Add( COLOR_ENTRY( color::directx::SpringGreen ) );
	pTable->Add( COLOR_ENTRY( color::directx::MediumSeaGreen ) );
	pTable->Add( COLOR_ENTRY( color::directx::SeaGreen ) );
	pTable->Add( COLOR_ENTRY( color::directx::ForestGreen ) );
	pTable->Add( COLOR_ENTRY( color::directx::Green ) );
	pTable->Add( COLOR_ENTRY( color::directx::DarkGreen ) );
	pTable->Add( COLOR_ENTRY( color::directx::YellowGreen ) );
	pTable->Add( COLOR_ENTRY( color::directx::OliveDrab ) );
	pTable->Add( COLOR_ENTRY( color::directx::Olive ) );
	pTable->Add( COLOR_ENTRY( color::directx::DarkOliveGreen ) );
	pTable->Add( COLOR_ENTRY( color::directx::MediumAquamarine ) );
	pTable->Add( COLOR_ENTRY( color::directx::DarkSeaGreen ) );
	pTable->Add( COLOR_ENTRY( color::directx::LightSeaGreen ) );
	pTable->Add( COLOR_ENTRY( color::directx::DarkCyan ) );
	pTable->Add( COLOR_ENTRY( color::directx::Teal ) );
	pTable->Add( COLOR_ENTRY( color::directx::Aqua ) );
	pTable->Add( COLOR_ENTRY( color::directx::Cyan ) );
	pTable->Add( COLOR_ENTRY( color::directx::LightCyan ) );
	pTable->Add( COLOR_ENTRY( color::directx::PaleTurquoise ) );
	pTable->Add( COLOR_ENTRY( color::directx::Aquamarine ) );
	pTable->Add( COLOR_ENTRY( color::directx::Turquoise ) );
	pTable->Add( COLOR_ENTRY( color::directx::MediumTurquoise ) );
	pTable->Add( COLOR_ENTRY( color::directx::DarkTurquoise ) );
	pTable->Add( COLOR_ENTRY( color::directx::CadetBlue ) );
	pTable->Add( COLOR_ENTRY( color::directx::SteelBlue ) );
	pTable->Add( COLOR_ENTRY( color::directx::LightSteelBlue ) );
	pTable->Add( COLOR_ENTRY( color::directx::PowderBlue ) );
	pTable->Add( COLOR_ENTRY( color::directx::LightBlue ) );
	pTable->Add( COLOR_ENTRY( color::directx::SkyBlue ) );
	pTable->Add( COLOR_ENTRY( color::directx::LightSkyBlue ) );
	pTable->Add( COLOR_ENTRY( color::directx::DeepSkyBlue ) );
	pTable->Add( COLOR_ENTRY( color::directx::DodgerBlue ) );
	pTable->Add( COLOR_ENTRY( color::directx::CornflowerBlue ) );
	pTable->Add( COLOR_ENTRY( color::directx::RoyalBlue ) );
	pTable->Add( COLOR_ENTRY( color::directx::Blue ) );
	pTable->Add( COLOR_ENTRY( color::directx::MediumBlue ) );
	pTable->Add( COLOR_ENTRY( color::directx::DarkBlue ) );
	pTable->Add( COLOR_ENTRY( color::directx::Navy ) );
	pTable->Add( COLOR_ENTRY( color::directx::MidnightBlue ) );
	pTable->Add( COLOR_ENTRY( color::directx::Cornsilk ) );
	pTable->Add( COLOR_ENTRY( color::directx::BlanchedAlmond ) );
	pTable->Add( COLOR_ENTRY( color::directx::Bisque ) );
	pTable->Add( COLOR_ENTRY( color::directx::NavajoWhite ) );
	pTable->Add( COLOR_ENTRY( color::directx::Wheat ) );
	pTable->Add( COLOR_ENTRY( color::directx::BurlyWood ) );
	pTable->Add( COLOR_ENTRY( color::directx::Tan ) );
	pTable->Add( COLOR_ENTRY( color::directx::RosyBrown ) );
	pTable->Add( COLOR_ENTRY( color::directx::SandyBrown ) );
	pTable->Add( COLOR_ENTRY( color::directx::Goldenrod ) );
	pTable->Add( COLOR_ENTRY( color::directx::DarkGoldenrod ) );
	pTable->Add( COLOR_ENTRY( color::directx::Peru ) );
	pTable->Add( COLOR_ENTRY( color::directx::Chocolate ) );
	pTable->Add( COLOR_ENTRY( color::directx::SaddleBrown ) );
	pTable->Add( COLOR_ENTRY( color::directx::Sienna ) );
	pTable->Add( COLOR_ENTRY( color::directx::Brown ) );
	pTable->Add( COLOR_ENTRY( color::directx::Maroon ) );
	pTable->Add( COLOR_ENTRY( color::directx::MistyRose ) );
	pTable->Add( COLOR_ENTRY( color::directx::LavenderBlush ) );
	pTable->Add( COLOR_ENTRY( color::directx::Linen ) );
	pTable->Add( COLOR_ENTRY( color::directx::AntiqueWhite ) );
	pTable->Add( COLOR_ENTRY( color::directx::Ivory ) );
	pTable->Add( COLOR_ENTRY( color::directx::FloralWhite ) );
	pTable->Add( COLOR_ENTRY( color::directx::OldLace ) );
	pTable->Add( COLOR_ENTRY( color::directx::Beige ) );
	pTable->Add( COLOR_ENTRY( color::directx::SeaShell ) );
	pTable->Add( COLOR_ENTRY( color::directx::WhiteSmoke ) );
	pTable->Add( COLOR_ENTRY( color::directx::GhostWhite ) );
	pTable->Add( COLOR_ENTRY( color::directx::AliceBlue ) );
	pTable->Add( COLOR_ENTRY( color::directx::Azure ) );
	pTable->Add( COLOR_ENTRY( color::directx::MintCream ) );
	pTable->Add( COLOR_ENTRY( color::directx::HoneyDew ) );
	pTable->Add( COLOR_ENTRY( color::directx::White ) );

	return pTable;
}

CColorTable* CColorRepository::MakeTable_HTML( void )
{
	CColorTable* pTable = new CColorTable( ui::HTML_Colors, color::html::_Html_ColorCount, 20 );		// 300 colors: 20 columns x 15 rows

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


CColorTable* CColorRepository::MakeTable_Dev( void )
{
	CColorTable* pTable = new CColorTable( ui::Dev_Colors, color::_Custom_ColorCount, 6 );		// 23 colors: 6 columns x 4 rows

	pTable->Add( COLOR_ENTRY( color::VeryDarkGray ) );
	pTable->Add( COLOR_ENTRY( color::DarkGray ) );
	pTable->Add( COLOR_ENTRY( color::LightGray ) );
	pTable->Add( COLOR_ENTRY( color::Gray60 ) );
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

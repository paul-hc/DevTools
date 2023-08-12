#ifndef StdColors_h
#define StdColors_h
#pragma once


namespace color
{
	enum GetType { Evaluate, Raw };


	__declspec( selectany ) extern const COLORREF Null = CLR_NONE;
	__declspec( selectany ) extern const COLORREF Auto = CLR_DEFAULT;
}


class CEnumTags;


namespace ui
{
	enum StdColorTable
	{
		// CColorRepository::Instance():
		Office2003_Colors, Office2007_Colors, DirectX_Colors, HTML_Colors,		// color-repo standard tables
		WindowsSys_Colors,

		// CHalftoneRepository::Instance():
		Halftone16_Colors, Halftone20_Colors, Halftone256_Colors,

		// implementation (not based in repository)
		Shades_Colors, UserCustom_Colors, Recent_Colors,
		Dev_Colors,

			_ColorTableCount,
			NullColorTable
	};

	const CEnumTags& GetTags_ColorTable( void );
}


namespace color
{
	enum Office2003		// aka "Standard"
	{
		Black			= RGB( 0, 0, 0 ),			// 0x000000		// row 1
		Brown			= RGB( 153, 51, 0 ),		// 0x003399
		OliveGreen		= RGB( 51, 51, 0 ),			// 0x003333
		DarkGreen		= RGB( 0, 51, 0 ),			// 0x003300
		DarkTeal		= RGB( 0, 51, 102 ),		// 0x663300
		DarkBlue		= RGB( 0, 0, 128 ),			// 0x800000
		Indigo			= RGB( 51, 51, 153 ),		// 0x993333
		Gray80			= RGB( 51, 51, 51 ),		// 0x333333
		DarkRed			= RGB( 128, 0, 0 ),			// 0x000080		// row 2
		Orange			= RGB( 255, 102, 0 ),		// 0x0066FF
		DarkYellow		= RGB( 128, 128, 0 ),		// 0x008080
		Green			= RGB( 0, 128, 0 ),			// 0x008000
		Teal			= RGB( 0, 128, 128 ),		// 0x808000
		Blue			= RGB( 0, 0, 255 ),			// 0xFF0000
		BlueGray		= RGB( 102, 102, 153 ),		// 0x996666
		Gray50			= RGB( 128, 128, 128 ),		// 0x808080
		Red				= RGB( 255, 0, 0 ),			// 0x0000FF		// row 3
		LightOrange		= RGB( 255, 153, 0 ),		// 0x0099FF
		Lime			= RGB( 153, 204, 0 ),		// 0x00CC99
		SeaGreen		= RGB( 51, 153, 102 ),		// 0x669933
		Aqua			= RGB( 51, 204, 204 ),		// 0xCCCC33
		LightBlue		= RGB( 51, 102, 255 ),		// 0xFF6633
		Violet			= RGB( 128, 0, 128 ),		// 0x800080
		Gray40			= RGB( 150, 150, 150 ),		// 0x969696
		Magenta			= RGB( 255, 0, 255 ),		// 0xFF00FF		// row 4
		Gold			= RGB( 255, 204, 0 ),		// 0x00CCFF
		Yellow			= RGB( 255, 255, 0 ),		// 0x00FFFF
		BrightGreen		= RGB( 0, 255, 0 ),			// 0x00FF00
		Turqoise		= RGB( 0, 255, 255 ),		// 0xFFFF00
		SkyBlue			= RGB( 0, 204, 255 ),		// 0xFFCC00
		Plum			= RGB( 153, 51, 102 ),		// 0x663399
		Gray25			= RGB( 192, 192, 192 ),		// 0xC0C0C0
		Rose			= RGB( 255, 153, 204 ),		// 0xCC99FF		// row 5
		Tan				= RGB( 255, 204, 153 ),		// 0x99CCFF
		LightYellow		= RGB( 255, 255, 153 ),		// 0x99FFFF
		LightGreen		= RGB( 204, 255, 204 ),		// 0xCCFFCC
		LightTurqoise	= RGB( 204, 255, 255 ),		// 0xFFFFCC
		PaleBlue		= RGB( 153, 204, 255 ),		// 0xFFCC99
		Lavender		= RGB( 204, 153, 255 ),		// 0xFF99CC
		White			= RGB( 255, 255, 255 ),		// 0xFFFFFF

		// synonym colors
		_Office2003_ColorCount = 40
	};


	enum DevColor		// not in CColorRepository
	{
		VeryDarkGray	= RGB( 64, 64, 64 ),
		DarkGray		= Gray40,
		LightGray		= Gray25,
		Gray60			= RGB( 96, 96, 96 ),
		Cyan			= Turqoise,
		DarkMagenta		= Violet,
		LightGreenish	= RGB( 192, 220, 192 ),
		SpringGreen		= RGB( 0, 255, 127 ),
		NeonGreen		= RGB( 0, 200, 0 ),
		AzureBlue		= RGB( 0, 125, 255 ),
		BlueWindows10	= RGB( 27, 161, 226 ),
		BlueTextWin10	= RGB( 83, 189, 235 ),
		ScarletRed		= RGB( 200, 0, 0 ),
		Salmon			= RGB( 255, 128, 128 ),
		Pink			= RGB( 255, 128, 255 ),
		PastelPink		= RGB( 255, 200, 200 ),
		LightPastelPink	= RGB( 255, 230, 230 ),
		ToolStripPink	= RGB( 255, 174, 201 ),
		TranspPink		= RGB( 255, 127, 182 ),			// rightmost pink in Paint.NET palette, used for transparent background mask
		SolidOrange		= RGB( 255, 128, 0 ),
		Amber			= RGB( 255, 104, 32 ),
		PaleYellow		= RGB( 255, 255, 220 ),
		GhostWhite		= RGB( 248, 248, 255 ),

			_Custom_ColorCount = 23
	};


	enum StatusColor
	{
		Error = Salmon,
		FocusedBorder = RGB( 181, 207, 231 ),

			_Status_ColorCount = 2
	};


	namespace paint
	{
		enum PaintColor
		{
			Black = 0x000000,
			White = 0xFFFFFF
		};
	}


	namespace Office2007
	{
		enum Color
		{
			WhiteBackground1			= RGB( 255, 255, 255 ),		// row 1 (base colors)
			BlackText1					= RGB( 0, 0, 0 ),
			TanBackground2				= RGB( 238, 236, 225 ),
			DarkBlueText2				= RGB( 31, 73, 125 ),
			BlueAccent1					= RGB( 79, 129, 189 ),
			RedAccent2					= RGB( 192, 80, 77 ),
			OliveGreenAccent3			= RGB( 155, 187, 89 ),
			PurpleAccent4				= RGB( 128, 100, 162 ),
			AquaAccent5					= RGB( 75, 172, 198 ),
			OrangeAccent6				= RGB( 245, 150, 70 ),

			WhiteBackground1Darker5		= RGB( 242, 242, 242 ),		// row 2
			BlackText1Lighter50			= RGB( 127, 127, 127 ),
			TanBackground2Darker10		= RGB( 214, 212, 202 ),
			DarkBlueText2Lighter80		= RGB( 210, 218, 229 ),
			BlueAccent1Lighter80		= RGB( 217, 228, 240 ),
			RedAccent2Lighter80			= RGB( 244, 219, 218 ),
			OliveGreenAccent3Lighter80	= RGB( 234, 241, 221 ),
			PurpleAccent4Lighter80		= RGB( 229, 223, 235 ),
			AquaAccent5Lighter80		= RGB( 216, 237, 242 ),
			OrangeAccent6Lighter80		= RGB( 255, 234, 218 ),

			WhiteBackground1Darker15	= RGB( 215, 215, 215 ),		// row 3
			BlackText1Lighter35			= RGB( 89, 89, 89 ),
			TanBackground2Darker25		= RGB( 177, 176, 167 ),
			DarkBlueText2Lighter60		= RGB( 161, 180, 201 ),
			BlueAccent1Lighter60		= RGB( 179, 202, 226 ),
			RedAccent2Lighter60			= RGB( 233, 184, 182 ),
			OliveGreenAccent3Lighter60	= RGB( 213, 226, 188 ),
			PurpleAccent4Lighter60		= RGB( 203, 191, 215 ),
			AquaAccent5Lighter60		= RGB( 176, 220, 231 ),
			OrangeAccent6Lighter60		= RGB( 255, 212, 181 ),

			WhiteBackground1Darker25	= RGB( 190, 190, 190 ),		// row 4
			BlackText1Lighter25			= RGB( 65, 65, 65 ),
			TanBackground2Darker35		= RGB( 118, 117, 112 ),
			DarkBlueText2Lighter40		= RGB( 115, 143, 175 ),
			BlueAccent1Lighter40		= RGB( 143, 177, 213 ),
			RedAccent2Lighter40			= RGB( 222, 149, 147 ),
			OliveGreenAccent3Lighter40	= RGB( 192, 213, 155 ),
			PurpleAccent4Lighter40		= RGB( 177, 160, 197 ),
			AquaAccent5Lighter40		= RGB( 137, 203, 218 ),
			OrangeAccent6Lighter40		= RGB( 255, 191, 145 ),

			WhiteBackground1Darker35	= RGB( 163, 163, 163 ),		// row 5
			BlackText1Lighter15			= RGB( 42, 42, 42 ),
			TanBackground2Darker50		= RGB( 61, 61, 59 ),
			DarkBlueText2Darker25		= RGB( 20, 57, 92 ),
			BlueAccent1Darker25			= RGB( 54, 96, 139 ),
			RedAccent2Darker25			= RGB( 149, 63, 60 ),
			OliveGreenAccent3Darker25	= RGB( 114, 139, 71 ),
			PurpleAccent4Darker25		= RGB( 97, 76, 119 ),
			AquaAccent5Darker25			= RGB( 41, 128, 146 ),
			OrangeAccent6Darker25		= RGB( 190, 112, 59 ),

			WhiteBackground1Darker50	= RGB( 126, 126, 126 ),		// row 6
			BlackText1Lighter5			= RGB( 20, 20, 20 ),
			TanBackground2Darker90		= RGB( 29, 29, 28 ),
			DarkBlueText2Darker50		= RGB( 17, 40, 64 ),
			BlueAccent1Darker50			= RGB( 38, 66, 94 ),
			RedAccent2Darker50			= RGB( 100, 44, 43 ),
			OliveGreenAccent3Darker50	= RGB( 77, 93, 49 ),
			PurpleAccent4Darker50		= RGB( 67, 53, 81 ),
			AquaAccent5Darker50			= RGB( 31, 86, 99 ),
			OrangeAccent6Darker50		= RGB( 126, 77, 42 ),

			_Office2007_ColorCount = 60
		};
	}


	namespace directx		// aka X11
	{
		enum Enum
		{
			Black				 = 0x000000,		// gray colors
			DarkSlateGray		 = 0x4F4F2F,
			SlateGray			 = 0x908070,
			LightSlateGray		 = 0x998877,
			DimGray				 = 0x696969,
			Gray				 = 0x808080,
			DarkGray			 = 0xA9A9A9,
			Silver				 = 0xC0C0C0,
			LightGray			 = 0xD3D3D3,
			Gainsboro			 = 0xDCDCDC,
			IndianRed			 = 0x5C5CCD,		// red colors
			LightCoral			 = 0x8080F0,
			Salmon				 = 0x7280FA,
			DarkSalmon			 = 0x7A96E9,
			LightSalmon			 = 0x7AA0E0,
			Red					 = 0x0000FF,
			Crimson				 = 0x3C14DC,
			Firebrick			 = 0x2222B2,
			DarkRed				 = 0x00008B,
			Pink				 = 0xCBC0FF,		// pink colors
			LightPink			 = 0xC1B6FF,
			HotPink				 = 0xB469FF,
			DeepPink			 = 0x9314FF,
			MediumVioletRed		 = 0x8515C7,
			PaleVioletRed		 = 0x9370DB,
			LightSalmon2		 = 0x7AA0FF,		// orange colors
			Coral				 = 0x507FFF,
			Tomato				 = 0x4763FF,
			OrangeRed			 = 0x0045FF,
			DarkOrange			 = 0x008CFF,
			Orange				 = 0x00A5FF,
			Gold				 = 0x00D7FF,		// yellow colors
			Yellow				 = 0x00FFFF,
			LightYellow			 = 0xE0FFFF,
			LemonChiffon		 = 0xCDFAFF,
			LightGoldenrodYellow = 0xD2FAFA,
			PapayaWhip			 = 0xD5EFFF,
			Moccasin			 = 0xB5E4FF,
			PeachPuff			 = 0xB9DAFF,
			PaleGoldenrod		 = 0xAAE8EE,
			Khaki				 = 0x8CE6F0,
			DarkKhaki			 = 0x6BB7BD,
			Lavender			 = 0xFAE6E6,		// purple colors
			Thistle				 = 0xD8BFD8,
			Plum				 = 0xDDA0DD,
			Violet				 = 0xEE82EE,
			Orchid				 = 0xD670DA,
			Fuchsia				 = 0xFF00FF,
			Magenta				 = 0xFF00FF,
			MediumOrchid		 = 0xD355BA,
			MediumPurple		 = 0xDB7093,
			BlueViolet			 = 0xE22B8A,
			DarkViolet			 = 0xD30094,
			DarkOrchid			 = 0xCC3299,
			DarkMagenta			 = 0x8B008B,
			Purple				 = 0x800080,
			Indigo				 = 0x82004B,
			DarkSlateBlue		 = 0x8B3D48,
			SlateBlue			 = 0xCD5A6A,
			MediumSlateBlue		 = 0xEE687B,
			GreenYellow			 = 0x2FFFAD,		// green colors
			Chartreuse			 = 0x00FF7F,
			LawnGreen			 = 0x00FC7C,
			Lime				 = 0x00FF00,
			LimeGreen			 = 0x32CD32,
			PaleGreen			 = 0x98FB98,
			LightGreen			 = 0x90EE90,
			MediumSpringGreen	 = 0x9AFA00,
			SpringGreen			 = 0x7FFF00,
			MediumSeaGreen		 = 0x71B33C,
			SeaGreen			 = 0x578B2E,
			ForestGreen			 = 0x228B22,
			Green				 = 0x008000,
			DarkGreen			 = 0x006400,
			YellowGreen			 = 0x32CD9A,
			OliveDrab			 = 0x238E6B,
			Olive				 = 0x008080,
			DarkOliveGreen		 = 0x2F6B55,
			MediumAquamarine	 = 0xAACD66,
			DarkSeaGreen		 = 0x8FBC8F,
			LightSeaGreen		 = 0xAAB220,
			DarkCyan			 = 0x8B8B00,
			Teal				 = 0x808000,
			Aqua				 = 0xFFFF00,		// blue/cyan colors
			Cyan				 = 0xFFFF00,
			LightCyan			 = 0xFFFFE0,
			PaleTurquoise		 = 0xEEEEAF,
			Aquamarine			 = 0xD4FF7F,
			Turquoise			 = 0xD0E040,
			MediumTurquoise		 = 0xCCD148,
			DarkTurquoise		 = 0xD1CE00,
			CadetBlue			 = 0xA09E5F,
			SteelBlue			 = 0xB48246,
			LightSteelBlue		 = 0xDEC4B0,
			PowderBlue			 = 0xE6E0B0,
			LightBlue			 = 0xE6D8AD,
			SkyBlue				 = 0xEBCE87,
			LightSkyBlue		 = 0xFACE87,
			DeepSkyBlue			 = 0xFFBF00,
			DodgerBlue			 = 0xFF901E,
			CornflowerBlue		 = 0xED9564,
			RoyalBlue			 = 0xE16941,
			Blue				 = 0xFF0000,
			MediumBlue			 = 0xCD0000,
			DarkBlue			 = 0x8B0000,
			Navy				 = 0x800000,
			MidnightBlue		 = 0x701919,
			Cornsilk			 = 0xDCF8FF,		// brown colors
			BlanchedAlmond		 = 0xCDEBFF,
			Bisque				 = 0xC4E4FF,
			NavajoWhite			 = 0xADDEFF,
			Wheat				 = 0xB3DEF5,
			BurlyWood			 = 0x87B8DE,
			Tan					 = 0x8CB4D2,
			RosyBrown			 = 0x8F8FBC,
			SandyBrown			 = 0x60A4F4,
			Goldenrod			 = 0x20A5DA,
			DarkGoldenrod		 = 0x0B86B8,
			Peru				 = 0x3F85CD,
			Chocolate			 = 0x1E69D2,
			SaddleBrown			 = 0x13458B,
			Sienna				 = 0x2D52A0,
			Brown				 = 0x2A2AA5,
			Maroon				 = 0x000080,
			MistyRose			 = 0xE1E4FF,		// white colors
			LavenderBlush		 = 0xF5F0FF,
			Linen				 = 0xE6F0FA,
			AntiqueWhite		 = 0xD7EBFA,
			Ivory				 = 0xF0FFFF,
			FloralWhite			 = 0xF0FAFF,
			OldLace				 = 0xE6F5FD,
			Beige				 = 0xDCF5F5,
			SeaShell			 = 0xEEF5FF,
			WhiteSmoke			 = 0xF5F5F5,
			GhostWhite			 = 0xFFF8F8,
			AliceBlue			 = 0xFFF8F0,
			Azure				 = 0xFFFFF0,
			MintCream			 = 0xFAFFF5,
			HoneyDew			 = 0xF0FFF0,
			White				 = 0xFFFFFF,

			// old discarded
			//Snow				 = 0xFAFAFF,	// RGB(255, 250, 250)	#FFFAFA

				_DirectX_ColorCount = 140
		};
	}

	namespace html
	{
		enum HtmlColor
		{
			Black				 = 0x000000,
			Gray0				 = 0x170515,
			Gray18				 = 0x170525,
			Gray21				 = 0x171B2B,
			Gray23				 = 0x172230,
			Gray24				 = 0x262230,
			Gray25				 = 0x262834,
			Gray26				 = 0x2C2834,
			Gray27				 = 0x2C2D38,
			Gray28				 = 0x31313B,
			Gray29				 = 0x35353E,
			Gray30				 = 0x393841,
			Gray31				 = 0x3C3841,
			Gray32				 = 0x3F3E46,
			Gray34				 = 0x44434A,
			Gray35				 = 0x46464C,
			Gray36				 = 0x48484E,
			Gray37				 = 0x4B4A50,
			Gray38				 = 0x4F4E54,
			Gray39				 = 0x515056,
			Gray40				 = 0x545459,
			Gray41				 = 0x58585C,
			Gray42				 = 0x595A5F,
			Gray43				 = 0x5D5D62,
			Gray44				 = 0x606064,
			Gray45				 = 0x626366,
			Gray46				 = 0x656569,
			Gray47				 = 0x68696D,
			Gray48				 = 0x6B6A6E,
			Gray49				 = 0x6D6E72,
			Gray50				 = 0x707174,
			Gray				 = 0x6E6F73,
			SlateGray4			 = 0x7E6D61,
			SlateGray			 = 0x837365,
			LightSteelBlue4		 = 0x7E6D64,
			LightSlateGray		 = 0x8D7B6D,
			CadetBlue4			 = 0x7E784C,
			DarkSlateGray4		 = 0x7E7D4C,
			Thistle4			 = 0x7E6D80,
			MediumSlateBlue		 = 0x805A5E,
			MediumPurple4		 = 0x7E384E,
			MidnightBlue		 = 0x541B15,
			DarkSlateBlue		 = 0x56382B,
			DarkSlateGray		 = 0x3C3825,
			DimGray				 = 0x413E46,
			CornflowerBlue		 = 0x8D1B15,
			RoyalBlue4			 = 0x7E3115,
			SlateBlue4			 = 0x7E2D34,
			RoyalBlue			 = 0xDE602B,
			RoyalBlue1			 = 0xFF6E30,
			RoyalBlue2			 = 0xEC652B,
			RoyalBlue3			 = 0xC75425,
			DeepSkyBlue			 = 0xFFB93B,
			DeepSkyBlue2		 = 0xECAC38,
			SlateBlue			 = 0xC77E35,
			DeepSkyBlue3		 = 0xC79030,
			DeepSkyBlue4		 = 0x7E5825,
			DodgerBlue			 = 0xFF8915,
			DodgerBlue2			 = 0xEC7D15,
			DodgerBlue3			 = 0xC76915,
			DodgerBlue4			 = 0x7E3E15,
			SteelBlue4			 = 0x7E542B,
			SteelBlue			 = 0xA06348,
			SlateBlue2			 = 0xEC6069,
			Violet				 = 0xC9388D,
			MediumPurple3		 = 0xC75D7A,
			MediumPurple		 = 0xD76784,
			MediumPurple2		 = 0xEC7291,
			MediumPurple1		 = 0xFF7B9E,
			LightSteelBlue		 = 0xCE8F72,
			SteelBlue3			 = 0xC78A48,
			SteelBlue2			 = 0xECA556,
			SteelBlue1			 = 0xFFB35C,
			SkyBlue3			 = 0xC79E65,
			SkyBlue4			 = 0x7E6241,
			SlateBlue3			 = 0xA17C73,
			SlateBlue5			 = 0xA17C80,
			SlateGray3			 = 0xC7AF98,
			VioletRed			 = 0x8A35F6,
			VioletRed1			 = 0x8A35F6,
			VioletRed2			 = 0x7F31E4,
			DeepPink			 = 0x8728F5,
			DeepPink2			 = 0x7C28E4,
			DeepPink3			 = 0x6722C1,
			DeepPink4			 = 0x3F057D,
			MediumVioletRed		 = 0x6B22CA,
			VioletRed3			 = 0x6928C1,
			Firebrick			 = 0x170580,
			VioletRed4			 = 0x41057D,
			Maroon4				 = 0x52057D,
			Maroon				 = 0x410581,
			Maroon3				 = 0x8322C1,
			Maroon2				 = 0x9D31E3,
			Maroon1				 = 0xAA35F5,
			Magenta				 = 0xFF00FF,
			Magenta1			 = 0xFF33F4,
			Magenta2			 = 0xEC38E2,
			Magenta3			 = 0xC731C0,
			MediumOrchid		 = 0xB548B0,
			MediumOrchid1		 = 0xFF62D4,
			MediumOrchid2		 = 0xEC5AC4,
			MediumOrchid3		 = 0xC74AA7,
			MediumOrchid4		 = 0x7E286A,
			Purple				 = 0xEF358E,
			Purple1				 = 0xFF3B89,
			Purple2				 = 0xEC387F,
			Purple3				 = 0xC72D6C,
			Purple4				 = 0x7E1B46,
			DarkOrchid4			 = 0x7E1B57,
			DarkOrchid			 = 0x7E1B7D,
			DarkViolet			 = 0xCE2D84,
			DarkOrchid3			 = 0xC7318B,
			DarkOrchid2			 = 0xEC3BA2,
			DarkOrchid1			 = 0xFF41B0,
			Plum4				 = 0x7E587E,
			PaleVioletRed		 = 0x8765D1,
			PaleVioletRed1		 = 0xA178F7,
			PaleVioletRed2		 = 0x946EE5,
			PaleVioletRed3		 = 0x7C5AC2,
			PaleVioletRed4		 = 0x4D357E,
			Plum				 = 0x8F3BB9,
			Plum1				 = 0xFFB7F9,
			Plum2				 = 0xECA9E6,
			Plum3				 = 0xC78EC3,
			Thistle				 = 0xD3B9D2,
			Thistle3			 = 0xC7AEC6,
			LavenderBlush2		 = 0xE2DDEB,
			LavenderBlush3		 = 0xBEBBC8,
			Thistle2			 = 0xECCFE9,
			Thistle1			 = 0xFFDFFC,
			Lavender			 = 0xFAE4E3,
			LavenderBlush		 = 0xF4EEFD,
			LightSteelBlue1		 = 0xFFDEC6,
			LightBlue			 = 0xFFDFAD,
			LightBlue1			 = 0xFFEDBD,
			LightCyan			 = 0xFFFFE0,
			SlateGray1			 = 0xFFDFC2,
			SlateGray2			 = 0xECCFB4,
			LightSteelBlue2		 = 0xECCEB7,
			Turquoise1			 = 0xFFF352,
			Cyan				 = 0xFFFF00,
			Cyan1				 = 0xFFFE57,
			Cyan2				 = 0xECEB50,
			Turquoise2			 = 0xECE24E,
			MediumTurquoise		 = 0xCDCC48,
			Turquoise			 = 0xDBC643,
			DarkSlateGray1		 = 0xFFFE9A,
			DarkSlateGray2		 = 0xECEB8E,
			DarkSlateGray3		 = 0xC7C778,
			Cyan3				 = 0xC7C746,
			Turquoise3			 = 0xC7BF43,
			CadetBlue3			 = 0xC7BF77,
			PaleTurquoise3		 = 0xC7C792,
			LightBlue2			 = 0xECDCAF,
			DarkTurquoise		 = 0x9C9C3B,
			Cyan4				 = 0x7E7D30,
			LightSeaGreen		 = 0x9FA93E,
			LightSkyBlue		 = 0xFACA82,
			LightSkyBlue2		 = 0xECCFA0,
			LightSkyBlue3		 = 0xC7AF87,
			SkyBlue5			 = 0xFFCA82,
			SkyBlue6			 = 0xECBA79,
			LightSkyBlue4		 = 0x7E6D56,
			SkyBlue				 = 0xFF9866,
			LightSlateBlue		 = 0xFF6A73,
			LightCyan2			 = 0xECECCF,
			LightCyan3			 = 0xC7C7AF,
			LightCyan4			 = 0x7D7D71,
			LightBlue3			 = 0xC7B995,
			LightBlue4			 = 0x7E765E,
			PaleTurquoise4		 = 0x7E7D5E,
			DarkSeaGreen4		 = 0x587C61,
			MediumAquamarine	 = 0x818734,
			MediumSeaGreen		 = 0x546730,
			SeaGreen			 = 0x75894E,
			DarkGreen			 = 0x174125,
			SeaGreen4			 = 0x447C38,
			ForestGreen			 = 0x58924E,
			MediumForestGreen	 = 0x357234,
			SpringGreen4		 = 0x2C7C34,
			DarkOliveGreen4		 = 0x267C66,
			Chartreuse4			 = 0x177C43,
			Green4				 = 0x177C34,
			MediumSpringGreen	 = 0x178034,
			SpringGreen			 = 0x2CA03A,
			LimeGreen			 = 0x17A341,
			SpringGreen3		 = 0x2CA04A,
			DarkSeaGreen		 = 0x81B38B,
			DarkSeaGreen3		 = 0x8EC699,
			Green3				 = 0x17C44C,
			Chartreuse3			 = 0x17C46C,
			YellowGreen			 = 0x17D052,
			SpringGreen5		 = 0x52C54C,
			SeaGreen3			 = 0x71C554,
			SpringGreen2		 = 0x64E957,
			SpringGreen1		 = 0x6EFB5E,
			SeaGreen2			 = 0x86E964,
			SeaGreen1			 = 0x92FB6A,
			DarkSeaGreen2		 = 0xAAEAB5,
			DarkSeaGreen1		 = 0xB8FDC3,
			Green				 = 0x00FF00,
			LawnGreen			 = 0x17F787,
			Green1				 = 0x17FB5F,
			Green2				 = 0x17E859,
			Chartreuse2			 = 0x17E87F,
			Chartreuse			 = 0x17FB8A,
			GreenYellow			 = 0x17FBB1,
			DarkOliveGreen1		 = 0x5DFBCC,
			DarkOliveGreen2		 = 0x54E9BC,
			DarkOliveGreen3		 = 0x44C5A0,
			Yellow				 = 0x00FFFF,
			Yellow1				 = 0x17FCFF,
			Khaki1				 = 0x80F3FF,
			Khaki2				 = 0x75E2ED,
			Goldenrod			 = 0x74DAED,
			Gold2				 = 0x17C1EA,
			Gold1				 = 0x17D0FD,
			Goldenrod1			 = 0x17B9FB,
			Goldenrod2			 = 0x17ABE9,
			Gold				 = 0x17A0D4,
			Gold3				 = 0x17A3C7,
			Goldenrod3			 = 0x178EC6,
			DarkGoldenrod		 = 0x1778AF,
			Khaki				 = 0x6EA9AD,
			Khaki3				 = 0x62BEC9,
			Khaki4				 = 0x397882,
			DarkGoldenrod1		 = 0x17B1FB,
			DarkGoldenrod2		 = 0x17A3E8,
			DarkGoldenrod3		 = 0x1789C5,
			Sienna1				 = 0x3174F8,
			Sienna2				 = 0x2C6CE6,
			DarkOrange			 = 0x1780F8,
			DarkOrange1			 = 0x1772F8,
			DarkOrange2			 = 0x1767E5,
			DarkOrange3			 = 0x1756C3,
			Sienna3				 = 0x1758C3,
			Sienna				 = 0x17418A,
			Sienna4				 = 0x17357E,
			IndianRed4			 = 0x17227E,
			DarkOrange4			 = 0x17317E,
			Salmon4				 = 0x17387E,
			DarkGoldenrod4		 = 0x17527F,
			Gold4				 = 0x176580,
			Goldenrod4			 = 0x175880,
			LightSalmon4		 = 0x2C467F,
			Chocolate			 = 0x175AC8,
			Coral3				 = 0x2C4AC3,
			Coral2				 = 0x3C5BE5,
			Coral				 = 0x4165F7,
			DarkSalmon			 = 0x6B8BE1,
			Salmon1				 = 0x5881F8,
			Salmon2				 = 0x5174E6,
			Salmon3				 = 0x4162C3,
			LightSalmon3		 = 0x5174C4,
			LightSalmon2		 = 0x618AE7,
			LightSalmon			 = 0x6B96F9,
			SandyBrown			 = 0x4D9AEE,
			HotPink				 = 0xAB60F6,
			HotPink1			 = 0xAB65F6,
			HotPink2			 = 0x9D5EE4,
			HotPink3			 = 0x8352C2,
			HotPink4			 = 0x52227D,
			LightCoral			 = 0x7174E7,
			IndianRed1			 = 0x595DF7,
			IndianRed2			 = 0x5154E5,
			IndianRed3			 = 0x4146C2,
			Red					 = 0x0000FF,
			Red1				 = 0x1722F6,
			Red2				 = 0x171BE4,
			Firebrick1			 = 0x1728F6,
			Firebrick2			 = 0x1722E4,
			Firebrick3			 = 0x171BC1,
			Pink				 = 0xBEAFFA,
			RosyBrown1			 = 0xB9BBFB,
			RosyBrown2			 = 0xAAADE8,
			Pink2				 = 0xB0A1E7,
			LightPink			 = 0xBAAFFA,
			LightPink1			 = 0xB0A7F9,
			LightPink2			 = 0xA399E7,
			Pink3				 = 0x9387C4,
			RosyBrown3			 = 0x8E90C5,
			RosyBrown			 = 0x8184B3,
			LightPink3			 = 0x8981C4,
			RosyBrown4			 = 0x585A7F,
			LightPink4			 = 0x524E7F,
			Pink4				 = 0x5D527F,
			LavenderBlush4		 = 0x797681,
			LightGoldenrod4		 = 0x397381,
			LemonChiffon4		 = 0x607B82,
			LemonChiffon3		 = 0x99C2C9,
			LightGoldenrod3		 = 0x60B5C8,
			LightGolden2		 = 0x72D6EC,
			LightGoldenrod		 = 0x72D8EC,
			LightGoldenrod1		 = 0x7CE8FF,
			BlanchedAlmond		 = 0xCDEBFF,
			LemonChiffon2		 = 0xB6E5EC,
			LemonChiffon		 = 0xC6F8FF,
			LightGoldenrodYellow = 0xCCF8FA,
			Cornsilk			 = 0xDCF8FF,
			White				 = 0xFFFFFF,

				_Html_ColorCount = 300
		};
	}
}


#endif // StdColors_h

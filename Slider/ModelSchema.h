#ifndef SliderModelSchema_h
#define SliderModelSchema_h
#pragma once


namespace app
{
	enum ModelSchema
	{
		Slider_v3_1 = 0,				// original workspace document format up to this version
		Slider_v3_2 = 0x32,				// persist CWorkspace::m_imageStates
		Slider_v3_5 = 0x35,				// persist the new CWorkspaceData struct
		Slider_v3_6 = 0x36,				// persist CWorkspaceData::m_thumbBoundsSize
		Slider_v3_7 = 0x37,				// persist CFileAttr::GetImageDim() - evaluate real image dimensions
		Slider_v3_8 = 0x38,				// persist CFileAttr::GetImageDim() - evaluate real image dimensions
		Slider_v4_0 = 0x40,				// persist CFileAttr::GetImageDim() - evaluate real image dimensions

			// * always update to the LATEST VERSION *
			Slider_LatestModelSchema = Slider_v4_0
	};


	std::tstring FormatSliderVersion( ModelSchema modelSchemaVersion );
}


#endif // SliderModelSchema_h

#ifndef ModelSchema_h
#define ModelSchema_h
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
		Slider_v3_8 = 0x38,				// WIDE-path encoding; persist CFileAttr::m_pathKey (path + framePos)
		Slider_v4_0 = 0x40,				// new CImageState::m_scalingMode
		Slider_v4_1 = 0x41,				// new CSearchSpec::Type, ...
		Slider_v4_2 = 0x42,				// extract CSearchModel, CSearchModel::m_maxFileCount
		Slider_v5_0 = 0x50,				// CImagesModel, std::vector< CFileAttr* >, custom_order::COpStep::m_newDroppedIndex not shifted by m_dragSelIndexes.size()
		Slider_v5_1 = 0x51,				// add CSlideData::m_imageFramePos
		Slider_v5_2 = 0x52,				// CImageArchiveStg: use '|' instead of '*' as deep sub-path separator
		Slider_v5_3 = 0x53,				// CImageArchiveStg: use "Thumbnails" storage for thumbnails of mixed types; use "_pwd.w" as password stream; drop using the obsolete "_Meta.data" stream
		Slider_v5_4 = 0x54,				// CImageArchiveStg: store album streams under the "Album" sub-storage
		Slider_v5_5 = 0x55,				// major redesign of catalog storage (no schema change); rename CImageArchiveStg to CImageCatalogStg
		Slider_v5_6 = 0x56,				// replace CFileAttr::m_fileType with CFileAttr::m_imageFormat

			// * always update to the LATEST VERSION *
			Slider_LatestModelSchema = Slider_v5_6
	};


	ModelSchema GetLoadingSchema( const CArchive& rArchive );

	std::tstring FormatModelVersion( ModelSchema modelSchemaVersion );
	std::tstring FormatSliderVersion( ModelSchema modelSchemaVersion );
}


#endif // ModelSchema_h

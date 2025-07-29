
Notes:
1) To resolve the ID_TRANSPARENT icon and ID_TRANSPARENT button in IDR_STD_STATUS_STRIP toolbar displaying as BLACK:
	- Add a pixel with RGB( 12, 12, 12 ) and ALPHA=3 to the bottom-right of:
		a) CommonUtl\utl\UI\res\StripStdStatus.png
		b) CommonUtl\utl\UI\res\Transparent.ico
	- This is good-enough workaround to trick Windows that the ICO/PNG is not empty, so it's not displayed as a black rectangle.

2) Format compatibility:
	For 1-bit monochrome bitmap resources, it's best to open and save the monochrome.bmp file in MS Paint as monochrome bitmap.

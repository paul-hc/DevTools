E:\MyProjects\Tools\Slider\Doc\More\Album1.sld

CM_EDIT_WORKSPACE

TODO:
move CAlbumImageView::HandleDropRecipientFiles() to CAlbumDoc


+	srcFullPath	{"D:\WINNT\Background\Wallpaper\New1\0002.jpg"}
+	destFullPath	{"D:\WINNT\Background\Wallpaper\New1\0003.jpg"}



E:\MyProjects\Tools\Slider\Doc\More\Album1.sld


HKEY_CLASSES_ROOT\%s\shell\Open with &Slider\ddeexec
HKEY_CLASSES_ROOT\jpegfile\shell\Open with &Slider\command


    VK_LEFT,        ID_NAVIG_SEEK_LEFT,     VIRTKEY, NOINVERT
    VK_NEXT,        ID_NAVIG_SEEK_NEXT_PAGE, VIRTKEY, NOINVERT
    VK_PRIOR,       ID_NAVIG_SEEK_PREV_PAGE, VIRTKEY, NOINVERT
    VK_RIGHT,       ID_NAVIG_SEEK_RIGHT,    VIRTKEY, NOINVERT

Normal
======
	hWnd=0x0049027E
	Style = 0x15CF8000
	WS_BORDER+WS_DLGFRAME+WS_THICKFRAME+WS_SYSMENU+WS_MAXIMIZE+WS_MAXIMIZEBOX+WS_MINIMIZEBOX

Full Screen
===========
	hWnd=0x0049027E
	Style = 0x15CF8000
	WS_BORDER+WS_DLGFRAME+WS_THICKFRAME+WS_SYSMENU+WS_MAXIMIZEBOX+WS_MINIMIZEBOX


========================================================================
       MICROSOFT FOUNDATION CLASS LIBRARY : Slider
========================================================================


AppWizard has created this Slider application for you.  This application
not only demonstrates the basics of using the Microsoft Foundation classes
but is also a starting point for writing your application.

This file contains a summary of what you will find in each of the files that
make up your Slider application.

Slider.dsp
    This file (the project file) contains information at the project level and
    is used to build a single project or subproject. Other users can share the
    project (.dsp) file, but they should export the makefiles locally.

Slider.h
    This is the main header file for the application.  It includes other
    project specific headers (including Resource.h) and declares the
    CApplication application class.

Slider.cpp
    This is the main application source file that contains the application
    class CApplication.

Slider.rc
    This is a listing of all of the Microsoft Windows resources that the
    program uses.  It includes the icons, bitmaps, and cursors that are stored
    in the RES subdirectory.  This file can be directly edited in Microsoft
	Visual C++.

Slider.clw
    This file contains information used by ClassWizard to edit existing
    classes or add new classes.  ClassWizard also uses this file to store
    information needed to create and edit message maps and dialog data
    maps and to create prototype member functions.

res\Slider.ico
    This is an icon file, which is used as the application's icon.  This
    icon is included by the main resource file Slider.rc.

res\Slider.rc2
    This file contains resources that are not edited by Microsoft 
	Visual C++.  You should place all resources not editable by
	the resource editor in this file.

Slider.reg
    This is an example .REG file that shows you the kind of registration
    settings the framework will set for you.  You can use this as a .REG
    file to go along with your application or just delete it and rely
    on the default RegisterShellFileTypes registration.



/////////////////////////////////////////////////////////////////////////////

For the main frame window:

MainFrame.h, MainFrame.cpp
    These files contain the frame class CMainFrame, which is derived from
    CMDIFrameWnd and controls all MDI frame features.

res\Toolbar.bmp
    This bitmap file is used to create tiled images for the toolbar.
    The initial toolbar and status bar are constructed in the CMainFrame
    class. Edit this toolbar bitmap using the resource editor, and
    update the IDR_MAINFRAME TOOLBAR array in Slider.rc to add
    toolbar buttons.
/////////////////////////////////////////////////////////////////////////////

For the child frame window:

ChildFrm.h, ChildFrm.cpp
    These files define and implement the CChildFrame class, which
    supports the child windows in an MDI application.

/////////////////////////////////////////////////////////////////////////////

AppWizard creates one document type and one view:

AlbumDoc.h, AlbumDoc.cpp - the document
    These files contain your CAlbumDoc class.  Edit these files to
    add your special document data and to implement file saving and loading
    (via CAlbumDoc::Serialize).

AlbumImageView.h, AlbumImageView.cpp - the view of the document
    These files contain your CAlbumImageView class.
    CAlbumImageView objects are used to view CAlbumDoc objects.

res\AlbumDoc.ico
    This is an icon file, which is used as the icon for MDI child windows
    for the CAlbumDoc class.  This icon is included by the main
    resource file Slider.rc.


/////////////////////////////////////////////////////////////////////////////
Other standard files:

StdAfx.h, StdAfx.cpp
    These files are used to build a precompiled header (PCH) file
    named Slider.pch and a precompiled types file named StdAfx.obj.

Resource.h
    This is the standard header file, which defines new resource IDs.
    Microsoft Visual C++ reads and updates this file.

/////////////////////////////////////////////////////////////////////////////
Other notes:

AppWizard uses "TODO:" to indicate parts of the source code you
should add to or customize.

If your application uses MFC in a shared DLL, and your application is 
in a language other than the operating system's current language, you
will need to copy the corresponding localized resources MFC42XXX.DLL
from the Microsoft Visual C++ CD-ROM onto the system or system32 directory,
and rename it to be MFCLOC.DLL.  ("XXX" stands for the language abbreviation.
For example, MFC42DEU.DLL contains resources translated to German.)  If you
don't do this, some of the UI elements of your application will remain in the
language of the operating system.

/////////////////////////////////////////////////////////////////////////////

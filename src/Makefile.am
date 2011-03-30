bin_PROGRAMS = gmenu2x

gmenu2x_SOURCES = asfont.cpp button.cpp cpu.cpp dirdialog.cpp filedialog.cpp \
	filelister.cpp gmenu2x.cpp iconbutton.cpp imagedialog.cpp inputdialog.cpp \
	inputmanager.cpp linkaction.cpp linkapp.cpp link.cpp \
	menu.cpp menusettingbool.cpp menusetting.cpp menusettingdir.cpp \
	menusettingfile.cpp menusettingimage.cpp menusettingint.cpp \
	menusettingmultistring.cpp menusettingrgba.cpp menusettingstring.cpp \
	menusettingstringbase.cpp \
	messagebox.cpp selector.cpp \
	settingsdialog.cpp sfontplus.cpp surfacecollection.cpp surface.cpp \
	textdialog.cpp textmanualdialog.cpp touchscreen.cpp translator.cpp \
	utilities.cpp wallpaperdialog.cpp \
	browsedialog.cpp buttonbox.cpp dialog.cpp \
	imageio.cpp

noinst_HEADERS = asfont.h button.h cpu.h dirdialog.h FastDelegate.h \
	filedialog.h filelister.h gmenu2x.h gp2x.h iconbutton.h imagedialog.h \
	inputdialog.h inputmanager.h linkaction.h linkapp.h link.h \
	menu.h menusettingbool.h menusettingdir.h \
	menusettingfile.h menusetting.h menusettingimage.h menusettingint.h \
	menusettingmultistring.h menusettingrgba.h menusettingstring.h \
	menusettingstringbase.h \
	messagebox.h selector.h settingsdialog.h \
	sfontplus.h surfacecollection.h surface.h textdialog.h textmanualdialog.h \
	touchscreen.h translator.h utilities.h wallpaperdialog.h \
	browsedialog.h buttonbox.h dialog.h \
	imageio.h

AM_CFLAGS= @CFLAGS@ @SDL_CFLAGS@

AM_CXXFLAGS = @CXXFLAGS@ @SDL_CFLAGS@ -DTARGET_GP2X \
	-fno-exceptions \
	-Wall -Wextra -Wundef -Wunused-macros

gmenu2x_LDADD = @LIBS@ @SDL_LIBS@

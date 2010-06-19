bin_PROGRAMS = gmenu2x

gmenu2x_SOURCES = asfont.cpp button.cpp cpu.cpp dirdialog.cpp filedialog.cpp \
	filelister.cpp gmenu2x.cpp iconbutton.cpp imagedialog.cpp inputdialog.cpp \
	inputmanager.cpp linkaction.cpp linkapp.cpp link.cpp listviewitem.cpp \
	menu.cpp menusettingbool.cpp menusetting.cpp menusettingdir.cpp \
	menusettingfile.cpp menusettingimage.cpp menusettingint.cpp \
	menusettingmultistring.cpp menusettingrgba.cpp menusettingstring.cpp \
	messagebox.cpp selector.cpp selectordetector.cpp \
	settingsdialog.cpp sfontplus.cpp surfacecollection.cpp surface.cpp \
	textdialog.cpp textmanualdialog.cpp touchscreen.cpp translator.cpp \
	utilities.cpp wallpaperdialog.cpp listview.cpp \
	browsedialog.cpp buttonbox.cpp dialog.cpp

noinst_HEADERS = asfont.h button.h cpu.h dirdialog.h FastDelegate.h \
	filedialog.h filelister.h gmenu2x.h gp2x.h iconbutton.h imagedialog.h \
	inputdialog.h inputmanager.h jz4740.h linkaction.h linkapp.h link.h \
	listview.h listviewitem.h menu.h menusettingbool.h menusettingdir.h \
	menusettingfile.h menusetting.h menusettingimage.h menusettingint.h \
	menusettingmultistring.h menusettingrgba.h menusettingstring.h \
	messagebox.h selectordetector.h selector.h settingsdialog.h \
	sfontplus.h surfacecollection.h surface.h textdialog.h textmanualdialog.h \
	touchscreen.h translator.h utilities.h wallpaperdialog.h \
	browsedialog.h buttonbox.h dialog.h

AM_CFLAGS= @CFLAGS@ @SDL_CFLAGS@

AM_CXXFLAGS = @CXXFLAGS@ @SDL_CFLAGS@ -Isparsehash-1.6/src -DTARGET_GP2X \
	-Wall -Wextra -Wundef -Wunused-macros

gmenu2x_LDADD = @LIBS@ @SDL_LIBS@ -lfreetype -lSDL_image -lSDL_ttf -lSDL_gfx -lSDL -ljpeg -lpng12 -lz -ldl -lpthread

gmenu2x_LDFLAGS = -lfreetype -lSDL_image -lSDL_ttf -lSDL_gfx -lSDL -ljpeg -lpng12 -lz -ldl -lpthread

gmenu2x_LIBS = @LIBS@ @SDL_LIBS@ -lfreetype -lSDL_image -lSDL_ttf -lSDL_gfx -lSDL -ljpeg -lpng12 -lz -ldl -lpthread

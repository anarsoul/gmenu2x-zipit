/***************************************************************************
 *   Copyright (C) 2006 by Massimiliano Torromeo   *
 *   massimiliano.torromeo@gmail.com   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <iostream>
#include <sstream>
#include <fstream>
#include <algorithm>
#include <stdlib.h>
#include <unistd.h>
#include <math.h>
#include <SDL.h>
#include <SDL_gfxPrimitives.h>
#include <signal.h>

#include <sys/statvfs.h>
#include <errno.h>

#include "gp2x.h"
#include <sys/fcntl.h> //for battery

#ifdef PLATFORM_DINGUX
#	define UNLOCK_VT
#	include <sys/ioctl.h>
#	include <linux/vt.h>
#	include <linux/kd.h>
#endif

//for browsing the filesystem
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>

//for soundcard
#include <sys/ioctl.h>
#include <linux/soundcard.h>

#include "linkapp.h"
#include "linkaction.h"
#include "menu.h"
#include "asfont.h"
#include "surface.h"
#include "filedialog.h"
#include "gmenu2x.h"
#include "filelister.h"

#include "iconbutton.h"
#include "messagebox.h"
#include "inputdialog.h"
#include "settingsdialog.h"
#include "wallpaperdialog.h"
#include "textdialog.h"
#include "menusettingint.h"
#include "menusettingbool.h"
#include "menusettingrgba.h"
#include "menusettingstring.h"
#include "menusettingmultistring.h"
#include "menusettingfile.h"
#include "menusettingimage.h"
#include "menusettingdir.h"

#include "debug.h"

#include <sys/mman.h>

#ifdef PLATFORM_PANDORA
//#include <pnd_container.h>
//#include <pnd_conf.h>
//#include <pnd_discovery.h>
#endif

#ifdef _CARD_ROOT
const char *CARD_ROOT = _CARD_ROOT;
#elif defined(PLATFORM_DINGUX)
const char *CARD_ROOT = "/media/";
#else
const char *CARD_ROOT = "/card/"; //Note: Add a trailing /!
#endif
const int CARD_ROOT_LEN = strlen(CARD_ROOT)-1;

static GMenu2X *app;
static string gmenu2x_home;

using namespace std;
using namespace fastdelegate;

// Note: Keep this in sync with the enum!
static const char *colorNames[NUM_COLORS] = {
	"topBarBg",
	"bottomBarBg",
	"selectionBg",
	"messageBoxBg",
	"messageBoxBorder",
	"messageBoxSelection",
};

static enum color stringToColor(const string &name)
{
	for (unsigned int i = 0; i < NUM_COLORS; i++) {
		if (strcmp(colorNames[i], name.c_str()) == 0) {
			return (enum color)i;
		}
	}
	return (enum color)-1;
}

static const char *colorToString(enum color c)
{
	return colorNames[c];
}

static void quit_all(int err) {
    delete app;
    exit(err);
}

#ifdef UNLOCK_VT

#define FB_TTY "/dev/tty%i"
static void unlockVT()
{
	int i;
	int fd;
	char tty[10];

	for (i=0; i < 10; i++) {
		int mode;

		sprintf(tty, FB_TTY, i);
		fd = open(tty, O_RDWR);
		if (fd < 0)
		  continue;

		if (ioctl(fd, KDGETMODE, &mode) < 0) {
			WARNING("Unable to get mode for tty %i.\n", i);
			close(fd);
			return;
		}

		if (mode != KD_TEXT)
		  break;

		close(fd);
	}

	if (i==10) {
		DEBUG("No graphic tty found.\n");
		return;
	}

	DEBUG("Graphic tty found on %s.\n", tty);

	if (ioctl(fd, KDSETMODE, KD_TEXT) < 0)
		WARNING("unable to set keyboard mode.\n");

	if (ioctl(fd, VT_UNLOCKSWITCH, 1) < 0)
		WARNING("unable to unlock terminal.\n");

	close(fd);
}
#endif

const string GMenu2X::getHome(void)
{
	return gmenu2x_home;
}

int main(int /*argc*/, char * /*argv*/[]) {
	INFO("----\nGMenu2X starting: If you read this message in the logs, check http://gmenu2x.sourceforge.net/page/Troubleshooting for a solution\n----\n");

	signal(SIGINT, &quit_all);
	signal(SIGSEGV,&quit_all);
	signal(SIGTERM,&quit_all);

	char *home = getenv("HOME");
	if (home == NULL) {
		ERROR("Unable to find gmenu2x home directory. The $HOME variable is not defined.\n");
		return 1;
	}

	gmenu2x_home = (string)home + (string)"/.gmenu2x";
	if (!fileExists(gmenu2x_home) && mkdir(gmenu2x_home.c_str(), 0770) < 0) {
		ERROR("Unable to create gmenu2x home directory.\n");
		return 1;
	}

	DEBUG("Home path: %s.\n", gmenu2x_home.c_str());

	app = new GMenu2X();
	DEBUG("Starting main()\n");
	app->main();

	return 0;
}

void GMenu2X::initCPULimits() {
	// Note: These values are for the Dingoo.
	//       The NanoNote does not have cpufreq enabled in its kernel and
	//       other devices are not actively maintained.
	// TODO: Read min and max from sysfs.
	cpuFreqMin = 30;
	cpuFreqMax = 500;
	cpuFreqSafeMax = 420;
	cpuFreqMenuDefault = 200;
	cpuFreqAppDefault = 384;
	cpuFreqMultiple = 24;

	// Round min and max values to the specified multiple.
	cpuFreqMin = ((cpuFreqMin + cpuFreqMultiple - 1) / cpuFreqMultiple)
			* cpuFreqMultiple;
	cpuFreqMax = (cpuFreqMax / cpuFreqMultiple) * cpuFreqMultiple;
	cpuFreqSafeMax = (cpuFreqSafeMax / cpuFreqMultiple) * cpuFreqMultiple;
	cpuFreqMenuDefault = (cpuFreqMenuDefault / cpuFreqMultiple) * cpuFreqMultiple;
	cpuFreqAppDefault = (cpuFreqAppDefault / cpuFreqMultiple) * cpuFreqMultiple;
}

void GMenu2X::init() {
#ifdef PLATFORM_GP2X
/*	gp2x_mem = open("/dev/mem", O_RDWR);
	gp2x_memregs=(unsigned short *)mmap(0, 0x10000, PROT_READ|PROT_WRITE, MAP_SHARED, gp2x_mem, 0xc0000000);
	MEM_REG=&gp2x_memregs[0];
	if (f200) {
		//if wm97xx fails to open, set f200 to false to prevent any further access to the touchscreen
		f200 = ts.init();
	}*/
#else
	batteryHandle = fopen("/sys/class/power_supply/battery/capacity", "r");
	usbHandle = fopen("/sys/class/power_supply/USB/online", "r");
	acHandle = fopen("/sys/class/power_supply/ac/online", "r");
	backlightHandle =
		fopen("/sys/class/backlight/gpm940b0-bl/brightness", "w+");
#endif
}

void GMenu2X::deinit() {
#ifdef PLATFORM_GP2X
/*	if (gp2x_mem!=0) {
		gp2x_memregs[0x28DA>>1]=0x4AB;
		gp2x_memregs[0x290C>>1]=640;
		close(gp2x_mem);
	}

	if (f200) ts.deinit();*/
#else
	if (batteryHandle) fclose(batteryHandle);
	if (backlightHandle) fclose(backlightHandle);
	if (usbHandle) fclose(usbHandle);
	if (acHandle) fclose(acHandle);
#endif
}

void GMenu2X::tvout_on(bool /*pal*/) {
#ifdef PLATFORM_GP2X
/*	if (gp2x_mem!=0) {
//		Ioctl_Dummy_t *msg;
//		int TVHandle = ioctl(SDL_videofd, FBMMSP2CTRL, msg);
		if (cx25874!=0) gp2x_tvout_off();
		//if tv-out is enabled without cx25874 open, stop
		//if (gp2x_memregs[0x2800>>1]&0x100) return;
		cx25874 = open("/dev/cx25874",O_RDWR);
		ioctl(cx25874, _IOW('v', 0x02, unsigned char), pal ? 4 : 3);
		gp2x_memregs[0x2906>>1]=512;
		gp2x_memregs[0x28E4>>1]=gp2x_memregs[0x290C>>1];
		gp2x_memregs[0x28E8>>1]=239;
	}*/
#endif
}

void GMenu2X::tvout_off() {
#ifdef PLATFORM_GP2X
/*	if (gp2x_mem!=0) {
		close(cx25874);
		cx25874 = 0;
		gp2x_memregs[0x2906>>1]=1024;
	}*/
#endif
}


GMenu2X::GMenu2X()
{
#ifdef PLATFORM_GP2X
	//Detect firmware version and type
	if (fileExists("/etc/open2x")) {
		fwType = "open2x";
		fwVersion = "";
	} else {
		fwType = "gph";
		fwVersion = "";
	}

	//open2x
	savedVolumeMode = 0;
	volumeMode = VOLUME_MODE_NORMAL;
	volumeScalerNormal = VOLUME_SCALER_NORMAL;
	volumeScalerPhones = VOLUME_SCALER_PHONES;

	o2x_usb_net_on_boot = false;
	o2x_usb_net_ip = "";
	o2x_ftp_on_boot = false;
	o2x_telnet_on_boot = false;
	o2x_gp2xjoy_on_boot = false;
	o2x_usb_host_on_boot = false;
	o2x_usb_hid_on_boot = false;
	o2x_usb_storage_on_boot = false;
#endif

	usbnet = samba = inet = web = false;
	useSelectionPng = false;

	initCPULimits();
	//load config data
	readConfig();

#ifdef PLATFORM_GP2X
	if (fwType=="open2x") {
		readConfigOpen2x();
		//	VOLUME MODIFIER
		switch(volumeMode) {
			case VOLUME_MODE_MUTE:   setVolumeScaler(VOLUME_SCALER_MUTE); break;
			case VOLUME_MODE_PHONES: setVolumeScaler(volumeScalerPhones);	break;
			case VOLUME_MODE_NORMAL: setVolumeScaler(volumeScalerNormal); break;
		}
	} else
		readCommonIni();
#endif

	halfX = resX/2;
	halfY = resY/2;
	bottomBarIconY = resY-18;
	bottomBarTextY = resY-10;

	path = "";
	getExePath();

	batteryHandle = 0;
	backlightHandle = 0;
	usbHandle = 0;
	acHandle = 0;

	init();

#ifdef PLATFORM_GP2X
	cx25874 = 0;

	//Fix tv-out
/*	if (gp2x_mem!=0) {
		if (gp2x_memregs[0x2800>>1]&0x100) {
			gp2x_memregs[0x2906>>1]=512;
			//gp2x_memregs[0x290C>>1]=640;
			gp2x_memregs[0x28E4>>1]=gp2x_memregs[0x290C>>1];
		}
		gp2x_memregs[0x28E8>>1]=239;
	}*/
#endif

#ifdef UNLOCK_VT
	unlockVT();
#endif

	/* Do not clear the screen on exit.
	 * This may require an SDL patch available at
	 * https://github.com/mthuurne/opendingux-buildroot/blob
	 * 			/opendingux-2010.11/package/sdl/sdl-fbcon-clear-onexit.patch
	 */
	setenv("SDL_FBCON_DONT_CLEAR", "1", 0);

	//Screen
	if( SDL_Init(SDL_INIT_VIDEO|SDL_INIT_JOYSTICK)<0 ) {
		ERROR("Could not initialize SDL: %s\n", SDL_GetError());
		quit();
	}

	s = Surface::openOutputSurface(resX, resY, confInt["videoBpp"]);

	bg = NULL;
	font = NULL;
	menu = NULL;
	setSkin(confStr["skin"], false);
	initMenu();

	if (!fileExists(confStr["wallpaper"])) {
		DEBUG("No wallpaper defined; we will take the default one.\n");
		confStr["wallpaper"] = DEFAULT_WALLPAPER_PATH;
	}

	initBG();

	/* If a user-specified input.conf file exists, we load it;
	 * otherwise, we load the default one. */
	const char *input_file = (getHome() + "/input.conf").c_str();
	if (fileExists(input_file)) {
		DEBUG("Loading user-specific input.conf file: %s.\n", input_file);
	} else {
		input_file = GMENU2X_SYSTEM_DIR "/input.conf";
		DEBUG("Loading system input.conf file: %s.\n", input_file);
	}

	input.init(input_file);

	if (confInt["backlightTimeout"] > 0)
        PowerSaver::getInstance()->setScreenTimeout( confInt["backlightTimeout"] );

	setInputSpeed();
	initServices();
	setBacklight(confInt["backlight"]);
	setVolume(confInt["globalVolume"]);
	applyDefaultTimings();
	setClock(confInt["menuClock"]);
	//recover last session
	readTmp();
	if (lastSelectorElement>-1 && menu->selLinkApp()!=NULL && (!menu->selLinkApp()->getSelectorDir().empty() || !lastSelectorDir.empty()))
		menu->selLinkApp()->selector(lastSelectorElement,lastSelectorDir);

}

GMenu2X::~GMenu2X() {
	if (PowerSaver::isRunning())
		delete PowerSaver::getInstance();
	quit();

	delete menu;
	delete font;
}

void GMenu2X::quit() {
	fflush(NULL);
	sc.clear();
	delete s;

	SDL_Quit();

	unsetenv("SDL_FBCON_DONT_CLEAR");

#ifdef PLATFORM_GP2X
/*	if (gp2x_mem!=0) {
		//Fix tv-out
		if (gp2x_memregs[0x2800>>1]&0x100) {
			gp2x_memregs[0x2906>>1]=512;
			gp2x_memregs[0x28E4>>1]=gp2x_memregs[0x290C>>1];
		}
		gp2x_deinit();
	}*/
#endif
}

void GMenu2X::initBG() {
	sc.del("bgmain");

	// Load wallpaper.
	delete bg;
	bg = Surface::loadImage(confStr["wallpaper"]);
	if (!bg) {
		bg = Surface::emptySurface(resX, resY);
	}

	drawTopBar(bg);
	drawBottomBar(bg);

	Surface *bgmain = new Surface(bg);
	sc.add(bgmain,"bgmain");

	Surface *sd = Surface::loadImage("imgs/sd.png", confStr["skin"]);
	if (sd) sd->blit(bgmain, 3, bottomBarIconY);

#ifdef PLATFORM_DINGUX
	string df = getDiskFree("/boot");
#else
	string df = getDiskFree(CARD_ROOT);
#endif
	bgmain->write(font, df, 22, bottomBarTextY, ASFont::HAlignLeft, ASFont::VAlignMiddle);
	delete sd;

	Surface *volume = Surface::loadImage("imgs/volume.png", confStr["skin"]);
	volumeX = 27+font->getTextWidth(df);
	if (volume) volume->blit(bgmain, volumeX, bottomBarIconY);
	volumeX += 19;
	delete volume;

	Surface *cpu = Surface::loadImage("imgs/cpu.png", confStr["skin"]);
	cpuX = volumeX+font->getTextWidth("100")+5;
	if (cpu) cpu->blit(bgmain, cpuX, bottomBarIconY);
	cpuX += 19;
	manualX = cpuX+font->getTextWidth("300Mhz")+5;
	delete cpu;

	int serviceX = resX-38;
	if (usbnet) {
		if (web) {
			Surface *webserver = Surface::loadImage(
				"imgs/webserver.png", confStr["skin"]);
			if (webserver) webserver->blit(bgmain, serviceX, bottomBarIconY);
			serviceX -= 19;
			delete webserver;
		}
		if (samba) {
			Surface *sambaS = Surface::loadImage(
				"imgs/samba.png", confStr["skin"]);
			if (sambaS) sambaS->blit(bgmain, serviceX, bottomBarIconY);
			serviceX -= 19;
			delete sambaS;
		}
		if (inet) {
			Surface *inetS = Surface::loadImage("imgs/inet.png", confStr["skin"]);
			if (inetS) inetS->blit(bgmain, serviceX, bottomBarIconY);
			serviceX -= 19;
			delete inetS;
		}
	}

	bgmain->convertToDisplayFormat();
}

void GMenu2X::initFont() {
	if (font != NULL) {
		delete font;
		font = NULL;
	}

	string fontFile = sc.getSkinFilePath("imgs/font.png");
	if (fontFile.empty()) {
		ERROR("Font png not found!\n");
		quit();
		exit(-1);
	}
	font = new ASFont(fontFile);
}

void GMenu2X::initMenu() {
	//Menu structure handler
	menu = new Menu(this);
	for (uint i=0; i<menu->getSections().size(); i++) {
		//Add virtual links in the applications section
		if (menu->getSections()[i]=="applications") {
			menu->addActionLink(i,"Explorer",MakeDelegate(this,&GMenu2X::explorer),tr["Launch an application"],"skin:icons/explorer.png");
		}

		//Add virtual links in the setting section
		else if (menu->getSections()[i]=="settings") {
			menu->addActionLink(i,"GMenu2X",MakeDelegate(this,&GMenu2X::options),tr["Configure GMenu2X's options"],"skin:icons/configure.png");
			menu->addActionLink(i,tr["Skin"],MakeDelegate(this,&GMenu2X::skinMenu),tr["Configure skin"],"skin:icons/skin.png");
			menu->addActionLink(i,tr["Wallpaper"],MakeDelegate(this,&GMenu2X::changeWallpaper),tr["Change GMenu2X wallpaper"],"skin:icons/wallpaper.png");
#ifdef PLATFORM_GP2X
			if (fwType=="open2x")
				menu->addActionLink(i,"Open2x",MakeDelegate(this,&GMenu2X::settingsOpen2x),tr["Configure Open2x system settings"],"skin:icons/o2xconfigure.png");
/*			menu->addActionLink(i,"TV",MakeDelegate(this,&GMenu2X::toggleTvOut),tr["Activate/deactivate tv-out"],"skin:icons/tv.png");
			menu->addActionLink(i,"USB Sd",MakeDelegate(this,&GMenu2X::activateSdUsb),tr["Activate Usb on SD"],"skin:icons/usb.png");
			if (fwType=="gph" && !f200)
				menu->addActionLink(i,"USB Nand",MakeDelegate(this,&GMenu2X::activateNandUsb),tr["Activate Usb on Nand"],"skin:icons/usb.png");
			//menu->addActionLink(i,"USB Root",MakeDelegate(this,&GMenu2X::activateRootUsb),tr["Activate Usb on the root of the Gp2x Filesystem"],"skin:icons/usb.png");*/
#endif
			if (fileExists(getHome()+"/log.txt"))
				menu->addActionLink(i,tr["Log Viewer"],MakeDelegate(this,&GMenu2X::viewLog),tr["Displays last launched program's output"],"skin:icons/ebook.png");
			menu->addActionLink(i,tr["About"],MakeDelegate(this,&GMenu2X::about),tr["Info about GMenu2X"],"skin:icons/about.png");
		}
	}

	menu->setSectionIndex(confInt["section"]);
	menu->setLinkIndex(confInt["link"]);

	menu->loadIcons();

	//DEBUG
	//menu->addLink( CARD_ROOT, "sample.pxml", "applications" );
}

void GMenu2X::about() {
	vector<string> text;
	split(text,"GMenu2X is developed by Massimiliano \"Ryo\" Torromeo, and is released under the GPL-v2 license.\n\
Website: http://gmenu2x.sourceforge.net\n\
E-Mail & PayPal account: massimiliano.torromeo@gmail.com\n\
\n\
Thanks goes to...\n\
\n\
 Contributors\n\
----\n\
NoidZ for his gp2x' buttons graphics\n\
\n\
 Beta testers\n\
----\n\
Goemon4, PokeParadox, PSyMastR and Tripmonkey_uk\n\
\n\
 Translators\n\
----\n\
English & Italian by me\n\
French by Yodaz\n\
Danish by claus\n\
Dutch by superfly\n\
Spanish by pedator\n\
Portuguese (Portugal) by NightShadow\n\
Slovak by Jozef\n\
Swedish by Esslan and Micket\n\
German by fusion_power, johnnysnet and Waldteufel\n\
Finnish by Jontte and Atte\n\
Norwegian by cowai\n\
Russian by XaMMaX90\n\
\n\
 Donors\n\
----\n\
EvilDragon (www.gp2x.de)\n\
Tecnologie Creative (www.tecnologiecreative.it)\n\
TelcoLou\n\
gaterooze\n\
deepmenace\n\
superfly\n\
halo9\n\
sbock\n\
b._.o._.b\n\
Jacopastorius\n\
lorystorm90\n\
and all the anonymous donors...\n\
(If I missed to list you or if you want to be removed, contact me.)","\n");
	TextDialog td(this, "GMenu2X", tr.translate("Version $1 (Build date: $2)","0.10-test4",__DATE__,NULL), "icons/about.png", &text);
	td.exec();
}

void GMenu2X::viewLog() {
	string logfile = getHome()+"/log.txt";
	if (fileExists(logfile)) {
		ifstream inf(logfile.c_str(), ios_base::in);
		if (inf.is_open()) {
			vector<string> log;

			string line;
			while (getline(inf, line, '\n'))
				log.push_back(line);
			inf.close();

			TextDialog td(this, tr["Log Viewer"], tr["Displays last launched program's output"], "icons/ebook.png", &log);
			td.exec();

			MessageBox mb(this, tr["Do you want to delete the log file?"], "icons/ebook.png");
			mb.setButton(ACCEPT, tr["Yes"]);
			mb.setButton(CANCEL, tr["No"]);
			if (mb.exec() == ACCEPT) {
				ledOn();
				unlink(logfile.c_str());
				sync();
				menu->deleteSelectedLink();
				ledOff();
			}
		}
	}
}

void GMenu2X::readConfig() {
	string conffile = GMENU2X_SYSTEM_DIR "/gmenu2x.conf";
	readConfig(conffile);

	conffile = getHome() + "/gmenu2x.conf";
	readConfig(conffile);
}

void GMenu2X::readConfig(string conffile) {
	if (fileExists(conffile)) {
		ifstream inf(conffile.c_str(), ios_base::in);
		if (inf.is_open()) {
			string line;
			while (getline(inf, line, '\n')) {
				string::size_type pos = line.find("=");
				string name = trim(line.substr(0,pos));
				string value = trim(line.substr(pos+1,line.length()));

				if (value.length()>1 && value.at(0)=='"' && value.at(value.length()-1)=='"')
					confStr[name] = value.substr(1,value.length()-2);
				else
					confInt[name] = atoi(value.c_str());
			}
			inf.close();
		}
	}
	if (!confStr["lang"].empty())
		tr.setLang(confStr["lang"]);

	if (!confStr["wallpaper"].empty() && !fileExists(confStr["wallpaper"]))
		confStr["wallpaper"] = "";

	if (confStr["skin"].empty() || SurfaceCollection::getSkinPath(confStr["skin"]).empty())
		confStr["skin"] = "Default";

	evalIntConf( &confInt["outputLogs"], 0, 0,1 );
	evalIntConf( &confInt["maxClock"],
				 cpuFreqSafeMax, cpuFreqMin, cpuFreqMax );
	evalIntConf( &confInt["menuClock"],
				 cpuFreqMenuDefault, cpuFreqMin, cpuFreqSafeMax );
	evalIntConf( &confInt["globalVolume"], 67, 0,100 );
	evalIntConf( &confInt["backlightTimeout"], 15, 0,120 );
	evalIntConf( &confInt["backlight"], 100, 5,100 );
	evalIntConf( &confInt["videoBpp"],
#ifdef PLATFORM_DINGUX
				 16,
#else
				 32,
#endif
				 16, 32 );

	if (confStr["tvoutEncoding"] != "PAL") confStr["tvoutEncoding"] = "NTSC";
	resX = constrain( confInt["resolutionX"], 320,1920 );
	resY = constrain( confInt["resolutionY"], 240,1200 );
}

void GMenu2X::writeConfig() {
	ledOn();
	string conffile = getHome() + "/gmenu2x.conf";
	ofstream inf(conffile.c_str());
	if (inf.is_open()) {
		ConfStrHash::iterator endS = confStr.end();
		for(ConfStrHash::iterator curr = confStr.begin(); curr != endS; curr++)
			inf << curr->first << "=\"" << curr->second << "\"" << endl;

		ConfIntHash::iterator endI = confInt.end();
		for(ConfIntHash::iterator curr = confInt.begin(); curr != endI; curr++)
			inf << curr->first << "=" << curr->second << endl;

		inf.close();
		sync();
	}
	ledOff();
}

#ifdef PLATFORM_GP2X
void GMenu2X::readConfigOpen2x() {
	string conffile = "/etc/config/open2x.conf";
	if (fileExists(conffile)) {
		ifstream inf(conffile.c_str(), ios_base::in);
		if (inf.is_open()) {
			string line;
			while (getline(inf, line, '\n')) {
				string::size_type pos = line.find("=");
				string name = trim(line.substr(0,pos));
				string value = trim(line.substr(pos+1,line.length()));

				if (name=="USB_NET_ON_BOOT") o2x_usb_net_on_boot = value == "y" ? true : false;
				else if (name=="USB_NET_IP") o2x_usb_net_ip = value;
				else if (name=="TELNET_ON_BOOT") o2x_telnet_on_boot = value == "y" ? true : false;
				else if (name=="FTP_ON_BOOT") o2x_ftp_on_boot = value == "y" ? true : false;
				else if (name=="GP2XJOY_ON_BOOT") o2x_gp2xjoy_on_boot = value == "y" ? true : false;
				else if (name=="USB_HOST_ON_BOOT") o2x_usb_host_on_boot = value == "y" ? true : false;
				else if (name=="USB_HID_ON_BOOT") o2x_usb_hid_on_boot = value == "y" ? true : false;
				else if (name=="USB_STORAGE_ON_BOOT") o2x_usb_storage_on_boot = value == "y" ? true : false;
				else if (name=="VOLUME_MODE") volumeMode = savedVolumeMode = constrain( atoi(value.c_str()), 0, 2);
				else if (name=="PHONES_VALUE") volumeScalerPhones = constrain( atoi(value.c_str()), 0, 100);
				else if (name=="NORMAL_VALUE") volumeScalerNormal = constrain( atoi(value.c_str()), 0, 150);
			}
			inf.close();
		}
	}
}
void GMenu2X::writeConfigOpen2x() {
	ledOn();
	string conffile = "/etc/config/open2x.conf";
	ofstream inf(conffile.c_str());
	if (inf.is_open()) {
		inf << "USB_NET_ON_BOOT=" << ( o2x_usb_net_on_boot ? "y" : "n" ) << endl;
		inf << "USB_NET_IP=" << o2x_usb_net_ip << endl;
		inf << "TELNET_ON_BOOT=" << ( o2x_telnet_on_boot ? "y" : "n" ) << endl;
		inf << "FTP_ON_BOOT=" << ( o2x_ftp_on_boot ? "y" : "n" ) << endl;
		inf << "GP2XJOY_ON_BOOT=" << ( o2x_gp2xjoy_on_boot ? "y" : "n" ) << endl;
		inf << "USB_HOST_ON_BOOT=" << ( (o2x_usb_host_on_boot || o2x_usb_hid_on_boot || o2x_usb_storage_on_boot) ? "y" : "n" ) << endl;
		inf << "USB_HID_ON_BOOT=" << ( o2x_usb_hid_on_boot ? "y" : "n" ) << endl;
		inf << "USB_STORAGE_ON_BOOT=" << ( o2x_usb_storage_on_boot ? "y" : "n" ) << endl;
		inf << "VOLUME_MODE=" << volumeMode << endl;
		if (volumeScalerPhones != VOLUME_SCALER_PHONES) inf << "PHONES_VALUE=" << volumeScalerPhones << endl;
		if (volumeScalerNormal != VOLUME_SCALER_NORMAL) inf << "NORMAL_VALUE=" << volumeScalerNormal << endl;
		inf.close();
		sync();
	}
	ledOff();
}
#endif

void GMenu2X::writeSkinConfig() {
	ledOn();

	string conffile = getHome() + "/skins/";
	if (!fileExists(conffile))
	  mkdir(conffile.c_str(), 0770);
	conffile = conffile + confStr["skin"];
	if (!fileExists(conffile))
	  mkdir(conffile.c_str(), 0770);
	conffile = conffile + "/skin.conf";

	ofstream inf(conffile.c_str());
	if (inf.is_open()) {
		ConfStrHash::iterator endS = skinConfStr.end();
		for(ConfStrHash::iterator curr = skinConfStr.begin(); curr != endS; curr++)
			inf << curr->first << "=\"" << curr->second << "\"" << endl;

		ConfIntHash::iterator endI = skinConfInt.end();
		for(ConfIntHash::iterator curr = skinConfInt.begin(); curr != endI; curr++)
			inf << curr->first << "=" << curr->second << endl;

		int i;
		for (i = 0; i < NUM_COLORS; ++i) {
			inf << colorToString((enum color)i) << "=#";
			inf.width(2); inf.fill('0');
			inf << right << hex << skinConfColors[i].r;
			inf.width(2); inf.fill('0');
			inf << right << hex << skinConfColors[i].g;
			inf.width(2); inf.fill('0');
			inf << right << hex << skinConfColors[i].b;
			inf.width(2); inf.fill('0');
			inf << right << hex << skinConfColors[i].a << endl;
		}

		inf.close();
		sync();
	}
	ledOff();
}

#ifdef PLATFORM_GP2X
void GMenu2X::readCommonIni() {
	if (fileExists("/usr/gp2x/common.ini")) {
		ifstream inf("/usr/gp2x/common.ini", ios_base::in);
		if (inf.is_open()) {
			string line;
			string section = "";
			while (getline(inf, line, '\n')) {
				line = trim(line);
				if (line[0]=='[' && line[line.length()-1]==']') {
					section = line.substr(1,line.length()-2);
				} else {
					string::size_type pos = line.find("=");
					string name = trim(line.substr(0,pos));
					string value = trim(line.substr(pos+1,line.length()));

					if (section=="usbnet") {
						if (name=="enable")
							usbnet = value=="true" ? true : false;
						else if (name=="ip")
							ip = value;

					} else if (section=="server") {
						if (name=="inet")
							inet = value=="true" ? true : false;
						else if (name=="samba")
							samba = value=="true" ? true : false;
						else if (name=="web")
							web = value=="true" ? true : false;
					}
				}
			}
			inf.close();
		}
	}
}

void GMenu2X::writeCommonIni() {}
#endif

void GMenu2X::readTmp() {
	lastSelectorElement = -1;
	if (fileExists("/tmp/gmenu2x.tmp")) {
		ifstream inf("/tmp/gmenu2x.tmp", ios_base::in);
		if (inf.is_open()) {
			string line;
			string section = "";
			while (getline(inf, line, '\n')) {
				string::size_type pos = line.find("=");
				string name = trim(line.substr(0,pos));
				string value = trim(line.substr(pos+1,line.length()));

				if (name=="section")
					menu->setSectionIndex(atoi(value.c_str()));
				else if (name=="link")
					menu->setLinkIndex(atoi(value.c_str()));
				else if (name=="selectorelem")
					lastSelectorElement = atoi(value.c_str());
				else if (name=="selectordir")
					lastSelectorDir = value;
			}
			inf.close();
		}
	}
}

void GMenu2X::writeTmp(int selelem, const string &selectordir) {
	string conffile = "/tmp/gmenu2x.tmp";
	ofstream inf(conffile.c_str());
	if (inf.is_open()) {
		inf << "section=" << menu->selSectionIndex() << endl;
		inf << "link=" << menu->selLinkIndex() << endl;
		if (selelem>-1)
			inf << "selectorelem=" << selelem << endl;
		if (selectordir!="")
			inf << "selectordir=" << selectordir << endl;
		inf.close();
		sync();
	}
}

void GMenu2X::initServices() {
#ifdef PLATFORM_GP2X
/*	if (usbnet) {
		string services = "scripts/services.sh "+ip+" "+(inet?"on":"off")+" "+(samba?"on":"off")+" "+(web?"on":"off")+" &";
		system(services.c_str());
	}*/
#endif
}

void GMenu2X::ledOn() {
#ifdef PLATFORM_GP2X
//	if (gp2x_mem!=0 && !f200) gp2x_memregs[0x106E >> 1] ^= 16;
	//SDL_SYS_JoystickGp2xSys(joy.joystick, BATT_LED_ON);
#endif
}

void GMenu2X::ledOff() {
#ifdef PLATFORM_GP2X
//	if (gp2x_mem!=0 && !f200) gp2x_memregs[0x106E >> 1] ^= 16;
	//SDL_SYS_JoystickGp2xSys(joy.joystick, BATT_LED_OFF);
#endif
}

void GMenu2X::setBacklight(int val)
{
	if (backlightHandle) {
		fprintf(backlightHandle, "%d", (val * 255) / 100);
		fflush(backlightHandle);
		rewind(backlightHandle);
	}
}

int GMenu2X::getBackLight()
{
	int val = 255;
	if (backlightHandle) {
		fscanf(backlightHandle, "%d", &val);
		rewind(backlightHandle);
		val = (val * 100) / 255;
		if (val < 0) val = 0; else if (val > 100) val = 100;
	}
	return val;
}

void GMenu2X::main() {
	uint linksPerPage = linkColumns*linkRows;
	int linkSpacingX = (resX-10 - linkColumns*skinConfInt["linkWidth"])/linkColumns;
	int linkSpacingY = (resY-35 - skinConfInt["topBarHeight"] - linkRows*skinConfInt["linkHeight"])/linkRows;
	uint sectionLinkPadding = (skinConfInt["topBarHeight"] - 32 - font->getLineHeight()) / 3;

	bool quit = false;
	int x,y, offset = menu->sectionLinks()->size()>linksPerPage ? 2 : 6;
#ifdef PLATFORM_GP2X
	int helpBoxHeight = fwType=="open2x" ? 154 : 139;
#else
	int helpBoxHeight = 154;
#endif
	uint i;
	long tickBattery = -60000, tickNow;
	string batteryIcon = "imgs/battery/0.png";
	stringstream ss;
	uint sectionsCoordX = 24;
	SDL_Rect re = {0,0,0,0};
	bool helpDisplayed = false;
#ifdef WITH_DEBUG
	//framerate
	long tickFPS = SDL_GetTicks();
	int drawn_frames = 0;
	string fps = "";
#endif

	IconButton btnContextMenu(this,"skin:imgs/menu.png");
	btnContextMenu.setPosition(resX-38, bottomBarIconY);
	btnContextMenu.setAction(MakeDelegate(this, &GMenu2X::contextMenu));

	if (!fileExists(CARD_ROOT))
		CARD_ROOT = "/";

	while (!quit) {
		tickNow = SDL_GetTicks();

		//Background
		sc["bgmain"]->blit(s,0,0);

		//Sections
		sectionsCoordX = halfX - (constrain((uint)menu->getSections().size(), 0 , linkColumns) * skinConfInt["linkWidth"]) / 2;
		if (menu->firstDispSection()>0)
			sc.skinRes("imgs/l_enabled.png")->blit(s,0,0);
		else
			sc.skinRes("imgs/l_disabled.png")->blit(s,0,0);
		if (menu->firstDispSection()+linkColumns<menu->getSections().size())
			sc.skinRes("imgs/r_enabled.png")->blit(s,resX-10,0);
		else
			sc.skinRes("imgs/r_disabled.png")->blit(s,resX-10,0);
		for (i=menu->firstDispSection(); i<menu->getSections().size() && i<menu->firstDispSection()+linkColumns; i++) {
			string sectionIcon = "skin:sections/"+menu->getSections()[i]+".png";
			x = (i-menu->firstDispSection())*skinConfInt["linkWidth"]+sectionsCoordX;
			if (menu->selSectionIndex()==(int)i)
				s->box(x, 0, skinConfInt["linkWidth"],
				skinConfInt["topBarHeight"], skinConfColors[COLOR_SELECTION_BG]);
			x += skinConfInt["linkWidth"]/2;
			if (sc.exists(sectionIcon))
				sc[sectionIcon]->blit(s,x-16,sectionLinkPadding,32,32);
			else
				sc.skinRes("icons/section.png")->blit(s,x-16,sectionLinkPadding);
			s->write( font, menu->getSections()[i], x, skinConfInt["topBarHeight"]-sectionLinkPadding, ASFont::HAlignCenter, ASFont::VAlignBottom );
		}

		//Links
		s->setClipRect(offset,skinConfInt["topBarHeight"],resX-9,resY-74); //32*2+10
		for (i=menu->firstDispRow()*linkColumns; i<(menu->firstDispRow()*linkColumns)+linksPerPage && i<menu->sectionLinks()->size(); i++) {
			int ir = i-menu->firstDispRow()*linkColumns;
			x = (ir%linkColumns)*(skinConfInt["linkWidth"]+linkSpacingX)+offset;
			y = ir/linkColumns*(skinConfInt["linkHeight"]+linkSpacingY)+skinConfInt["topBarHeight"]+2;
			menu->sectionLinks()->at(i)->setPosition(x,y);

			if (i==(uint)menu->selLinkIndex())
				menu->sectionLinks()->at(i)->paintHover();

			menu->sectionLinks()->at(i)->paint();
		}
		s->clearClipRect();

		drawScrollBar(linkRows,menu->sectionLinks()->size()/linkColumns + ((menu->sectionLinks()->size()%linkColumns==0) ? 0 : 1),menu->firstDispRow(),43,resY-81);

        /*
		switch(volumeMode) {
			case VOLUME_MODE_MUTE:   sc.skinRes("imgs/mute.png")->blit(s,279,bottomBarIconY); break;
			case VOLUME_MODE_PHONES: sc.skinRes("imgs/phones.png")->blit(s,279,bottomBarIconY); break;
			default: sc.skinRes("imgs/volume.png")->blit(s,279,bottomBarIconY); break;
		}
        */

		if (menu->selLink()!=NULL) {
			s->write ( font, menu->selLink()->getDescription(), halfX, resY-19, ASFont::HAlignCenter, ASFont::VAlignBottom );
			if (menu->selLinkApp()!=NULL) {
				s->write ( font, menu->selLinkApp()->clockStr(confInt["maxClock"]), cpuX, bottomBarTextY, ASFont::HAlignLeft, ASFont::VAlignMiddle );
				s->write ( font, menu->selLinkApp()->volumeStr(), volumeX, bottomBarTextY, ASFont::HAlignLeft, ASFont::VAlignMiddle );
				//Manual indicator
				if (!menu->selLinkApp()->getManual().empty())
					sc.skinRes("imgs/manual.png")->blit(s,manualX,bottomBarIconY);
			}
		}

		if (ts.initialized()) {
			btnContextMenu.paint();
		}
		//check battery status every 60 seconds
		if (tickNow-tickBattery >= 60000) {
			tickBattery = tickNow;
			unsigned short battlevel = getBatteryLevel();
			if (battlevel>5) {
				batteryIcon = "imgs/battery/ac.png";
			} else {
				ss.clear();
				ss << battlevel;
				ss >> batteryIcon;
				batteryIcon = "imgs/battery/"+batteryIcon+".png";
			}
		}
		sc.skinRes(batteryIcon)->blit( s, resX-19, bottomBarIconY );
		//s->write( font, tr[batstr.c_str()], 20, 170 );
		//On Screen Help


		if (helpDisplayed) {
			s->box(10,50,300,helpBoxHeight+4, skinConfColors[COLOR_MESSAGE_BOX_BG]);
			s->rectangle( 12,52,296,helpBoxHeight,
			skinConfColors[COLOR_MESSAGE_BOX_BORDER] );
			s->write( font, tr["CONTROLS"], 20, 60 );
#ifdef PLATFORM_DINGUX
			s->write( font, tr["A: Launch link / Confirm action"], 20, 80 );
			s->write( font, tr["B: Show this help menu"], 20, 95 );
			s->write( font, tr["L, R: Change section"], 20, 110 );
			s->write( font, tr["SELECT: Show contextual menu"], 20, 155 );
			s->write( font, tr["START: Show options menu"], 20, 170 );
#endif
#ifdef PLATFORM_GP2X
			s->write( font, tr["B, Stick press: Launch link / Confirm action"], 20, 80 );
			s->write( font, tr["L, R: Change section"], 20, 95 );
			s->write( font, tr["VOLUP, VOLDOWN: Change cpu clock"], 20, 125 );
			s->write( font, tr["A+VOLUP, A+VOLDOWN: Change volume"], 20, 140 );
			s->write( font, tr["SELECT: Show contextual menu"], 20, 155 );
			s->write( font, tr["START: Show options menu"], 20, 170 );
			if (fwType=="open2x") s->write( font, tr["X: Toggle speaker mode"], 20, 185 );
#endif
			s->flip();
			while (input.waitForPressedButton() != CANCEL) {}
			helpDisplayed=false;
			continue;
		}

#ifdef WITH_DEBUG
		//framerate
		drawn_frames++;
		if (tickNow-tickFPS>=1000) {
			ss.clear();
			ss << drawn_frames*(tickNow-tickFPS+1)/1000;
			ss >> fps;
			tickFPS = tickNow;
			drawn_frames = 0;
		}
		s->write( font, fps+" FPS", resX-1,1 ,ASFont::HAlignRight );
#endif

		s->flip();

		//touchscreen
		if (ts.initialized()) {
			ts.poll();
			btnContextMenu.handleTS();
			re.x = 0; re.y = 0; re.h = skinConfInt["topBarHeight"]; re.w = resX;
			if (ts.pressed() && ts.inRect(re)) {
				re.w = skinConfInt["linkWidth"];
				sectionsCoordX = halfX - (constrain((uint)menu->getSections().size(), 0 , linkColumns) * skinConfInt["linkWidth"]) / 2;
				for (i=menu->firstDispSection(); !ts.handled() && i<menu->getSections().size() && i<menu->firstDispSection()+linkColumns; i++) {
					re.x = (i-menu->firstDispSection())*re.w+sectionsCoordX;

					if (ts.inRect(re)) {
						menu->setSectionIndex(i);
						ts.setHandled();
					}
				}
			}

			i=menu->firstDispRow()*linkColumns;
			while ( i<(menu->firstDispRow()*linkColumns)+linksPerPage && i<menu->sectionLinks()->size()) {
				if (menu->sectionLinks()->at(i)->isPressed())
					menu->setLinkIndex(i);
				if (menu->sectionLinks()->at(i)->handleTS())
					i = menu->sectionLinks()->size();
				i++;
			}
		}

        switch (input.waitForPressedButton()) {
            case ACCEPT:
                if (menu->selLink() != NULL) menu->selLink()->run();
                break;
            case CANCEL:
                helpDisplayed=true;
                break;
            case SETTINGS:
                options();
                break;
            case MENU:
                contextMenu();
                break;
            case UP:
                menu->linkUp();
                break;
            case DOWN:
                menu->linkDown();
                break;
            case LEFT:
                menu->linkLeft();
                break;
            case RIGHT:
                menu->linkRight();
                break;
            case ALTLEFT:
				menu->decSectionIndex();
				offset = menu->sectionLinks()->size()>linksPerPage ? 2 : 6;
                break;
            case ALTRIGHT:
				menu->incSectionIndex();
				offset = menu->sectionLinks()->size()>linksPerPage ? 2 : 6;
                break;
            default:
                break;
        }

        /*
		while (!input.update())
			usleep(LOOP_DELAY);
		if ( input[ACCEPT] && menu->selLink()!=NULL ) menu->selLink()->run();
		else if ( input[SETTINGS]  ) options();
		else if ( input[MENU] ) contextMenu();
		// VOLUME SCALE MODIFIER
		else if ( fwType=="open2x" && input[CLEAR] ) {
			volumeMode = constrain(volumeMode-1, -VOLUME_MODE_MUTE-1, VOLUME_MODE_NORMAL);
			if(volumeMode < VOLUME_MODE_MUTE)
				volumeMode = VOLUME_MODE_NORMAL;
			switch(volumeMode) {
				case VOLUME_MODE_MUTE:   setVolumeScaler(VOLUME_SCALER_MUTE); break;
				case VOLUME_MODE_PHONES: setVolumeScaler(volumeScalerPhones); break;
				case VOLUME_MODE_NORMAL: setVolumeScaler(volumeScalerNormal); break;
			}
			setVolume(confInt["globalVolume"]);
		}
		// LINK NAVIGATION
		else if ( input[ALTLEFTEFT ]  ) menu->linkLeft();
		else if ( input[ALTRIGHTIGHT]  ) menu->linkRight();
		else if ( input[UP   ]  ) menu->linkUp();
		else if ( input[DOWN ]  ) menu->linkDown();
		// SELLINKAPP SELECTED
		else if (menu->selLinkApp()!=NULL) {
			if ( input[MANUAL] ) menu->selLinkApp()->showManual();
			else if ( input.isActive(CANCEL) ) {
				// VOLUME
				if ( input[VOLDOWN] && !input.isActive(VOLUP) )
					menu->selLinkApp()->setVolume( constrain(menu->selLinkApp()->volume()-1,0,100) );
				if ( input[VOLUP] && !input.isActive(VOLDOWN) )
					menu->selLinkApp()->setVolume( constrain(menu->selLinkApp()->volume()+1,0,100) );;
				if ( input.isActive(VOLUP) && input.isActive(VOLDOWN) ) menu->selLinkApp()->setVolume(-1);
			} else {
				// CLOCK
				if ( input[VOLDOWN] && !input.isActive(VOLUP) )
					menu->selLinkApp()->setClock( constrain(menu->selLinkApp()->clock()-1,200,confInt["maxClock"]) );
				if ( input[VOLUP] && !input.isActive(VOLDOWN) )
					menu->selLinkApp()->setClock( constrain(menu->selLinkApp()->clock()+1,200,confInt["maxClock"]) );
				if ( input.isActive(VOLUP) && input.isActive(VOLDOWN) ) menu->selLinkApp()->setClock(336);
			}
		}
		if ( input.isActive(CANCEL) ) {
			if (input.isActive(ALTLEFT) && input.isActive(ALTRIGHT))
				saveScreenshot();
		} else {
			// SECTIONS
			if ( input[ALTLEFT     ] ) {
				menu->decSectionIndex();
				offset = menu->sectionLinks()->size()>linksPerPage ? 2 : 6;
			} else if ( input[ALTRIGHT     ] ) {
				menu->incSectionIndex();
				offset = menu->sectionLinks()->size()>linksPerPage ? 2 : 6;
			}
		}
        */
	}
}

void GMenu2X::explorer() {
	FileDialog fd(this,tr["Select an application"],".gpu,.dge,.sh,");
	if (fd.exec()) {
		if (confInt["saveSelection"] && (confInt["section"]!=menu->selSectionIndex() || confInt["link"]!=menu->selLinkIndex()))
			writeConfig();
#ifdef PLATFORM_GP2X
		if (fwType == "open2x" && savedVolumeMode != volumeMode)
			writeConfigOpen2x();
#endif

		//string command = cmdclean(fd.path()+"/"+fd.file) + "; sync & cd "+cmdclean(getExePath())+"; exec ./gmenu2x";
		string command = cmdclean(fd.getPath()+"/"+fd.getFile());
		chdir(fd.getPath().c_str());
		quit();
		setClock(cpuFreqAppDefault);
		execlp("/bin/sh","/bin/sh","-c",command.c_str(),NULL);

		//if execution continues then something went wrong and as we already called SDL_Quit we cannot continue
		//try relaunching gmenu2x
		ERROR("Error executing selected application, re-launching gmenu2x\n");
		chdir(getExePath().c_str());
		execlp("./gmenu2x", "./gmenu2x", NULL);
	}
}

void GMenu2X::options() {
	int curMenuClock = confInt["menuClock"];
	int curGlobalVolume = confInt["globalVolume"];
	//G
	int prevbacklight = confInt["backlight"];
	bool showRootFolder = fileExists(CARD_ROOT);

	FileLister fl_tr(getHome() + "/translations");
	fl_tr.browse();
	fl_tr.setPath(GMENU2X_SYSTEM_DIR "/translations", false);
	fl_tr.browse(false);

	fl_tr.insertFile("English");
	string lang = tr.lang();

	vector<string> encodings;
	encodings.push_back("NTSC");
	encodings.push_back("PAL");

	SettingsDialog sd(this, input, ts, tr["Settings"]);
	sd.addSetting(new MenuSettingMultiString(this,tr["Language"],tr["Set the language used by GMenu2X"],&lang,&fl_tr.getFiles()));
	sd.addSetting(new MenuSettingBool(this,tr["Save last selection"],tr["Save the last selected link and section on exit"],&confInt["saveSelection"]));
	sd.addSetting(new MenuSettingInt(this,tr["Clock for GMenu2X"],tr["Set the cpu working frequency when running GMenu2X"],&confInt["menuClock"],cpuFreqMin,cpuFreqSafeMax,cpuFreqMultiple));
	sd.addSetting(new MenuSettingInt(this,tr["Maximum overclock"],tr["Set the maximum overclock for launching links"],&confInt["maxClock"],cpuFreqMin,cpuFreqMax,cpuFreqMultiple));
	sd.addSetting(new MenuSettingInt(this,tr["Global Volume"],tr["Set the default volume for the gp2x soundcard"],&confInt["globalVolume"],0,100));
	sd.addSetting(new MenuSettingBool(this,tr["Output logs"],tr["Logs the output of the links. Use the Log Viewer to read them."],&confInt["outputLogs"]));
	//G
	sd.addSetting(new MenuSettingInt(this,tr["Lcd Backlight"],tr["Set dingoo's Lcd Backlight value (default: 100)"],&confInt["backlight"],5,100));
	sd.addSetting(new MenuSettingInt(this,tr["Screen Timeout"],tr["Set screen's backlight timeout in seconds"],&confInt["backlightTimeout"],0,120));
//	sd.addSetting(new MenuSettingMultiString(this,tr["Tv-Out encoding"],tr["Encoding of the tv-out signal"],&confStr["tvoutEncoding"],&encodings));
	sd.addSetting(new MenuSettingBool(this,tr["Show root"],tr["Show root folder in the file selection dialogs"],&showRootFolder));

	if (sd.exec() && sd.edited()) {
		//G
		if (prevbacklight != confInt["backlight"]) setBacklight(confInt["backlight"]);
		if (curMenuClock!=confInt["menuClock"]) setClock(confInt["menuClock"]);
		if (curGlobalVolume!=confInt["globalVolume"]) setVolume(confInt["globalVolume"]);

		if (confInt["backlightTimeout"] == 0) {
			if (PowerSaver::isRunning())
				delete PowerSaver::getInstance();
		} else {
			PowerSaver::getInstance()->setScreenTimeout( confInt["backlightTimeout"] );
		}

		if (lang == "English") lang = "";
		if (lang != tr.lang()) {
			tr.setLang(lang);
			confStr["lang"] = lang;
		}
		/*if (fileExists(CARD_ROOT) && !showRootFolder)
			unlink(CARD_ROOT);
		else if (!fileExists(CARD_ROOT) && showRootFolder)
			symlink("/", CARD_ROOT);*/
		//WARNING: the above might be dangerous with CARD_ROOT set to /
		writeConfig();
	}
}

#ifdef PLATFORM_GP2X
void GMenu2X::settingsOpen2x() {
	SettingsDialog sd(this, input, ts, tr["Open2x Settings"]);
	sd.addSetting(new MenuSettingBool(this,tr["USB net on boot"],tr["Allow USB networking to be started at boot time"],&o2x_usb_net_on_boot));
	sd.addSetting(new MenuSettingString(this,tr["USB net IP"],tr["IP address to be used for USB networking"],&o2x_usb_net_ip));
	sd.addSetting(new MenuSettingBool(this,tr["Telnet on boot"],tr["Allow telnet to be started at boot time"],&o2x_telnet_on_boot));
	sd.addSetting(new MenuSettingBool(this,tr["FTP on boot"],tr["Allow FTP to be started at boot time"],&o2x_ftp_on_boot));
	sd.addSetting(new MenuSettingBool(this,tr["GP2XJOY on boot"],tr["Create a js0 device for GP2X controls"],&o2x_gp2xjoy_on_boot));
	sd.addSetting(new MenuSettingBool(this,tr["USB host on boot"],tr["Allow USB host to be started at boot time"],&o2x_usb_host_on_boot));
	sd.addSetting(new MenuSettingBool(this,tr["USB HID on boot"],tr["Allow USB HID to be started at boot time"],&o2x_usb_hid_on_boot));
	sd.addSetting(new MenuSettingBool(this,tr["USB storage on boot"],tr["Allow USB storage to be started at boot time"],&o2x_usb_storage_on_boot));
	//sd.addSetting(new MenuSettingInt(this,tr["Speaker Mode on boot"],tr["Set Speaker mode. 0 = Mute, 1 = Phones, 2 = Speaker"],&volumeMode,0,2));
	sd.addSetting(new MenuSettingInt(this,tr["Speaker Scaler"],tr["Set the Speaker Mode scaling 0-150\% (default is 100\%)"],&volumeScalerNormal,0,150));
	sd.addSetting(new MenuSettingInt(this,tr["Headphones Scaler"],tr["Set the Headphones Mode scaling 0-100\% (default is 65\%)"],&volumeScalerPhones,0,100));

	if (sd.exec() && sd.edited()) {
		writeConfigOpen2x();
		switch(volumeMode) {
			case VOLUME_MODE_MUTE:   setVolumeScaler(VOLUME_SCALER_MUTE); break;
			case VOLUME_MODE_PHONES: setVolumeScaler(volumeScalerPhones);   break;
			case VOLUME_MODE_NORMAL: setVolumeScaler(volumeScalerNormal); break;
		}
		setVolume(confInt["globalVolume"]);
	}
}
#endif

void GMenu2X::skinMenu() {
	FileLister fl_sk(getHome() + "/skins", true, false);
	fl_sk.addExclude("..");
	fl_sk.browse();
	fl_sk.setPath(GMENU2X_SYSTEM_DIR "/skins", false);
	fl_sk.browse(false);

	string curSkin = confStr["skin"];

	SettingsDialog sd(this, input, ts, tr["Skin"]);
	sd.addSetting(new MenuSettingMultiString(this,tr["Skin"],tr["Set the skin used by GMenu2X"],&confStr["skin"],&fl_sk.getDirectories()));
	sd.addSetting(new MenuSettingRGBA(this,tr["Top Bar Color"],tr["Color of the top bar"],&skinConfColors[COLOR_TOP_BAR_BG]));
	sd.addSetting(new MenuSettingRGBA(this,tr["Bottom Bar Color"],tr["Color of the bottom bar"],&skinConfColors[COLOR_BOTTOM_BAR_BG]));
	sd.addSetting(new MenuSettingRGBA(this,tr["Selection Color"],tr["Color of the selection and other interface details"],&skinConfColors[COLOR_SELECTION_BG]));
	sd.addSetting(new MenuSettingRGBA(this,tr["Message Box Color"],tr["Background color of the message box"],&skinConfColors[COLOR_MESSAGE_BOX_BG]));
	sd.addSetting(new MenuSettingRGBA(this,tr["Message Box Border Color"],tr["Border color of the message box"],&skinConfColors[COLOR_MESSAGE_BOX_BORDER]));
	sd.addSetting(new MenuSettingRGBA(this,tr["Message Box Selection Color"],tr["Color of the selection of the message box"],&skinConfColors[COLOR_MESSAGE_BOX_SELECTION]));

	if (sd.exec() && sd.edited()) {
		if (curSkin != confStr["skin"]) {
			setSkin(confStr["skin"]);
			writeConfig();
		}
		writeSkinConfig();
		initBG();
	}
}

void GMenu2X::toggleTvOut() {
#ifdef PLATFORM_GP2X
/*	if (cx25874!=0)
		gp2x_tvout_off();
	else
		gp2x_tvout_on(confStr["tvoutEncoding"] == "PAL");*/
#endif
}

void GMenu2X::setSkin(const string &skin, bool setWallpaper) {
	confStr["skin"] = skin;

	//Clear previous skin settings
	skinConfStr.clear();
	skinConfInt.clear();

	DEBUG("GMenu2X: setting new skin %s.\n", skin.c_str());

	//clear collection and change the skin path
	sc.clear();
	sc.setSkin(skin);

	//reset colors to the default values
	skinConfColors[COLOR_TOP_BAR_BG] = (RGBAColor){255,255,255,130};
	skinConfColors[COLOR_BOTTOM_BAR_BG] = (RGBAColor){255,255,255,130};
	skinConfColors[COLOR_SELECTION_BG] = (RGBAColor){255,255,255,130};
	skinConfColors[COLOR_MESSAGE_BOX_BG] = (RGBAColor){255,255,255,255};
	skinConfColors[COLOR_MESSAGE_BOX_BORDER] = (RGBAColor){80,80,80,255};
	skinConfColors[COLOR_MESSAGE_BOX_SELECTION] = (RGBAColor){160,160,160,255};

	/* Load skin settings from user directory if present,
	 * or from the system directory. */
	string skinconfname = getHome() + "/skins/" + skin + "/skin.conf";
	if (!fileExists(skinconfname))
	  skinconfname = GMENU2X_SYSTEM_DIR "/skins/" + skin + "/skin.conf";

	if (fileExists(skinconfname)) {
		ifstream skinconf(skinconfname.c_str(), ios_base::in);
		if (skinconf.is_open()) {
			string line;
			while (getline(skinconf, line, '\n')) {
				line = trim(line);
				DEBUG("skinconf: '%s'\n", line.c_str());
				string::size_type pos = line.find("=");
				string name = trim(line.substr(0,pos));
				string value = trim(line.substr(pos+1,line.length()));

				if (value.length()>0) {
					if (value.length()>1 && value.at(0)=='"' && value.at(value.length()-1)=='"')
						skinConfStr[name] = value.substr(1,value.length()-2);
					else if (value.at(0) == '#')
						skinConfColors[stringToColor(name)] = strtorgba( value.substr(1,value.length()) );
					else
						skinConfInt[name] = atoi(value.c_str());
				}
			}
			skinconf.close();

			if (setWallpaper && !skinConfStr["wallpaper"].empty() && fileExists("skins/"+skin+"/wallpapers/"+skinConfStr["wallpaper"]))
				confStr["wallpaper"] = "skins/"+skin+"/wallpapers/"+skinConfStr["wallpaper"];
		}
	}

	evalIntConf( &skinConfInt["topBarHeight"], 40, 32,120 );
	evalIntConf( &skinConfInt["linkHeight"], 40, 32,120 );
	evalIntConf( &skinConfInt["linkWidth"], 60, 32,120 );

	//recalculate some coordinates based on the new element sizes
	linkColumns = (resX-10)/skinConfInt["linkWidth"];
	linkRows = (resY-35-skinConfInt["topBarHeight"])/skinConfInt["linkHeight"];

	if (menu != NULL) menu->loadIcons();

	//Selection png
	useSelectionPng = sc.addSkinRes("imgs/selection.png", false) != NULL;

	//font
	initFont();
}

/*
void GMenu2X::activateSdUsb() {
	if (usbnet) {
		MessageBox mb(this,tr["Operation not permitted."]+"\n"+tr["You should disable Usb Networking to do this."]);
		mb.exec();
	} else {
		system("scripts/usbon.sh sd");
		MessageBox mb(this,tr["USB Enabled (SD)"],"icons/usb.png");
		mb.setButton(ACCEPT, tr["Turn off"]);
		mb.exec();
		system("scripts/usboff.sh sd");
	}
}

void GMenu2X::activateNandUsb() {
	if (usbnet) {
		MessageBox mb(this,tr["Operation not permitted."]+"\n"+tr["You should disable Usb Networking to do this."]);
		mb.exec();
	} else {
		system("scripts/usbon.sh nand");
		MessageBox mb(this,tr["USB Enabled (Nand)"],"icons/usb.png");
		mb.setButton(ACCEPT, tr["Turn off"]);
		mb.exec();
		system("scripts/usboff.sh nand");
	}
}

void GMenu2X::activateRootUsb() {
	if (usbnet) {
		MessageBox mb(this,tr["Operation not permitted."]+"\n"+tr["You should disable Usb Networking to do this."]);
		mb.exec();
	} else {
		system("scripts/usbon.sh root");
		MessageBox mb(this,tr["USB Enabled (Root)"],"icons/usb.png");
		mb.setButton(ACCEPT, tr["Turn off"]);
		mb.exec();
		system("scripts/usboff.sh root");
	}
}
*/

void GMenu2X::showManual() {
	menu->selLinkApp()->showManual();
}

void GMenu2X::contextMenu() {
	vector<MenuOption> voices;
	{
	MenuOption opt = {tr.translate("Add link in $1",menu->selSection().c_str(),NULL), MakeDelegate(this, &GMenu2X::addLink)};
	voices.push_back(opt);
	}

	{
		LinkApp* app = menu->selLinkApp();
		if (app && fileExists(app->getManual())) {
			MenuOption opt = {tr.translate("Show manual of $1",menu->selLink()->getTitle().c_str(),NULL),
				MakeDelegate(this, &GMenu2X::showManual),
			};
			voices.push_back(opt);
		}
	}

	if (menu->selLinkApp()!=NULL) {
		{
		MenuOption opt = {tr.translate("Edit $1",menu->selLink()->getTitle().c_str(),NULL), MakeDelegate(this, &GMenu2X::editLink)};
		voices.push_back(opt);
		}{
		MenuOption opt = {tr.translate("Delete $1 link",menu->selLink()->getTitle().c_str(),NULL), MakeDelegate(this, &GMenu2X::deleteLink)};
		voices.push_back(opt);
		}
	}

	{
	MenuOption opt = {tr["Add section"], MakeDelegate(this, &GMenu2X::addSection)};
	voices.push_back(opt);
	}{
	MenuOption opt = {tr["Rename section"], MakeDelegate(this, &GMenu2X::renameSection)};
	voices.push_back(opt);
	}{
	MenuOption opt = {tr["Delete section"], MakeDelegate(this, &GMenu2X::deleteSection)};
	voices.push_back(opt);
	}{
	MenuOption opt = {tr["Scan for applications and games"], MakeDelegate(this, &GMenu2X::scanner)};
	voices.push_back(opt);
	}

	bool close = false;
	uint i, sel=0, fadeAlpha=0;

	int h = font->getHeight();
	SDL_Rect box;
	box.h = (h+2)*voices.size()+8;
	box.w = 0;
	for (i=0; i<voices.size(); i++) {
		int w = font->getTextWidth(voices[i].text);
		if (w>box.w) box.w = w;
	}
	box.w += 23;
	box.x = halfX - box.w/2;
	box.y = halfY - box.h/2;

	SDL_Rect selbox = {box.x+4, 0, box.w-8, h+2};
	long tickNow, tickStart = SDL_GetTicks();

	Surface bg(s);
	/*//Darken background
	bg.box(0, 0, resX, resY, 0,0,0,150);
	bg.box(box.x, box.y, box.w, box.h, skinConfColors["messageBoxBg"]);
	bg.rectangle( box.x+2, box.y+2, box.w-4, box.h-4, skinConfColors["messageBoxBorder"] );*/

    bevent_t event;
	while (!close) {
		tickNow = SDL_GetTicks();

		selbox.y = box.y+4+(h+2)*sel;
		bg.blit(s,0,0);

		if (fadeAlpha<200) fadeAlpha = intTransition(0,200,tickStart,500,tickNow);
		s->box(0, 0, resX, resY, 0,0,0,fadeAlpha);
		s->box(box.x, box.y, box.w, box.h, skinConfColors[COLOR_MESSAGE_BOX_BG]);
		s->rectangle( box.x+2, box.y+2, box.w-4, box.h-4, skinConfColors[COLOR_MESSAGE_BOX_BORDER] );


		//draw selection rect
		s->box( selbox.x, selbox.y, selbox.w, selbox.h, skinConfColors[COLOR_MESSAGE_BOX_SELECTION] );
		for (i=0; i<voices.size(); i++)
			s->write( font, voices[i].text, box.x+12, box.y+5+(h+2)*i, ASFont::HAlignLeft, ASFont::VAlignTop );
		s->flip();

		//touchscreen
		if (ts.initialized()) {
			ts.poll();
			if (ts.released()) {
				if (!ts.inRect(box))
					close = true;
				else if (ts.getX() >= selbox.x
					  && ts.getX() <= selbox.x + selbox.w)
					for (i=0; i<voices.size(); i++) {
						selbox.y = box.y+4+(h+2)*i;
						if (ts.getY() >= selbox.y
						 && ts.getY() <= selbox.y + selbox.h) {
							voices[i].action();
							close = true;
							i = voices.size();
						}
					}
			} else if (ts.pressed() && ts.inRect(box)) {
				for (i=0; i<voices.size(); i++) {
					selbox.y = box.y+4+(h+2)*i;
					if (ts.getY() >= selbox.y
					 && ts.getY() <= selbox.y + selbox.h) {
						sel = i;
						i = voices.size();
					}
				}
			}
		}


        if (fadeAlpha < 200) {
            if (!input.pollEvent(&event) || event.state != PRESSED) continue;
        } else {
            event.button = input.waitForPressedButton();
        }

        switch(event.button) {
            case MENU:
                close = true;
                break;
            case UP:
                sel = max(0, sel-1);
                break;
            case DOWN:
                sel = min((int)voices.size()-1, sel+1);
                break;
            case ACCEPT:
                voices[sel].action();
                close = true;
                break;
            default:
                break;
        }
	}
}

void GMenu2X::changeWallpaper() {
	WallpaperDialog wp(this);
	if (wp.exec() && confStr["wallpaper"] != wp.wallpaper) {
		confStr["wallpaper"] = wp.wallpaper;
		initBG();
		writeConfig();
	}
}

void GMenu2X::addLink() {
	FileDialog fd(this,tr["Select an application"]);
	if (fd.exec()) {
		ledOn();
		menu->addLink(fd.getPath(), fd.getFile());
		sync();
		ledOff();
	}
}

void GMenu2X::editLink() {
	if (menu->selLinkApp()==NULL) return;

	vector<string> pathV;
	split(pathV,menu->selLinkApp()->getFile(),"/");
	string oldSection = "";
	if (pathV.size()>1)
		oldSection = pathV[pathV.size()-2];
	string newSection = oldSection;

	string linkTitle = menu->selLinkApp()->getTitle();
	string linkDescription = menu->selLinkApp()->getDescription();
	string linkIcon = menu->selLinkApp()->getIcon();
	string linkManual = menu->selLinkApp()->getManual();
	string linkParams = menu->selLinkApp()->getParams();
	string linkSelFilter = menu->selLinkApp()->getSelectorFilter();
	string linkSelDir = menu->selLinkApp()->getSelectorDir();
	bool linkSelBrowser = menu->selLinkApp()->getSelectorBrowser();
	bool linkUseRamTimings = menu->selLinkApp()->getUseRamTimings();
	string linkSelScreens = menu->selLinkApp()->getSelectorScreens();
	string linkSelAliases = menu->selLinkApp()->getAliasFile();
	int linkClock = menu->selLinkApp()->clock();
	int linkVolume = menu->selLinkApp()->volume();
	//G
	//int linkGamma = menu->selLinkApp()->gamma();
	int linkBacklight = menu->selLinkApp()->backlight();

	string diagTitle = tr.translate("Edit link: $1",linkTitle.c_str(),NULL);
	string diagIcon = menu->selLinkApp()->getIconPath();

	SettingsDialog sd(this, input, ts, diagTitle, diagIcon);
	sd.addSetting(new MenuSettingString(this,tr["Title"],tr["Link title"],&linkTitle, diagTitle,diagIcon));
	sd.addSetting(new MenuSettingString(this,tr["Description"],tr["Link description"],&linkDescription, diagTitle,diagIcon));
	sd.addSetting(new MenuSettingMultiString(this,tr["Section"],tr["The section this link belongs to"],&newSection,&menu->getSections()));
	sd.addSetting(new MenuSettingImage(this,tr["Icon"],tr.translate("Select an icon for the link: $1",linkTitle.c_str(),NULL),&linkIcon,".png,.bmp,.jpg,.jpeg"));
	sd.addSetting(new MenuSettingFile(this,tr["Manual"],tr["Select a graphic/textual manual or a readme"],&linkManual,".man.png,.txt"));
	sd.addSetting(new MenuSettingInt(this,tr["Clock (default: 336)"],tr["Cpu clock frequency to set when launching this link"],&linkClock,cpuFreqMin,confInt["maxClock"],cpuFreqMultiple));
//	sd.addSetting(new MenuSettingBool(this,tr["Tweak RAM Timings"],tr["This usually speeds up the application at the cost of stability"],&linkUseRamTimings));
	sd.addSetting(new MenuSettingInt(this,tr["Volume (default: -1)"],tr["Volume to set for this link"],&linkVolume,-1,100));
	sd.addSetting(new MenuSettingInt(this,tr["Backlight (default: -1)"],tr["LCD backlight value to set when launching this link"],&linkBacklight,-1,100));
	sd.addSetting(new MenuSettingString(this,tr["Parameters"],tr["Parameters to pass to the application"],&linkParams, diagTitle,diagIcon));
	sd.addSetting(new MenuSettingDir(this,tr["Selector Directory"],tr["Directory to scan for the selector"],&linkSelDir));
	sd.addSetting(new MenuSettingBool(this,tr["Selector Browser"],tr["Allow the selector to change directory"],&linkSelBrowser));
	sd.addSetting(new MenuSettingString(this,tr["Selector Filter"],tr["Selector filter (Separate values with a comma)"],&linkSelFilter, diagTitle,diagIcon));
	sd.addSetting(new MenuSettingDir(this,tr["Selector Screenshots"],tr["Directory of the screenshots for the selector"],&linkSelScreens));
	sd.addSetting(new MenuSettingFile(this,tr["Selector Aliases"],tr["File containing a list of aliases for the selector"],&linkSelAliases));
	//G
	sd.addSetting(new MenuSettingBool(this,tr["Wrapper"],tr["Explicitly relaunch GMenu2X after execution"],&menu->selLinkApp()->needsWrapperRef()));
	sd.addSetting(new MenuSettingBool(this,tr["Don't Leave"],tr["Don't quit GMenu2X when launching this link"],&menu->selLinkApp()->runsInBackgroundRef()));

	if (sd.exec() && sd.edited()) {
		ledOn();

		menu->selLinkApp()->setTitle(linkTitle);
		menu->selLinkApp()->setDescription(linkDescription);
		menu->selLinkApp()->setIcon(linkIcon);
		menu->selLinkApp()->setManual(linkManual);
		menu->selLinkApp()->setParams(linkParams);
		menu->selLinkApp()->setSelectorFilter(linkSelFilter);
		menu->selLinkApp()->setSelectorDir(linkSelDir);
		menu->selLinkApp()->setSelectorBrowser(linkSelBrowser);
		menu->selLinkApp()->setUseRamTimings(linkUseRamTimings);
		menu->selLinkApp()->setSelectorScreens(linkSelScreens);
		menu->selLinkApp()->setAliasFile(linkSelAliases);
		menu->selLinkApp()->setClock(linkClock);
		menu->selLinkApp()->setVolume(linkVolume);
		//G
		if ((linkBacklight < 5) && (linkBacklight > -1))
			linkBacklight = 5;
		menu->selLinkApp()->setBacklight(linkBacklight);

		INFO("New Section: '%s'\n", newSection.c_str());

		//if section changed move file and update link->file
		if (oldSection!=newSection) {
			vector<string>::const_iterator newSectionIndex = find(menu->getSections().begin(),menu->getSections().end(),newSection);
			if (newSectionIndex==menu->getSections().end()) return;
			string newFileName = "sections/"+newSection+"/"+linkTitle;
			uint x=2;
			while (fileExists(newFileName)) {
				string id = "";
				stringstream ss; ss << x; ss >> id;
				newFileName = "sections/"+newSection+"/"+linkTitle+id;
				x++;
			}
			rename(menu->selLinkApp()->getFile().c_str(),newFileName.c_str());
			menu->selLinkApp()->renameFile(newFileName);

			INFO("New section index: %i.\n", newSectionIndex - menu->getSections().begin());

			menu->linkChangeSection(menu->selLinkIndex(), menu->selSectionIndex(), newSectionIndex - menu->getSections().begin());
		}
		menu->selLinkApp()->save();
		sync();

		ledOff();
	}
}

void GMenu2X::deleteLink() {
	if (menu->selLinkApp()!=NULL) {
		MessageBox mb(this, tr.translate("Deleting $1",menu->selLink()->getTitle().c_str(),NULL)+"\n"+tr["Are you sure?"], menu->selLink()->getIconPath());
		mb.setButton(ACCEPT, tr["Yes"]);
		mb.setButton(CANCEL, tr["No"]);
		if (mb.exec() == ACCEPT) {
			ledOn();
			menu->deleteSelectedLink();
			sync();
			ledOff();
		}
	}
}

void GMenu2X::addSection() {
	InputDialog id(this, input, ts, tr["Insert a name for the new section"]);
	if (id.exec()) {
		//only if a section with the same name does not exist
		if (find(menu->getSections().begin(), menu->getSections().end(), id.getInput())
				== menu->getSections().end()) {
			//section directory doesn't exists
			ledOn();
			if (menu->addSection(id.getInput())) {
				menu->setSectionIndex( menu->getSections().size()-1 ); //switch to the new section
				sync();
			}
			ledOff();
		}
	}
}

void GMenu2X::renameSection() {
	InputDialog id(this, input, ts, tr["Insert a new name for this section"],menu->selSection());
	if (id.exec()) {
		//only if a section with the same name does not exist & !samename
		if (menu->selSection() != id.getInput()
		 && find(menu->getSections().begin(),menu->getSections().end(), id.getInput())
				== menu->getSections().end()) {
			//section directory doesn't exists
			string newsectiondir = getHome() + "/sections/" + id.getInput();
			string sectiondir = getHome() + "/sections/" + menu->selSection();
			ledOn();

			if (!rename(sectiondir.c_str(), newsectiondir.c_str())) {
				string oldpng = menu->selSection() + ".png";
				string newpng = id.getInput() + ".png";
				string oldicon = sc.getSkinFilePath(oldpng);
				string newicon = sc.getSkinFilePath(newpng);

				if (!oldicon.empty() && newicon.empty()) {
					newicon = oldicon;
					newicon.replace(newicon.find(oldpng), oldpng.length(), newpng);

					if (!fileExists(newicon)) {
						rename(oldicon.c_str(), newicon.c_str());
						sc.move("skin:"+oldpng, "skin:"+newpng);
					}
				}
				menu->renameSection(menu->selSectionIndex(), id.getInput());
				sync();
			}
			ledOff();
		}
	}
}

void GMenu2X::deleteSection() {
	MessageBox mb(this,tr["You will lose all the links in this section."]+"\n"+tr["Are you sure?"]);
	mb.setButton(ACCEPT, tr["Yes"]);
	mb.setButton(CANCEL, tr["No"]);
	if (mb.exec() == ACCEPT) {
		ledOn();
		if (rmtree(getHome() + "/sections/" + menu->selSection())) {
			menu->deleteSelectedSection();
			sync();
		}
		ledOff();
	}
}

void GMenu2X::scanner() {
	Surface scanbg(bg);
	drawButton(&scanbg, "cancel", tr["Exit"],
	drawButton(&scanbg, "accept", "", 5)-10);
	scanbg.write(font,tr["Link Scanner"],halfX,7,ASFont::HAlignCenter,ASFont::VAlignMiddle);
	scanbg.convertToDisplayFormat();

	uint lineY = 42;

#ifdef PLATFORM_PANDORA
	//char *configpath = pnd_conf_query_searchpath();
#else
	if (confInt["menuClock"]<336) {
		setClock(336);
		scanbg.write(font,tr["Raising cpu clock to 336Mhz"],5,lineY);
		scanbg.blit(s,0,0);
		s->flip();
		lineY += 26;
	}

	scanbg.write(font,tr["Scanning SD filesystem..."],5,lineY);
	scanbg.blit(s,0,0);
	s->flip();
	lineY += 26;

	vector<string> files;
	scanPath(CARD_ROOT, &files);

	const char *nandpath = NULL;

#if defined(PLATFORM_GP2X)
	//Onyl gph firmware has nand
	if (fwType=="gph" && !isF200())
		nandpath = "/boot/local/nand";
#elif defined(PLATFORM_DINGUX)
	nandpath = "/media/ccnandb1";
#endif

	if (nandpath) {
		scanbg.write(font,tr["Scanning NAND filesystem..."],5,lineY);
		scanbg.blit(s,0,0);
		s->flip();
		lineY += 26;
		scanPath(nandpath, &files);
	}

	stringstream ss;
	ss << files.size();
	string str = "";
	ss >> str;
	scanbg.write(font,tr.translate("$1 files found.",str.c_str(),NULL),5,lineY);
	lineY += 26;
	scanbg.write(font,tr["Creating links..."],5,lineY);
	scanbg.blit(s,0,0);
	s->flip();
	lineY += 26;

	string path, file;
	string::size_type pos;
	uint linkCount = 0;

	ledOn();
	for (uint i = 0; i<files.size(); i++) {
		pos = files[i].rfind("/");
		if (pos!=string::npos && pos>0) {
			path = files[i].substr(0, pos+1);
			file = files[i].substr(pos+1, files[i].length());
			if (menu->addLink(path,file,"found "+file.substr(file.length()-3,3)))
				linkCount++;
		}
	}

	ss.clear();
	ss << linkCount;
	ss >> str;
	scanbg.write(font,tr.translate("$1 links created.",str.c_str(),NULL),5,lineY);
	scanbg.blit(s,0,0);
	s->flip();
	lineY += 26;

	if (confInt["menuClock"]<336) {
		setClock(confInt["menuClock"]);
		scanbg.write(font,tr["Decreasing cpu clock"],5,lineY);
		scanbg.blit(s,0,0);
		s->flip();
		lineY += 26;
	}

	sync();
	ledOff();
#endif

    buttontype_t button;
    do {
        button = input.waitForPressedButton();
    } while ((button != SETTINGS)
                && (button != ACCEPT)
                && (button != CANCEL));

    /*
    bevent_t event;
    do {
        input.getEvent(&event, true);
    } while ((event.state != PRESSED) ||
                (  (event.button != SETTINGS)
                && (event.button != ACCEPT)
                && (event.button != CLEAR)));
                */
}

void GMenu2X::scanPath(string path, vector<string> *files) {
	DIR *dirp;
	struct stat st;
	struct dirent *dptr;
	string filepath, ext;

	if (path[path.length()-1]!='/') path += "/";
	if ((dirp = opendir(path.c_str())) == NULL) return;

	while ((dptr = readdir(dirp))) {
		if (dptr->d_name[0]=='.')
			continue;
		filepath = path+dptr->d_name;
		int statRet = stat(filepath.c_str(), &st);
		if (S_ISDIR(st.st_mode))
			scanPath(filepath, files);
		if (statRet != -1) {
			ext = filepath.substr(filepath.length()-4,4);
#ifdef PLATFORM_GP2X
			if (ext==".gpu")
#elif defined(PLATFORM_DINGUX) || defined(PLATFORM_NANONOTE)
			if (ext==".dge")
#else
			if (ext==".pxml")
#endif
				files->push_back(filepath);
		}
	}

	closedir(dirp);
}

unsigned short GMenu2X::getBatteryLevel() {
#ifdef PLATFORM_GP2X
/*	if (batteryHandle<=0) return 0;

	if (f200) {
		MMSP2ADC val;
		read(batteryHandle, &val, sizeof(MMSP2ADC));

		if (val.batt==0) return 5;
		if (val.batt==1) return 3;
		if (val.batt==2) return 1;
		if (val.batt==3) return 0;
	} else {
		int battval = 0;
		unsigned short cbv, min=900, max=0;

		for (int i = 0; i < BATTERY_READS; i ++) {
			if ( read(batteryHandle, &cbv, 2) == 2) {
				battval += cbv;
				if (cbv>max) max = cbv;
				if (cbv<min) min = cbv;
			}
		}

		battval -= min+max;
		battval /= BATTERY_READS-2;

		if (battval>=850) return 6;
		if (battval>780) return 5;
		if (battval>740) return 4;
		if (battval>700) return 3;
		if (battval>690) return 2;
		if (battval>680) return 1;
	}*/

#else
	if (!batteryHandle) return 0;
	int battval = 0;
	char battvalcstr[5];
	fscanf(batteryHandle, "%s", &battvalcstr[0]);
	rewind(batteryHandle);
	battval = atoi(battvalcstr);
	if (battval>90) return 5;
	if (battval>70) return 4;
	if (battval>50) return 3;
	if (battval>30) return 2;
	if (battval>10) return 1;

	if (!usbHandle) return 0;
	int usbval = 0;
	char usbvalcstr[5];
	fscanf(usbHandle, "%s", &usbvalcstr[0]);
	rewind(usbHandle);
	usbval = atoi(usbvalcstr);
	if (usbval==1) return 6;

	return 0;
//#else
//	return 6; //AC Power
#endif
}

void GMenu2X::setInputSpeed() {
    /*
	input.setInterval(150);
	input.setInterval(30,  VOLDOWN);
	input.setInterval(30,  VOLUP  );
	input.setInterval(30,  CANCEL      );
	input.setInterval(500, SETTINGS  );
	input.setInterval(500, MENU );
	input.setInterval(300, CLEAR      );
	input.setInterval(300,  MANUAL      );
	input.setInterval(1000,ACCEPT      );
	//joy.setInterval(1000,ACTION_CLICK  );
	input.setInterval(300, ALTLEFT      );
	input.setInterval(300, ALTRIGHT      );
    */
	SDL_EnableKeyRepeat(1,150);
}

void GMenu2X::applyRamTimings() {
#ifdef PLATFORM_GP2X
	// 6 4 1 1 1 2 2
/*	if (gp2x_mem!=0) {
		int tRC = 5, tRAS = 3, tWR = 0, tMRD = 0, tRFC = 0, tRP = 1, tRCD = 1;
		gp2x_memregs[0x3802>>1] = ((tMRD & 0xF) << 12) | ((tRFC & 0xF) << 8) | ((tRP & 0xF) << 4) | (tRCD & 0xF);
		gp2x_memregs[0x3804>>1] = ((tRC & 0xF) << 8) | ((tRAS & 0xF) << 4) | (tWR & 0xF);
	}*/
#endif
}

void GMenu2X::applyDefaultTimings() {
#ifdef PLATFORM_GP2X
	// 8 16 3 8 8 8 8
/*	if (gp2x_mem!=0) {
		int tRC = 7, tRAS = 15, tWR = 2, tMRD = 7, tRFC = 7, tRP = 7, tRCD = 7;
		gp2x_memregs[0x3802>>1] = ((tMRD & 0xF) << 12) | ((tRFC & 0xF) << 8) | ((tRP & 0xF) << 4) | (tRCD & 0xF);
		gp2x_memregs[0x3804>>1] = ((tRC & 0xF) << 8) | ((tRAS & 0xF) << 4) | (tWR & 0xF);
	}*/
#endif
}



void GMenu2X::setClock(unsigned mhz) {
	mhz = constrain(mhz, 30, confInt["maxClock"]);
#if defined(PLATFORM_DINGUX) || defined(PLATFORM_NANONOTE)
	jz_cpuspeed(mhz);
#endif
}

void GMenu2X::setGamma(int /*gamma*/) {
#ifdef PLATFORM_GP2X
/*	float fgamma = (float)constrain(gamma,1,100)/10;
	fgamma = 1 / fgamma;
	MEM_REG[0x2880>>1]&=~(1<<12);
	MEM_REG[0x295C>>1]=0;

	for (int i=0; i<256; i++) {
		unsigned char g = (unsigned char)(255.0*pow(i/255.0,fgamma));
		unsigned short s = (g<<8) | g;
		MEM_REG[0x295E>>1]= s;
		MEM_REG[0x295E>>1]= g;
	}*/
#endif
}

int GMenu2X::getVolume() {
	unsigned long mixer;
    	int basevolume = -1;
	mixer = open("/dev/mixer", O_RDONLY);
	if(mixer)
	{
		if (ioctl(mixer, SOUND_MIXER_READ_VOLUME, &basevolume) == -1) {
			ERROR("Failed opening mixer for read - VOLUME\n");
		}
		close(mixer);
		if(basevolume != -1)
			return  (basevolume>>8) & basevolume ;
	}
	return basevolume;
}

void GMenu2X::setVolume(int vol) {
	unsigned long mixer;
	int newvolume = vol;
	int oss_volume = newvolume | (newvolume << 8); // set volume for both channels
	mixer = open("/dev/mixer", O_WRONLY);
	if(mixer)
	{
		if (ioctl(mixer, SOUND_MIXER_WRITE_VOLUME, &oss_volume) == -1) {
			ERROR("Failed opening mixer for write - VOLUME\n");
		}
		close(mixer);
	}

}

void GMenu2X::setVolumeScaler(int scale) {
	scale = constrain(scale,0,MAX_VOLUME_SCALE_FACTOR);
	unsigned long soundDev = open("/dev/mixer", O_WRONLY);
	if (soundDev) {
		ioctl(soundDev, SOUND_MIXER_PRIVATE2, &scale);
		close(soundDev);
	}
}

int GMenu2X::getVolumeScaler() {
	int currentscalefactor = -1;
	unsigned long soundDev = open("/dev/mixer", O_RDONLY);
	if (soundDev) {
		ioctl(soundDev, SOUND_MIXER_PRIVATE1, &currentscalefactor);
		close(soundDev);
	}
	return currentscalefactor;
}

const string &GMenu2X::getExePath() {
	if (path.empty()) {
		char buf[255];
		memset(buf, 0, 255);
		int l = readlink("/proc/self/exe", buf, 255);

		path = buf;
		path = path.substr(0,l);
		l = path.rfind("/");
		path = path.substr(0,l+1);
	}
	return path;
}

string GMenu2X::getDiskFree(const char *path) {
	stringstream ss;
	string df = "";
	struct statvfs b;

	int ret = statvfs(path, &b);
	if (ret==0) {
		// Make sure that the multiplication happens in 64 bits.
		unsigned long long free =
			((unsigned long long)b.f_bfree * b.f_bsize) / 1048576;
		unsigned long long total =
			((unsigned long long)b.f_blocks * b.f_frsize) / 1048576;
		ss << free << "/" << total << "MB";
		ss >> df;
	} else WARNING("statvfs failed with error '%s'.\n", strerror(errno));
	return df;
}

int GMenu2X::drawButton(Button *btn, int x, int y) {
	if (y<0) y = resY+y;
	btn->setPosition(x, y-7);
	btn->paint();
	return x+btn->getRect().w+6;
}

int GMenu2X::drawButton(Surface *s, const string &btn, const string &text, int x, int y) {
	if (y<0) y = resY+y;
	SDL_Rect re = {x, y-7, 0, 16};
	if (sc.skinRes("imgs/buttons/"+btn+".png") != NULL) {
		sc["imgs/buttons/"+btn+".png"]->blit(s, x, y-7);
		re.w = sc["imgs/buttons/"+btn+".png"]->width() + 3;
		s->write(font, text, x+re.w, y, ASFont::HAlignLeft, ASFont::VAlignMiddle);
		re.w += font->getTextWidth(text);
	}
	return x+re.w+6;
}

int GMenu2X::drawButtonRight(Surface *s, const string &btn, const string &text, int x, int y) {
	if (y<0) y = resY+y;
	if (sc.skinRes("imgs/buttons/"+btn+".png") != NULL) {
		x -= 16;
		sc["imgs/buttons/"+btn+".png"]->blit(s, x, y-7);
		x -= 3;
		s->write(font, text, x, y, ASFont::HAlignRight, ASFont::VAlignMiddle);
		return x-6-font->getTextWidth(text);
	}
	return x-6;
}

void GMenu2X::drawScrollBar(uint pagesize, uint totalsize, uint pagepos, uint top, uint height) {
	if (totalsize<=pagesize) return;

	s->rectangle(resX-8, top, 7, height, skinConfColors[COLOR_SELECTION_BG]);

	//internal bar total height = height-2
	//bar size
	uint bs = (height-2) * pagesize / totalsize;
	//bar y position
	uint by = (height-2) * pagepos / totalsize;
	by = top+2+by;
	if (by+bs>top+height-2) by = top+height-2-bs;


	s->box(resX-6, by, 3, bs, skinConfColors[COLOR_SELECTION_BG]);
}

void GMenu2X::drawTopBar(Surface *s) {
	if (s==NULL) s = this->s;

	Surface *bar = sc.skinRes("imgs/topbar.png", false);
	if (bar != NULL)
		bar->blit(s, 0, 0);
	else
		s->box(0, 0, resX, skinConfInt["topBarHeight"],
		skinConfColors[COLOR_TOP_BAR_BG]);
}

void GMenu2X::drawBottomBar(Surface *s) {
	if (s==NULL) s = this->s;

	Surface *bar = sc.skinRes("imgs/bottombar.png", false);
	if (bar != NULL)
		bar->blit(s, 0, resY-bar->height());
	else
		s->box(0, resY-20, resX, 20, skinConfColors[COLOR_BOTTOM_BAR_BG]);
}

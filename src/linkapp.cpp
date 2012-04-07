/***************************************************************************
 *   Copyright (C) 2006 by Massimiliano Torromeo                           *
 *   massimiliano.torromeo@gmail.com                                       *
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

#include "linkapp.h"

#include "debug.h"
#include "gmenu2x.h"
#include "menu.h"
#include "selector.h"
#include "textmanualdialog.h"
#include "utilities.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <signal.h>
#include <unistd.h>

#include <fstream>
#include <sstream>

using namespace std;

LinkApp::LinkApp(GMenu2X *gmenu2x_, Touchscreen &ts, InputManager &inputMgr_,
				 const char* linkfile)
	: Link(gmenu2x_, ts)
	, inputMgr(inputMgr_)
{
	manual = "";
	file = linkfile;
	wrapper = false;
	dontleave = false;
	setClock(312);
	selectordir = "";
	selectorfilter = "";
	icon = iconPath = "";
	selectorbrowser = false;
	useRamTimings = false;

	string line;
	ifstream infile (linkfile, ios_base::in);
	while (getline(infile, line, '\n')) {
		line = trim(line);
		if (line=="") continue;
		if (line[0]=='#') continue;

		string::size_type position = line.find("=");
		string name = trim(line.substr(0,position));
		string value = trim(line.substr(position+1));
		if (name == "title") {
			title = value;
		} else if (name == "description") {
			description = value;
		} else if (name == "icon") {
			setIcon(value);
		} else if (name == "exec") {
			exec = value;
		} else if (name == "params") {
			params = value;
		} else if (name == "manual") {
			manual = value;
		} else if (name == "wrapper") {
			if (value=="true") wrapper = true;
		} else if (name == "dontleave") {
			if (value=="true") dontleave = true;
		} else if (name == "clock") {
			setClock( atoi(value.c_str()) );
		} else if (name == "selectordir") {
			setSelectorDir( value );
		} else if (name == "selectorbrowser") {
			if (value=="true") selectorbrowser = true;
		} else if (name == "useramtimings") {
			if (value=="true") useRamTimings = true;
		} else if (name == "selectorfilter") {
			setSelectorFilter( value );
		} else if (name == "selectorscreens") {
			setSelectorScreens( value );
		} else if (name == "selectoraliases") {
			setAliasFile( value );
		} else {
			WARNING("Unrecognized option: '%s'\n", name.c_str());
			break;
		}
	}
	infile.close();

	if (iconPath.empty()) searchIcon();

	edited = false;
}

const string &LinkApp::searchIcon() {
	string execicon = exec;
	string::size_type pos = exec.rfind(".");
	if (pos != string::npos) execicon = exec.substr(0,pos);
	execicon += ".png";
	string exectitle = execicon;
	pos = execicon.rfind("/");
	if (pos != string::npos)
		string exectitle = execicon.substr(pos+1,execicon.length());

	if (!gmenu2x->sc.getSkinFilePath("icons/"+exectitle).empty())
		iconPath = gmenu2x->sc.getSkinFilePath("icons/"+exectitle);
	else if (fileExists(execicon))
		iconPath = execicon;
	else
		iconPath = gmenu2x->sc.getSkinFilePath("icons/generic.png");

	return iconPath;
}

int LinkApp::clock() {
	return iclock;
}

const string &LinkApp::clockStr(int maxClock) {
	if (iclock>maxClock) setClock(maxClock);
	return sclock;
}

void LinkApp::setClock(int mhz) {
	iclock = mhz;
	stringstream ss;
	sclock = "";
	ss << iclock << "MHz";
	ss >> sclock;

	edited = true;
}

bool LinkApp::targetExists()
{
	return fileExists(exec);
}

bool LinkApp::save() {
	if (!edited) return false;

	ofstream f(file.c_str());
	if (f.is_open()) {
		if (title!=""          ) f << "title="           << title           << endl;
		if (description!=""    ) f << "description="     << description     << endl;
		if (icon!=""           ) f << "icon="            << icon            << endl;
		if (exec!=""           ) f << "exec="            << exec            << endl;
		if (params!=""         ) f << "params="          << params          << endl;
		if (manual!=""         ) f << "manual="          << manual          << endl;
		if (iclock!=0          ) f << "clock="           << iclock          << endl;
		if (useRamTimings      ) f << "useramtimings=true"                  << endl;
		if (selectordir!=""    ) f << "selectordir="     << selectordir     << endl;
		if (selectorbrowser    ) f << "selectorbrowser=true"                << endl;
		if (selectorfilter!="" ) f << "selectorfilter="  << selectorfilter  << endl;
		if (selectorscreens!="") f << "selectorscreens=" << selectorscreens << endl;
		if (aliasfile!=""      ) f << "selectoraliases=" << aliasfile       << endl;
		if (wrapper            ) f << "wrapper=true"                        << endl;
		if (dontleave          ) f << "dontleave=true"                      << endl;
		f.close();
		sync();
		return true;
	} else
		ERROR("Error while opening the file '%s' for write.\n", file.c_str());
	return false;
}

void LinkApp::drawRun() {
	//Darkened background
	gmenu2x->s->box(0, 0, gmenu2x->resX, gmenu2x->resY, 0,0,0,150);

	string text = gmenu2x->tr.translate("Launching $1",getTitle().c_str(),NULL);
	int textW = gmenu2x->font->getTextWidth(text);
	int boxW = 62+textW;
	int halfBoxW = boxW/2;

	//outer box
	gmenu2x->s->box(gmenu2x->halfX-2-halfBoxW, gmenu2x->halfY-23, halfBoxW*2+5, 47, gmenu2x->skinConfColors[COLOR_MESSAGE_BOX_BG]);
	//inner rectangle
	gmenu2x->s->rectangle(gmenu2x->halfX-halfBoxW, gmenu2x->halfY-21, boxW, 42, gmenu2x->skinConfColors[COLOR_MESSAGE_BOX_BORDER]);

	int x = gmenu2x->halfX+10-halfBoxW;
	/*if (getIcon()!="")
		gmenu2x->sc[getIcon()]->blit(gmenu2x->s,x,104);
	else
		gmenu2x->sc["icons/generic.png"]->blit(gmenu2x->s,x,104);*/
	gmenu2x->sc[getIconPath()]->blit(gmenu2x->s,x,gmenu2x->halfY-16);
	gmenu2x->s->write( gmenu2x->font, text, x+42, gmenu2x->halfY+1, ASFont::HAlignLeft, ASFont::VAlignMiddle );
	gmenu2x->s->flip();
}

void LinkApp::run() {
	if (selectordir!="")
		selector();
	else
		launch();
}

void LinkApp::showManual() {
	if (manual=="" || !fileExists(manual)) return;

	// Png manuals
	string ext8 = manual.substr(manual.size()-8,8);
	if (ext8==".man.png" || ext8==".man.bmp" || ext8==".man.jpg" || manual.substr(manual.size()-9,9)==".man.jpeg") {
		//Raise the clock to speed-up the loading of the manual
		gmenu2x->setClock(312);

		Surface *pngman = Surface::loadImage(manual);
		if (!pngman) {
			return;
		}
		Surface *bg = Surface::loadImage(gmenu2x->confStr["wallpaper"]);
		if (!bg) {
			bg = Surface::emptySurface(gmenu2x->s->width(), gmenu2x->s->height());
		}
		bg->convertToDisplayFormat();

		stringstream ss;
		string pageStatus;

		bool close = false, repaint = true;
		int page = 0, pagecount = pngman->width() / 320;

		ss << pagecount;
		string spagecount;
		ss >> spagecount;

		//Lower the clock
		gmenu2x->setClock(gmenu2x->confInt["menuClock"]);

		while (!close) {
			if (repaint) {
				bg->blit(gmenu2x->s, 0, 0);
				pngman->blit(gmenu2x->s, -page*320, 0);

				gmenu2x->drawBottomBar();
				gmenu2x->drawButton(gmenu2x->s, "start", gmenu2x->tr["Exit"],
				gmenu2x->drawButton(gmenu2x->s, "cancel", "",
				gmenu2x->drawButton(gmenu2x->s, "right", gmenu2x->tr["Change page"],
				gmenu2x->drawButton(gmenu2x->s, "left", "", 5)-10))-10);

				ss.clear();
				ss << page+1;
				ss >> pageStatus;
				pageStatus = gmenu2x->tr["Page"]+": "+pageStatus+"/"+spagecount;
				gmenu2x->s->write(gmenu2x->font, pageStatus, 310, 230, ASFont::HAlignRight, ASFont::VAlignMiddle);

				gmenu2x->s->flip();
				repaint = false;
			}

            switch(inputMgr.waitForPressedButton()) {
				case InputManager::SETTINGS:
                case InputManager::CANCEL:
                    close = true;
                    break;
                case InputManager::LEFT:
                    if (page > 0) {
                        page--;
                        repaint = true;
                    }
                    break;
                case InputManager::RIGHT:
                    if (page < pagecount-1) {
                        page++;
                        repaint=true;
                    }
                    break;
                default:
                    break;
            }
        }
		delete bg;
		return;
	}

	// Txt manuals
	if (manual.substr(manual.size()-8,8)==".man.txt") {
		vector<string> txtman;

		string line;
		ifstream infile(manual.c_str(), ios_base::in);
		if (infile.is_open()) {
			gmenu2x->setClock(312);
			while (getline(infile, line, '\n')) txtman.push_back(line);
			infile.close();

			TextManualDialog tmd(gmenu2x, getTitle(), getIconPath(), &txtman);
			gmenu2x->setClock(gmenu2x->confInt["menuClock"]);
			tmd.exec();
		}

		return;
	}

	//Readmes
	vector<string> readme;

	string line;
	ifstream infile(manual.c_str(), ios_base::in);
	if (infile.is_open()) {
		gmenu2x->setClock(312);
		while (getline(infile, line, '\n')) readme.push_back(line);
		infile.close();

		TextDialog td(gmenu2x, getTitle(), "ReadMe", getIconPath(), &readme);
		gmenu2x->setClock(gmenu2x->confInt["menuClock"]);
		td.exec();
	}
}

void LinkApp::selector(int startSelection, const string &selectorDir) {
	//Run selector interface
	Selector sel(gmenu2x, this, selectorDir);
	int selection = sel.exec(startSelection);
	if (selection!=-1) {
		gmenu2x->writeTmp(selection, sel.getDir());
		launch(sel.getFile(), sel.getDir());
	}
}

void LinkApp::launch(const string &selectedFile, const string &selectedDir) {
	drawRun();
	save();

	//Set correct working directory
	string::size_type pos = exec.rfind("/");
	if (pos != string::npos) {
		string wd = exec.substr(0, pos + 1);
		chdir(wd.c_str());
	}

	//selectedFile
	if (selectedFile!="") {
		string selectedFileExtension;
		string selectedFileName;
		string dir;
		string::size_type i = selectedFile.rfind(".");
		if (i != string::npos) {
			selectedFileExtension = selectedFile.substr(i,selectedFile.length());
			selectedFileName = selectedFile.substr(0,i);
		}

		if (selectedDir=="")
			dir = getSelectorDir();
		else
			dir = selectedDir;
		if (params=="") {
			params = cmdclean(dir+selectedFile);
		} else {
			string origParams = params;
			params = strreplace(params,"[selFullPath]",cmdclean(dir+selectedFile));
			params = strreplace(params,"[selPath]",cmdclean(dir));
			params = strreplace(params,"[selFile]",cmdclean(selectedFileName));
			params = strreplace(params,"[selExt]",cmdclean(selectedFileExtension));
			if (params == origParams) params += " " + cmdclean(dir+selectedFile);
		}
	}

	if (useRamTimings)
		gmenu2x->applyRamTimings();

	INFO("Executing '%s' (%s %s)\n", title.c_str(), exec.c_str(), params.c_str());

	//check if we have to quit
	string command = cmdclean(exec);

	// Check to see if permissions are desirable
	struct stat fstat;
	if( stat( command.c_str(), &fstat ) == 0 ) {
		struct stat newstat = fstat;
		if( S_IRUSR != ( fstat.st_mode & S_IRUSR ) )
			newstat.st_mode |= S_IRUSR;
		if( S_IXUSR != ( fstat.st_mode & S_IXUSR ) )
			newstat.st_mode |= S_IXUSR;
		if( fstat.st_mode != newstat.st_mode )
			chmod( command.c_str(), newstat.st_mode );
	} // else, well.. we are no worse off :)

	if (params!="") command += " " + params;
	if (gmenu2x->confInt["outputLogs"]) command += " &> " + cmdclean(gmenu2x->getHome()) + "/log.txt";
	if (wrapper) command += "; sync & cd "+cmdclean(gmenu2x->getExePath())+"; exec ./gmenu2x";
	if (dontleave) {
		system(command.c_str());
	} else {
		if (gmenu2x->confInt["saveSelection"] && (
				gmenu2x->confInt["section"]!=gmenu2x->menu->selSectionIndex()
				|| gmenu2x->confInt["link"]!=gmenu2x->menu->selLinkIndex()
		)) {
			gmenu2x->writeConfig();
		}

#ifdef PLATFORM_GP2X
		if (gmenu2x->fwType == "open2x"
				&& gmenu2x->savedVolumeMode != gmenu2x->volumeMode) {
			gmenu2x->writeConfigOpen2x();
		}
#endif

		if (selectedFile == "") {
			gmenu2x->writeTmp();
		}
		if (clock() != gmenu2x->confInt["menuClock"]) {
			gmenu2x->setClock(clock());
		}
		gmenu2x->quit();

		/* Make the terminal we're connected to (via stdin/stdout) our
		   controlling terminal again.  Else many console programs are
		   not going to work correctly.  Actually this would not be
		   necessary, if SDL correctly restored terminal state after
		   SDL_Quit(). */
		pid_t pid = setsid();
		if (pid == (pid_t)-1) {
			WARNING("Failed to create new process group\n");
		}
		ioctl(1, TIOCSCTTY, STDOUT_FILENO);

		int pgid = tcgetpgrp(STDOUT_FILENO);
		signal(SIGTTOU, SIG_IGN);
		tcsetpgrp(STDOUT_FILENO, pgid);
		gmenu2x->writePID();
		execlp("/bin/sh","/bin/sh", "-c", command.c_str(), NULL);
		//if execution continues then something went wrong and as we already called SDL_Quit we cannot continue
		//try relaunching gmenu2x
		chdir(gmenu2x->getExePath().c_str());
		execlp("./gmenu2x", "./gmenu2x", NULL);
	}

	chdir(gmenu2x->getExePath().c_str());
}

const string &LinkApp::getExec() {
	return exec;
}

void LinkApp::setExec(const string &exec) {
	this->exec = exec;
	edited = true;
}

const string &LinkApp::getParams() {
	return params;
}

void LinkApp::setParams(const string &params) {
	this->params = params;
	edited = true;
}

const string &LinkApp::getManual() {
	return manual;
}

void LinkApp::setManual(const string &manual) {
	this->manual = manual;
	edited = true;
}

const string &LinkApp::getSelectorDir() {
	return selectordir;
}

void LinkApp::setSelectorDir(const string &selectordir) {
	this->selectordir = selectordir;
	if (this->selectordir!="" && this->selectordir[this->selectordir.length()-1]!='/') this->selectordir += "/";
	edited = true;
}

bool LinkApp::getSelectorBrowser() {
	return selectorbrowser;
}

void LinkApp::setSelectorBrowser(bool value) {
	selectorbrowser = value;
	edited = true;
}

bool LinkApp::getUseRamTimings() {
	return useRamTimings;
}

void LinkApp::setUseRamTimings(bool value) {
	useRamTimings = value;
	edited = true;
}

const string &LinkApp::getSelectorFilter() {
	return selectorfilter;
}

void LinkApp::setSelectorFilter(const string &selectorfilter) {
	this->selectorfilter = selectorfilter;
	edited = true;
}

const string &LinkApp::getSelectorScreens() {
	return selectorscreens;
}

void LinkApp::setSelectorScreens(const string &selectorscreens) {
	this->selectorscreens = selectorscreens;
	edited = true;
}

const string &LinkApp::getAliasFile() {
	return aliasfile;
}

void LinkApp::setAliasFile(const string &aliasfile) {
	if (fileExists(aliasfile)) {
		this->aliasfile = aliasfile;
		edited = true;
	}
}

void LinkApp::renameFile(const string &name) {
	file = name;
}

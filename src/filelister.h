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

#ifndef FILELISTER_H
#define FILELISTER_H

#include <string>
#include <vector>

class FileLister {
private:
	std::string path, filter;
	bool showDirectories, showFiles;

	std::vector<std::string> directories, files, excludes;

public:
	FileLister(const std::string &startPath = "/boot/local", bool showDirectories = true, bool showFiles = true);
	void browse(bool clean = true);

	unsigned int size();
	unsigned int dirCount();
	unsigned int fileCount();
	std::string operator[](unsigned int);
	std::string at(unsigned int);
	bool isFile(unsigned int);
	bool isDirectory(unsigned int);

	const std::string &getPath();
	void setPath(const std::string &path, bool doBrowse=true);
	const std::string &getFilter();
	void setFilter(const std::string &filter);

	const std::vector<std::string> &getDirectories() { return directories; }
	const std::vector<std::string> &getFiles() { return files; }
	void insertFile(const std::string &file);
	void addExclude(const std::string &exclude);
};

#endif // FILELISTER_H

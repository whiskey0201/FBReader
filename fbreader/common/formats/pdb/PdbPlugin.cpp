/*
 * FBReader -- electronic book reader
 * Copyright (C) 2004, 2005 Nikolay Pultsin <geometer@mawhrin.net>
 * Copyright (C) 2005 Mikhail Sobolev <mss@mawhrin.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 */

#include <abstract/ZLFSManager.h>
#include <abstract/ZLInputStream.h>

#include "PdbPlugin.h"
#include "PdbReader.h"
//#include "PdbDescriptionReader.h"
//#include "PdbBookReader.h"
#include "../../description/BookDescription.h"

bool PdbPlugin::acceptsFile(const std::string &extension) const {
	return (extension == "pdb") || (extension == "PDB");
}

bool PdbPlugin::readDescription(const std::string &path, BookDescription &description) const {
	//return PdbReader().readDocument(ZLFile(path).inputStream());
	ZLFile file(path);
	WritableBookDescription wDescription(description);
	wDescription.encoding() = "US_ASCII";
	wDescription.addAuthor("Unknown", "PDB", "Author");
	wDescription.title() = file.name();
	return true;
}

bool PdbPlugin::readModel(const BookDescription &description, BookModel &model) const {
	return PdbReader().readDocument(description.fileName(), model);
}

const std::string &PdbPlugin::iconName() const {
	static const std::string ICON_NAME = "FBReader/pdb";
	return ICON_NAME;
}

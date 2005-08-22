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

#include <abstract/ZLStringUtil.h>
#include <abstract/ZLFSManager.h>

#include "FBFileHandler.h"
#include "../formats/FormatPlugin.h"

bool FBFileHandler::isNodeVisible(const ZLTreeNodePtr node) const {
	const std::string &name = node->name();
	if (name.length() == 0) {
		return false;
	}
	if (name[0] == '.') {
		return !node->isFile() && (name == "..");
	}
	ZLFile file(name);
	return !node->isFile() || file.isArchive() || (PluginCollection::instance().plugin(file.extension(), false) != 0);
}

const std::string &FBFileHandler::pixmapName(const ZLTreeNodePtr node) const {
	static const std::string FOLDER_ICON = "FBReader/folder";
	static const std::string ZIPFOLDER_ICON = "FBReader/zipfolder";
	static const std::string UNKNOWN_ICON = "FBReader/unknown";
	ZLFile file(node->name());
	if (!node->isFile()) {
		return FOLDER_ICON;
	} else if (file.isArchive()) {
		return ZIPFOLDER_ICON;
	} else {
		FormatPlugin *plugin = PluginCollection::instance().plugin(file.extension(), false);
		if (plugin != 0) {
			return plugin->iconName();
		}
	}
	return UNKNOWN_ICON;
}

void FBFileHandler::accept(const ZLTreeStatePtr state) const {
	myDescription = BookDescription::create(state->name());
}

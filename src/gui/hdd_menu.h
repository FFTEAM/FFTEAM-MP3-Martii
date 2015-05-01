/*
	Neutrino-GUI  -   DBoxII-Project


	Copyright (C) 2009-2014 CoolStream International Ltd
	Copyright (C) 2013-2014 martii

	License: GPLv2

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; version 2 of the License.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/


#ifndef __hdd_menu__
#define __hdd_menu__


#include "widget/menue.h"
#if HAVE_SPARK_HARDWARE || HAVE_DUCKBOX_HARDWARE
#include <string>
#include <vector>
#endif

using namespace std;

enum {
	fs_ext3,
	fs_ext4,
	fs_ext2,
	fs_jfs,

	fs_max
};

class CHDDDestExec : public CMenuTarget
{
public:
        int exec(CMenuTarget* parent, const std::string&);
};
class CHDDFmtExec : public CMenuTarget
{
public:
        int exec(CMenuTarget* parent, const std::string&);
};
class CHDDChkExec : public CMenuTarget
{
public:
        int exec(CMenuTarget* parent, const std::string&);
};

class CHDDMenuHandler : public CMenuTarget
{
	private:
		int width;
#if HAVE_SPARK_HARDWARE || HAVE_DUCKBOX_HARDWARE
		struct hdd_s {
			std::string devname;
			std::string name;
			const char *label;
			const char *type;
			std::string mountpoint;
			CMenuForwarder *cmf;
			bool mounted;
		};
		std::vector<hdd_s> hdd_list;
#endif
		void showError(neutrino_locale_t err);
	public:
		CHDDMenuHandler();
		~CHDDMenuHandler();

		static CHDDMenuHandler* getInstance();
		int exec( CMenuTarget* parent,  const std::string &actionkey);
		int doMenu();
};

#endif

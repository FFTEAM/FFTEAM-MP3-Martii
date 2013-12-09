/*
	WebTV Setup

	(C) 2012-2013 by martii


	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#define __USE_FILE_OFFSET64 1
#include "filebrowser.h"
#include <stdio.h>
#include <global.h>
#include <libgen.h>
#include <neutrino.h>
#include <driver/screen_max.h>
#include <driver/framebuffer.h>
#include <gui/widget/hintbox.h>
#include "webtv_setup.h"

CWebTVSetup::CWebTVSetup()
{
	width = w_max (40, 10);
	selected = -1;
	item_offset = 0;
	changed = false;
}

#define CWebTVSetupFooterButtonCount 1
static const struct button_label CWebTVSetupFooterButtons[CWebTVSetupFooterButtonCount] = {
	{ NEUTRINO_ICON_BUTTON_RED, LOCALE_WEBTV_XML_DEL }
};

int CWebTVSetup::exec(CMenuTarget* parent, const std::string & actionKey)
{
	int res = menu_return::RETURN_REPAINT;

	if(actionKey == "d" /* delete */) {
		selected = m->getSelected();
		if (selected >= item_offset) {
			m->removeItem(selected);
			if (item_offset == m->getItemsCount()) {
				m->removeItem(selected - 1);
			}
		    m->hide();
			selected = m->getSelected();
			changed = true;
		}
		return res;
	}
	if(actionKey == "c" /* change */) {
		CFileBrowser fileBrowser;
		CFileFilter fileFilter;
		fileFilter.addFilter("xml");
		fileBrowser.Filter = &fileFilter;
		selected = m->getSelected();
		CMenuItem* item = m->getItem(selected);
		CMenuForwarder *f = reinterpret_cast<CMenuForwarder*>(item);
		std::string dirname(f->getName());
		if (fileBrowser.exec(dirname.substr(0, dirname.rfind('/')).c_str())) {
			f->setName(fileBrowser.getSelectedFile()->Name);
			changed = true;
		}
		return res;
	}
	if(actionKey == "a" /* add */) {
		CFileBrowser fileBrowser;
		CFileFilter fileFilter;
		fileFilter.addFilter("xml");
		fileBrowser.Filter = &fileFilter;
		if (fileBrowser.exec("/") == true) {
			std::string s = fileBrowser.getSelectedFile()->Name;
			if (item_offset == m->getItemsCount() + 1)
				m->addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_WEBTV_XML));
			m->addItem(new CMenuForwarder(s, true, NULL, this, "c"));
			changed = true;
		}
		return res;
	}

    if(parent)
		parent->hide();

	Show();

	return res;
}

void CWebTVSetup::Show()
{
	item_offset = 0;

	m = new CMenuWidget(LOCALE_WEBTV_HEAD, NEUTRINO_ICON_MOVIEPLAYER, width);
	m->addKey(CRCInput::RC_spkr, this, "d");
	m->addKey(CRCInput::RC_red, this, "d");
	m->setSelected(selected);
	m->addIntroItems(LOCALE_EPGPLUS_OPTIONS);
	m->addItem(new CMenuForwarder(LOCALE_WEBTV_XML_ADD, true, NULL, this, "a", CRCInput::RC_green, NEUTRINO_ICON_BUTTON_GREEN));
	for (std::list<std::string>::iterator it = g_settings.webtv_xml.begin(); it != g_settings.webtv_xml.end(); ++it) {
		if (item_offset == 0) {
			m->addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_WEBTV_XML));
			item_offset = m->getItemsCount();
		}
		m->addItem(new CMenuForwarder(*it, true, NULL, this, "c"));
	}
	m->setFooter(CWebTVSetupFooterButtons, CWebTVSetupFooterButtonCount);
	m->exec(NULL, "");
	m->hide();
	if (changed) {
			g_settings.webtv_xml.clear();
			for (int i = item_offset; i < m->getItemsCount(); i++) {
				CMenuItem *item = m->getItem(i);
				CMenuForwarder *f = reinterpret_cast<CMenuForwarder*>(item);
				g_settings.webtv_xml.push_back(f->getName());
			}
			g_Zapit->reinitChannels();
			changed = false;
	}
	selected = m->getSelected();
	delete m;
}
// vim:ts=4

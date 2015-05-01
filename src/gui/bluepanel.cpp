/*
	FFTEAMBluePanel - Neutrino-GUI

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <dirent.h>
#include "bluepanel.h"
#include "network_setup.h"
#include <gui/widget/icons.h>
#include <gui/widget/stringinput.h>
#include <gui/widget/stringinput_ext.h>
#include <gui/widget/hintbox.h>
#include <gui/widget/messagebox.h>
#include <cs_api.h>
#include <global.h>
#include <neutrino.h>
#include <mymenu.h>
#include <neutrino_menue.h>
#include <driver/screen_max.h>
#include <system/debug.h>
#include <fstream>
#include <zapit/zapit.h>

// Strings
std::string on = "An";
std::string off = "Aus";
char none_cam[] = "keine";

CBluePanel::CBluePanel()
{
	width = 40;
}

CBluePanel::~CBluePanel()
{
	//leer
}

int CBluePanel::exec(CMenuTarget* parent, const std::string &actionKey)
{
	dprintf(DEBUG_DEBUG, "Init: BluePanel v1.0 by rgmviper+thomas\n");

	int  res = menu_return::RETURN_EXIT_REPAINT;
		
	if (parent)
		parent->hide();

	printf("CBluePanel::exec: %s\n", actionKey.c_str());

	if (actionKey == "save")
	{
		CHintBox * hintBox = new CHintBox(LOCALE_MESSAGEBOX_INFO, "Einstellungen werden gespeichert..."); // UTF-8
		hintBox->paint();
		SaveConfig();
		system("sleep 2");
		hintBox->hide();
		delete hintBox;
		return res;
	}

	res = showBluePanel();
	return res;
}

void CBluePanel::LoadConfig()
{
			
	// Openvpn
	openvpn = off;
	openvpn_is = false;
	std::ifstream FileTest_Openvpn("/etc/init.d/openvpn"); 
	if(FileTest_Openvpn)
		openvpn_is = true;
	std::ifstream FileTest_Openvpn_Autostart("/etc/init.d/S80openvpn"); 
	if(FileTest_Openvpn_Autostart && FileTest_Openvpn)
		openvpn = on;
	
	// Inadyn
	inadyn = off;
	inadyn_is = false;
	std::ifstream FileTest_Inadyn("/etc/init.d/inadyn"); 
	if(FileTest_Inadyn)
		inadyn_is = true;
	std::ifstream FileTest_Inadyn_Autostart("/etc/init.d/S85inadyn"); 
	if(FileTest_Inadyn_Autostart && FileTest_Inadyn)
		inadyn = on;
		
}

void CBluePanel::SaveConfig()
{
	// OpenVPN
	std::ifstream FileTest_Openvpn("/etc/init.d/S80openvpn"); 
	if((FileTest_Openvpn) && (openvpn == off)) 
		system("rm /etc/init.d/S80openvpn");
		system("/etc/init.d/openvpn stop");
	if((!FileTest_Openvpn) && (openvpn == on)) 
		system("ln -s /etc/init.d/openvpn /etc/init.d/S80openvpn");
		system("/etc/init.d/openvpn start");
		
	// Inadyn
	std::ifstream FileTest_Inadyn("/etc/init.d/S85inadyn"); 
	if((FileTest_Inadyn) && (inadyn == off)) 
		system("rm /etc/init.d/s85inadyn");
		system("/etc/init.d/inadyn stop");
	if((!FileTest_Inadyn) && (inadyn == on)) 
		system("ln -s /etc/init.d/inadyn /etc/init.d/s85inadyn");
		system("/etc/init.d/inadyn start");
}

/* BluePanel Menü */
int CBluePanel::showBluePanel()
{
	// Head
	CMenuWidget w_mf("BluePanel", NEUTRINO_ICON_FEATURES, width);
	w_mf.addIntroItems();
	
	LoadConfig();
	
	// Save
	CMenuForwarder *mf1 = new CMenuForwarder("Speichern", true, NULL, this, "save", CRCInput::RC_red);
    mf1->setHint(NEUTRINO_ICON_HINT_SAVE_SETTINGS, "Einstellungen speichern.");
	w_mf.addItem(mf1);
	w_mf.addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, "Softcam"));
	
	// Dienste
	w_mf.addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, "Dienste"));
	
	// OpenVPN
	CMenuOptionStringChooser * openvpn_sw = new CMenuOptionStringChooser("OpenVPN", &openvpn, openvpn_is, this, CRCInput::RC_nokey, "", false);
	openvpn_sw->addOption(std::string(on));
	openvpn_sw->addOption(std::string(off));
	openvpn_sw->setHint(NEUTRINO_ICON_HINT_NETWORK, "OpenVPN Dienst");
	w_mf.addItem(openvpn_sw);
	
	// Inadyn
	CMenuOptionStringChooser * inadyn_sw = new CMenuOptionStringChooser("Inadyn", &inadyn, inadyn_is, this, CRCInput::RC_nokey, "", false);
	inadyn_sw->addOption(std::string(on));
	inadyn_sw->addOption(std::string(off));
	inadyn_sw->setHint(NEUTRINO_ICON_HINT_NETWORK, "Inadyn DynDNS Dienst");
	w_mf.addItem(inadyn_sw);
	
	// Exit
	return w_mf.exec(NULL, "");;
}

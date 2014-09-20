/*
	BluePanel - Neutrino-GUI

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
	width = w_max (40, 10);
}

CBluePanel::~CBluePanel()
{
	//leer
}

int CBluePanel::exec(CMenuTarget* parent, const std::string &actionKey)
{
	dprintf(DEBUG_DEBUG, "Init: BluePanel v1.0 by rgmviper\n");

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
	
	if (actionKey == "restart_cam")
	{
		CHintBox * hintBox = new CHintBox(LOCALE_MESSAGEBOX_INFO, "Softcam wird neu gestartet...");
		hintBox->paint();
		RestartCam();
		system("sleep 2");
		delete hintBox;
		system("sleep 1");
		CHintBox * hintBox_rezap = new CHintBox(LOCALE_MESSAGEBOX_INFO, "Rezap...");
		hintBox_rezap->paint();
		system("sleep 2");
		g_Zapit->Rezap();
		hintBox_rezap->hide();
		delete hintBox_rezap;
		return res;
	}

	res = showBluePanel();
	return res;
}

static int cam_filter(const struct dirent * dent)
{
	if(strstr(dent->d_name, "_cam.sh"))
		return 1;
	return 0;
}

void CBluePanel::RestartCam()
{
	system("/etc/init.d/softcam stop");
	system("sleep 3");
	system("/etc/init.d/softcam start");
}

void CBluePanel::LoadConfig()
{
	// Aktive Softcam
	ifstream load_cam_cfg;
	char delimiter[] = "=";
	char *ptr;
	char zeile[32];
	ptr = none_cam;
	load_cam_cfg.open("/var/tuxbox/config/softcam.conf",ios_base::in);
	while(load_cam_cfg.getline(zeile,200)){
		if(strstr(zeile, "camd=")){
			ptr = strtok(zeile, delimiter);
			ptr = strtok(NULL, delimiter);
		}
	}
	load_cam_cfg.close();
	active_cam = ptr;
	old_cam = ptr;
			
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
		
	// Bootlogo
	bootlogo = off;
	std::ifstream FileTest_Bootlogo("/etc/touch/.bootlogo"); 
	if(FileTest_Bootlogo) 
		bootlogo = on;
}

void CBluePanel::SaveConfig()
{
	// Softcam
	if (old_cam != active_cam){
		old_cam = active_cam;
		if (active_cam == none_cam){
			system("/etc/init.d/softcam stop");
			system("rm /etc/init.d/softcam");
			system("rm /var/tuxbox/config/softcam.conf");
		}else{
			char run[64];
			for(int i = 0; i < softcam_count; i++) {
				if(softcams[i][2] == active_cam){
						system("/etc/init.d/softcam stop");
						system("rm /etc/init.d/softcam");
						sprintf(run, "ln -s %s /etc/init.d/softcam", softcams[i][0]);
						system(run);
						sprintf(run, "echo camd=\"%s\" > /var/tuxbox/config/softcam.conf", softcams[i][2]);
						system(run);
						system("/etc/init.d/softcam start");
				}
			}
		}
	}
		
	// Bootlogo
	std::ifstream FileTest_Bootlogo("/etc/touch/.bootlogo"); 
	if((FileTest_Bootlogo) && (bootlogo == off)) 
		system("rm /etc/touch/.bootlogo");
	if((!FileTest_Bootlogo) && (bootlogo == on)) 
		system("touch /etc/touch/.bootlogo");
		
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
    mf1->setHint(NEUTRINO_ICON_HINT_SAVE_SETTINGS, "Einstellungen speichern und Softcam neustarten.");
	w_mf.addItem(mf1);
	w_mf.addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, "Softcam"));

	// Softcam
	struct dirent **namelist;
	softcam_count = scandir("/var/script", &namelist, cam_filter, alphasort);
	CMenuOptionStringChooser * Softcam = new CMenuOptionStringChooser("Softcam", &active_cam, softcam_count >= 1, this, CRCInput::RC_blue, "", true);
    Softcam->setHint(NEUTRINO_ICON_LOCK, "Aktive Softcam auswählen");
	
	bool found = false;
	ifstream datei;
	char delimiter[] = "\"";
	char *ptr;
	char zeile[32];
		
	for(int i = 0; i < softcam_count; i++) {
		char dir[] = "/var/script/";
		strcat(dir, namelist[i]->d_name);
		strcpy(softcams[i][0],dir);
		strcpy(softcams[i][1],namelist[i]->d_name);
		strcpy(softcams[i][2],namelist[i]->d_name);
		datei.open(dir,ios_base::in);
		while(datei.getline(zeile,200)){
			if(strstr(zeile, "CAMD_NAME=")){
				ptr = strtok(zeile, delimiter);
				ptr = strtok(NULL, delimiter);
				sprintf(softcams[i][2],"%s",ptr);
			}
		}
		datei.close(); 
		Softcam->addOption(softcams[i][2]);
		if(active_cam == softcams[i][2])
			found = true;
		free(namelist[i]);
	}

	if (softcam_count >= 0)
		free(namelist);
		
	if(!found)
		active_cam = none_cam;
		
	Softcam->addOption(std::string(none_cam));
    w_mf.addItem(Softcam);
	
	// Restart softcam
    mf1 = new CMenuForwarder("Softcam Reset", true, NULL, this, "restart_cam", CRCInput::RC_green);
    mf1->setHint(NEUTRINO_ICON_HINT_RELOAD_CHANNELS, "Startet die aktuelle Softcam neu.");
    w_mf.addItem(mf1);
	
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
	
	// Sonstiges
	w_mf.addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, "Sonstiges"));
	
	// Bootlogo
	CMenuOptionStringChooser * bootlogo_sw = new CMenuOptionStringChooser("Bootlogo", &bootlogo, true, this, CRCInput::RC_nokey, "", false);
	bootlogo_sw->addOption(std::string(on));
	bootlogo_sw->addOption(std::string(off));
	bootlogo_sw->setHint(NEUTRINO_ICON_HINT_PICVIEW, "Logo beim Booten anzeigen.");
	w_mf.addItem(bootlogo_sw);
	
	// Exit
	return w_mf.exec(NULL, "");;
}

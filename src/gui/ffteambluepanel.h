/*
	BluePanel - Neutrino-GUI

	Copyright (C) 2001 Steffen Hehn 'McClean'
	and some other guys
	Homepage: http://dbox.cyberphoria.org/

	Copyright (C) 2011 T. Graf 'dbt'
	Homepage: http://www.dbox2-tuning.net/

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

#ifndef __FFTEAMBLUE_PANEL__
#define __FFTEAMBLUE_PANEL__

#include <gui/widget/menue.h>
#include <gui/widget/icons.h>
#include <gui/components/cc.h>
#include <system/configure_network.h>
#include <string>

class CBluePanel : public CMenuTarget, CChangeObserver
{
	private:
		int width, selected;
		int showBluePanel();
					 			
		std::string openvpn;
		std::string inadyn;
		std::string bootlogo;
		std::string active_cam;
		std::string old_cam;
				
		char softcams[10][3][32];
		int softcam_count;
		bool openvpn_is;
		bool inadyn_is;

	public:
		CBluePanel();
		~CBluePanel();
		void RestartCam();
		void LoadConfig();
		void SaveConfig();
		int exec(CMenuTarget* parent, const std::string & actionKey);
};

#endif






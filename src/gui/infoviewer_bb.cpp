/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Homepage: http://dbox.cyberphoria.org/

	Copyright (C) 2012-2013 Stefan Seyfried

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
#include "infoviewer_bb.h"

#include <algorithm>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/sysinfo.h>
#include <sys/vfs.h>
#include <sys/timeb.h>
#include <sys/param.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>

#include <global.h>
#include <neutrino.h>

#include <gui/infoviewer.h>
#include <gui/bouquetlist.h>
#include <gui/widget/icons.h>
#include <gui/widget/hintbox.h>
#include <gui/customcolor.h>
#include <gui/pictureviewer.h>
#include <gui/movieplayer.h>
#include <system/helpers.h>
#include <system/hddstat.h>
#include <daemonc/remotecontrol.h>
#include <driver/volume.h>
#include <driver/radiotext.h>

#include <zapit/femanager.h>
#include <zapit/zapit.h>

#include <video.h>

extern CRemoteControl *g_RemoteControl;	/* neutrino.cpp */
extern cVideo * videoDecoder;

#define COL_INFOBAR_BUTTONS_BACKGROUND (COL_INFOBAR_SHADOW_PLUS_1)

CInfoViewerBB::CInfoViewerBB()
{
	frameBuffer = CFrameBuffer::getInstance();

	is_visible		= false;
	scrambledErr		= false;
	scrambledErrSave	= false;
	scrambledNoSig		= false;
	scrambledNoSigSave	= false;
	scrambledT		= 0;
#if 0
	if(!scrambledT) {
		pthread_create(&scrambledT, NULL, scrambledThread, (void*) this) ;
		pthread_detach(scrambledT);
	}
#endif
	hddscale 		= NULL;
	sysscale 		= NULL;
	bbIconInfo[0].x = 0;
	bbIconInfo[0].h = 0;
	BBarY = 0;
	BBarFontY = 0;
	hddscale 		= NULL;
	sysscale 		= NULL;

	Init();
}

void CInfoViewerBB::Init()
{
	hddwidth		= 0;
	bbIconMaxH 		= 0;
	bbButtonMaxH 		= 0;
	bbIconMinX 		= 0;
	bbButtonMaxX 		= 0;
	fta			= true;
	minX			= 0;

	for (int i = 0; i < CInfoViewerBB::BUTTON_MAX; i++) {
		tmp_bbButtonInfoText[i] = "";
		bbButtonInfo[i].x   = -1;
	}

	InfoHeightY_Info = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getHeight() + 5;
	setBBOffset();

	changePB();
}

CInfoViewerBB::~CInfoViewerBB()
{
	if(scrambledT) {
		pthread_cancel(scrambledT);
		scrambledT = 0;
	}
	if (hddscale)
		delete hddscale;
	if (sysscale)
		delete sysscale;
}

CInfoViewerBB* CInfoViewerBB::getInstance()
{
	static CInfoViewerBB* InfoViewerBB = NULL;

	if(!InfoViewerBB) {
		InfoViewerBB = new CInfoViewerBB();
	}
	return InfoViewerBB;
}

bool CInfoViewerBB::checkBBIcon(const char * const icon, int *w, int *h)
{
	frameBuffer->getIconSize(icon, w, h);
	if ((*w != 0) && (*h != 0))
		return true;
	return false;
}

void CInfoViewerBB::getBBIconInfo()
{
	bbIconMaxH 		= 0;
	BBarY 			= g_InfoViewer->BoxEndY + bottom_bar_offset;
	BBarFontY 		= BBarY + InfoHeightY_Info - (InfoHeightY_Info - g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getHeight()) / 2; /* center in buttonbar */
	bbIconMinX 		= g_InfoViewer->BoxEndX - 8; //should be 10px, but 2px will be reduced for each icon
	CNeutrinoApp* neutrino	= CNeutrinoApp::getInstance();

	for (int i = 0; i < CInfoViewerBB::ICON_MAX; i++) {
		int w = 0, h = 0;
		bool iconView = false;
		switch (i) {
		case CInfoViewerBB::ICON_SUBT:  //no radio
			if (neutrino->getMode() != NeutrinoMessages::mode_radio)
				iconView = checkBBIcon(NEUTRINO_ICON_SUBT, &w, &h);
			break;
		case CInfoViewerBB::ICON_VTXT:  //no radio
			if (neutrino->getMode() != NeutrinoMessages::mode_radio)
				iconView = checkBBIcon(NEUTRINO_ICON_VTXT, &w, &h);
			break;
		case CInfoViewerBB::ICON_RT:
			if ((neutrino->getMode() == NeutrinoMessages::mode_radio) && g_settings.radiotext_enable)
				iconView = checkBBIcon(NEUTRINO_ICON_RADIOTEXTGET, &w, &h);
			break;
		case CInfoViewerBB::ICON_DD:
			if( g_settings.infobar_show_dd_available )
				iconView = checkBBIcon(NEUTRINO_ICON_DD, &w, &h);
			break;
		case CInfoViewerBB::ICON_16_9:  //no radio
			if (neutrino->getMode() != NeutrinoMessages::mode_radio)
				iconView = checkBBIcon(NEUTRINO_ICON_16_9, &w, &h);
			break;
		case CInfoViewerBB::ICON_RES:  //no radio
			if ((g_settings.infobar_show_res < 2) && (neutrino->getMode() != NeutrinoMessages::mode_radio))
				iconView = checkBBIcon(NEUTRINO_ICON_RESOLUTION_1280, &w, &h);
			break;
		case CInfoViewerBB::ICON_CA:
			if (g_settings.casystem_display == 2)
				iconView = checkBBIcon(NEUTRINO_ICON_SCRAMBLED2, &w, &h);
			break;
		case CInfoViewerBB::ICON_TUNER:
			if (CFEManager::getInstance()->getEnabledCount() > 1 && g_settings.infobar_show_tuner == 1)
				iconView = checkBBIcon(NEUTRINO_ICON_TUNER_1, &w, &h);
			break;
		default:
			break;
		}
		if (iconView) {
			bbIconMinX -= w + 2;
			bbIconInfo[i].x = bbIconMinX;
			bbIconInfo[i].h = h;
		}
		else
			bbIconInfo[i].x = -1;
	}
	for (int i = 0; i < CInfoViewerBB::ICON_MAX; i++) {
		if (bbIconInfo[i].x != -1)
			bbIconMaxH = std::max(bbIconMaxH, bbIconInfo[i].h);
	}
	if (g_settings.infobar_show_sysfs_hdd)
		bbIconMinX -= hddwidth + 2;
}

void CInfoViewerBB::getBBButtonInfo()
{
	bbButtonMaxH = 0;
	bbButtonMaxX = g_InfoViewer->ChanInfoX;
	int bbButtonMaxW = 0;
	for (int i = 0; i < CInfoViewerBB::BUTTON_MAX; i++) {
		int w = 0, h = 0;
		bool active;
		std::string text, icon;
		switch (i) {
		case CInfoViewerBB::BUTTON_EPG:
			icon = NEUTRINO_ICON_BUTTON_RED;
			frameBuffer->getIconSize(icon.c_str(), &w, &h);
			text = CUserMenu::getUserMenuButtonName(0, active);
			if (!text.empty())
				break;
			text = g_settings.usermenu[SNeutrinoSettings::BUTTON_RED]->title;
			if (text.empty())
				text = g_Locale->getText(LOCALE_INFOVIEWER_EVENTLIST);
			break;
		case CInfoViewerBB::BUTTON_AUDIO:
			icon = NEUTRINO_ICON_BUTTON_GREEN;
			frameBuffer->getIconSize(icon.c_str(), &w, &h);
			text = CUserMenu::getUserMenuButtonName(1, active);
			if (!text.empty())
				break;
			text = g_settings.usermenu[SNeutrinoSettings::BUTTON_GREEN]->title;
			if (text == g_Locale->getText(LOCALE_AUDIOSELECTMENUE_HEAD))
				text = "";
			if(NeutrinoMessages::mode_ts == CNeutrinoApp::getInstance()->getMode() && !CMoviePlayerGui::getInstance().timeshift){
				text = CMoviePlayerGui::getInstance().CurrentAudioName();
			}else if (!g_RemoteControl->current_PIDs.APIDs.empty()) {
				int selected = g_RemoteControl->current_PIDs.PIDs.selected_apid;
				if (text.empty()){
					text = g_RemoteControl->current_PIDs.APIDs[selected].desc;
				}
			}
			break;
		case CInfoViewerBB::BUTTON_SUBS:
			icon = NEUTRINO_ICON_BUTTON_YELLOW;
			frameBuffer->getIconSize(icon.c_str(), &w, &h);
			text = CUserMenu::getUserMenuButtonName(2, active);
			if (!text.empty())
				break;
			text = g_settings.usermenu[SNeutrinoSettings::BUTTON_YELLOW]->title;
			if (text.empty())
				text = g_Locale->getText((g_RemoteControl->are_subchannels) ? LOCALE_INFOVIEWER_SUBSERVICE : LOCALE_INFOVIEWER_SELECTTIME);
			break;
		case CInfoViewerBB::BUTTON_FEAT:
			icon = NEUTRINO_ICON_BUTTON_BLUE;
			frameBuffer->getIconSize(icon.c_str(), &w, &h);
			text = CUserMenu::getUserMenuButtonName(3, active);
			if (!text.empty())
				break;
			text = g_settings.usermenu[SNeutrinoSettings::BUTTON_BLUE]->title;
			if (text.empty())
				text = g_Locale->getText(LOCALE_INFOVIEWER_STREAMINFO);
			break;
		default:
			break;
		}
		bbButtonInfo[i].w = g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->getRenderWidth(text) + w + 10;
		bbButtonInfo[i].cx = w + 5;
		bbButtonInfo[i].h = h;
		bbButtonInfo[i].text = text;
		bbButtonInfo[i].icon = icon;
		bbButtonInfo[i].active = active;
	}
	// Calculate position/size of buttons
	minX = std::min(bbIconMinX, g_InfoViewer->ChanInfoX + (((g_InfoViewer->BoxEndX - g_InfoViewer->ChanInfoX) * 75) / 100));
	int MaxBr = minX - (g_InfoViewer->ChanInfoX + 10);
	bbButtonMaxX = g_InfoViewer->ChanInfoX + 10;
	int br = 0, count = 0;
	for (int i = 0; i < CInfoViewerBB::BUTTON_MAX; i++) {
		count++;
		bbButtonInfo[i].paint = true;
		br += bbButtonInfo[i].w;
		bbButtonInfo[i].x = bbButtonMaxX;
		bbButtonMaxX += bbButtonInfo[i].w;
		bbButtonMaxW = std::max(bbButtonMaxW, bbButtonInfo[i].w);
	}
	if (br > MaxBr)
		printf("[infoviewer_bb:%s#%d] width br (%d) > MaxBr (%d) count %d\n", __func__, __LINE__, br, MaxBr, count);
#if 0
	int Btns = 0;
	// counting buttons
	for (int i = 0; i < CInfoViewerBB::BUTTON_MAX; i++) {
		if (bbButtonInfo[i].x != -1) {
			Btns++;
		}
	}
	bbButtonMaxX = g_InfoViewer->ChanInfoX + 10;

	bbButtonInfo[CInfoViewerBB::BUTTON_EPG].x = bbButtonMaxX;
	bbButtonInfo[CInfoViewerBB::BUTTON_FEAT].x = minX - bbButtonInfo[CInfoViewerBB::BUTTON_FEAT].w;

	int x1 = bbButtonInfo[CInfoViewerBB::BUTTON_EPG].x + bbButtonInfo[CInfoViewerBB::BUTTON_EPG].w;
	int rest = bbButtonInfo[CInfoViewerBB::BUTTON_FEAT].x - x1;

	if (Btns < 4) {
		rest -= bbButtonInfo[CInfoViewerBB::BUTTON_AUDIO].w;
		bbButtonInfo[CInfoViewerBB::BUTTON_AUDIO].x = x1 + rest / 2;
	}
	else {
		rest -= bbButtonInfo[CInfoViewerBB::BUTTON_AUDIO].w + bbButtonInfo[CInfoViewerBB::BUTTON_SUBS].w;
		rest = rest / 3;
		bbButtonInfo[CInfoViewerBB::BUTTON_AUDIO].x = x1 + rest;
		bbButtonInfo[CInfoViewerBB::BUTTON_SUBS].x = bbButtonInfo[CInfoViewerBB::BUTTON_AUDIO].x + 
								bbButtonInfo[CInfoViewerBB::BUTTON_AUDIO].w + rest;
	}
#endif
	bbButtonMaxX = g_InfoViewer->ChanInfoX + 10;
	int step = MaxBr / 4;
	if (count > 0) { /* avoid div-by-zero :-) */
		step = MaxBr / count;
		count = 0;
		for (int i = 0; i < BUTTON_MAX; i++) {
			if (!bbButtonInfo[i].paint)
				continue;
			bbButtonInfo[i].x = bbButtonMaxX + step * count;
			// printf("%s: i = %d count = %d b.x = %d\n", __func__, i, count, bbButtonInfo[i].x);
			count++;
		}
	} else {
		printf("[infoviewer_bb:%s#%d: count <= 0???\n", __func__, __LINE__);
		bbButtonInfo[BUTTON_EPG].x   = bbButtonMaxX;
		bbButtonInfo[BUTTON_AUDIO].x = bbButtonMaxX + step;
		bbButtonInfo[BUTTON_SUBS].x  = bbButtonMaxX + 2*step;
		bbButtonInfo[BUTTON_FEAT].x  = bbButtonMaxX + 3*step;
	}
}

void CInfoViewerBB::showBBButtons(const int modus)
{
	if (!is_visible)
		return;
	int i;
	bool paint = false;

	if (g_settings.volume_pos == CVolumeBar::VOLUMEBAR_POS_BOTTOM_LEFT || 
	    g_settings.volume_pos == CVolumeBar::VOLUMEBAR_POS_BOTTOM_RIGHT || 
	    g_settings.volume_pos == CVolumeBar::VOLUMEBAR_POS_BOTTOM_CENTER || 
	    g_settings.volume_pos == CVolumeBar::VOLUMEBAR_POS_HIGHER_CENTER)
		g_InfoViewer->isVolscale = CVolume::getInstance()->hideVolscale();
	else
		g_InfoViewer->isVolscale = false;

	getBBButtonInfo();
	for (i = 0; i < CInfoViewerBB::BUTTON_MAX; i++) {
		if (tmp_bbButtonInfoText[i] != bbButtonInfo[i].text) {
			paint = true;
			break;
		}
	}

	if (paint) {
		int last_x = minX;
		frameBuffer->paintBoxRel(g_InfoViewer->ChanInfoX, BBarY, minX - g_InfoViewer->ChanInfoX, InfoHeightY_Info, COL_INFOBAR_BUTTONS_BACKGROUND, RADIUS_LARGE, CORNER_BOTTOM); //round
		for (i = BUTTON_MAX; i > 0;) {
			--i;
			if ((bbButtonInfo[i].x <= g_InfoViewer->ChanInfoX) || (bbButtonInfo[i].x >= g_InfoViewer->BoxEndX) || (!bbButtonInfo[i].paint))
				continue;
			if (bbButtonInfo[i].x > 0) {
				if (bbButtonInfo[i].x + bbButtonInfo[i].w > last_x) /* text too long */
					bbButtonInfo[i].w = last_x - bbButtonInfo[i].x;
				last_x = bbButtonInfo[i].x;
				if (bbButtonInfo[i].w - bbButtonInfo[i].cx <= 0) {
					printf("[infoviewer_bb:%d cannot paint icon %d (not enough space)\n",
							__LINE__, i);
					continue;
				}
				if (bbButtonInfo[i].active) {
					frameBuffer->paintIcon(bbButtonInfo[i].icon, bbButtonInfo[i].x, BBarY, InfoHeightY_Info);

					g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(bbButtonInfo[i].x + bbButtonInfo[i].cx, BBarFontY, 
						bbButtonInfo[i].w - bbButtonInfo[i].cx, bbButtonInfo[i].text, COL_INFOBAR_TEXT);
				}
			}
		}

		if (modus == CInfoViewerBB::BUTTON_AUDIO)
			showIcon_DD();

		for (i = 0; i < CInfoViewerBB::BUTTON_MAX; i++) {
			tmp_bbButtonInfoText[i] = bbButtonInfo[i].text;
		}
	}
	if (g_InfoViewer->isVolscale)
		CVolume::getInstance()->showVolscale();
}

void CInfoViewerBB::showBBIcons(const int modus, const std::string & icon)
{
	if ((bbIconInfo[modus].x <= g_InfoViewer->ChanInfoX) || (bbIconInfo[modus].x >= g_InfoViewer->BoxEndX))
		return;
	if ((modus >= CInfoViewerBB::ICON_SUBT) && (modus < CInfoViewerBB::ICON_MAX) && (bbIconInfo[modus].x != -1) && (is_visible)) {
		frameBuffer->paintIcon(icon, bbIconInfo[modus].x, BBarY, 
				       InfoHeightY_Info, 1, true, true, COL_INFOBAR_BUTTONS_BACKGROUND);
	}
}

void CInfoViewerBB::paintshowButtonBar()
{
	if (!is_visible)
		return;
	getBBIconInfo();
	for (int i = 0; i < CInfoViewerBB::BUTTON_MAX; i++) {
		tmp_bbButtonInfoText[i] = "";
	}
	g_InfoViewer->sec_timer_id = g_RCInput->addTimer(1*1000*1000, false);

	if (g_settings.casystem_display < 2)
		paintCA_bar(0,0);

	frameBuffer->paintBoxRel(g_InfoViewer->ChanInfoX, BBarY, g_InfoViewer->BoxEndX - g_InfoViewer->ChanInfoX, InfoHeightY_Info, COL_INFOBAR_BUTTONS_BACKGROUND, RADIUS_LARGE, CORNER_BOTTOM); //round

	g_InfoViewer->showSNR();

	// Buttons
	showBBButtons();

	// Icons, starting from right
	showIcon_SubT();
	showIcon_VTXT();
	showIcon_DD();
	showIcon_16_9();
#if 0
	scrambledCheck(true);
#endif
	showIcon_CA_Status(0);
	showIcon_Resolution();
	showIcon_Tuner();
	showSysfsHdd();
}

void CInfoViewerBB::showIcon_SubT()
{
	if (!is_visible)
		return;
	bool have_sub = false;
	CZapitChannel * cc = CNeutrinoApp::getInstance()->channelList->getChannel(CNeutrinoApp::getInstance()->channelList->getActiveChannelNumber());
	if (cc && cc->getSubtitleCount())
		have_sub = true;

	showBBIcons(CInfoViewerBB::ICON_SUBT, (have_sub) ? NEUTRINO_ICON_SUBT : NEUTRINO_ICON_SUBT_GREY);
}

void CInfoViewerBB::showIcon_VTXT()
{
	if (!is_visible)
		return;
	showBBIcons(CInfoViewerBB::ICON_VTXT, (g_RemoteControl->current_PIDs.PIDs.vtxtpid != 0) ? NEUTRINO_ICON_VTXT : NEUTRINO_ICON_VTXT_GREY);
}

void CInfoViewerBB::showIcon_DD()
{
	if (!is_visible || !g_settings.infobar_show_dd_available)
		return;
	std::string dd_icon;
	if ((g_RemoteControl->current_PIDs.PIDs.selected_apid < g_RemoteControl->current_PIDs.APIDs.size()) && 
	    (g_RemoteControl->current_PIDs.APIDs[g_RemoteControl->current_PIDs.PIDs.selected_apid].is_ac3))
		dd_icon = NEUTRINO_ICON_DD;
	else 
		dd_icon = g_RemoteControl->has_ac3 ? NEUTRINO_ICON_DD_AVAIL : NEUTRINO_ICON_DD_GREY;

	showBBIcons(CInfoViewerBB::ICON_DD, dd_icon);
}

void CInfoViewerBB::showIcon_RadioText(bool rt_available)
{
	if (!is_visible || !g_settings.radiotext_enable)
		return;

	std::string rt_icon;
	if (rt_available)
		rt_icon = (g_Radiotext->S_RtOsd) ? NEUTRINO_ICON_RADIOTEXTGET : NEUTRINO_ICON_RADIOTEXTWAIT;
	else
		rt_icon = NEUTRINO_ICON_RADIOTEXTOFF;

	showBBIcons(CInfoViewerBB::ICON_RT, rt_icon);
}

void CInfoViewerBB::showIcon_16_9()
{
	if (!is_visible)
		return;
	if ((g_InfoViewer->aspectRatio == 0) || ( g_RemoteControl->current_PIDs.PIDs.vpid == 0 ) || (g_InfoViewer->aspectRatio != videoDecoder->getAspectRatio())) {
		if (g_InfoViewer->chanready && g_RemoteControl->current_PIDs.PIDs.vpid > 0 ) {
			g_InfoViewer->aspectRatio = videoDecoder->getAspectRatio();
		}
		else
			g_InfoViewer->aspectRatio = 0;

		showBBIcons(CInfoViewerBB::ICON_16_9, (g_InfoViewer->aspectRatio > 2) ? NEUTRINO_ICON_16_9 : NEUTRINO_ICON_16_9_GREY);
	}
}

void CInfoViewerBB::showIcon_Resolution()
{
	if ((!is_visible) || (g_settings.infobar_show_res == 2)) //show resolution icon is off
		return;
	if (CNeutrinoApp::getInstance()->getMode() == NeutrinoMessages::mode_radio)
		return;
	const char *icon_name = NULL;
#if 0
	if ((scrambledNoSig) || ((!fta) && (scrambledErr)))
#else
	if (!g_InfoViewer->chanready || videoDecoder->getBlank())
#endif
	{
		icon_name = NEUTRINO_ICON_RESOLUTION_000;
	} else {
		int xres, yres, framerate;
		if (g_settings.infobar_show_res == 0) {//show resolution icon on infobar
			videoDecoder->getPictureInfo(xres, yres, framerate);
			switch (yres) {
			case 1920:
				icon_name = NEUTRINO_ICON_RESOLUTION_1920;
				break;
			case 1088:
			case 1080:
				icon_name = NEUTRINO_ICON_RESOLUTION_1080;
				break;
			case 1440:
				icon_name = NEUTRINO_ICON_RESOLUTION_1440;
				break;
			case 1280:
				icon_name = NEUTRINO_ICON_RESOLUTION_1280;
				break;
			case 720:
				icon_name = NEUTRINO_ICON_RESOLUTION_720;
				break;
			case 704:
				icon_name = NEUTRINO_ICON_RESOLUTION_704;
				break;
			case 576:
				icon_name = NEUTRINO_ICON_RESOLUTION_576;
				break;
			case 544:
				icon_name = NEUTRINO_ICON_RESOLUTION_544;
				break;
			case 528:
				icon_name = NEUTRINO_ICON_RESOLUTION_528;
				break;
			case 480:
				icon_name = NEUTRINO_ICON_RESOLUTION_480;
				break;
			case 382:
				icon_name = NEUTRINO_ICON_RESOLUTION_382;
				break;
			case 352:
				icon_name = NEUTRINO_ICON_RESOLUTION_352;
				break;
			case 288:
				icon_name = NEUTRINO_ICON_RESOLUTION_288;
				break;
			default:
				icon_name = NEUTRINO_ICON_RESOLUTION_000;
				break;
			}
		}
		if (g_settings.infobar_show_res == 1) {//show simple resolution icon on infobar
			videoDecoder->getPictureInfo(xres, yres, framerate);
			if (yres > 576)
				icon_name = NEUTRINO_ICON_RESOLUTION_HD;
			else if (yres > 0)
				icon_name = NEUTRINO_ICON_RESOLUTION_SD;
			else
				icon_name = NEUTRINO_ICON_RESOLUTION_000;
		}
	}
	showBBIcons(CInfoViewerBB::ICON_RES, icon_name);
}

void CInfoViewerBB::showOne_CAIcon()
{
	std::string sIcon = "";
#if 0
	if (CNeutrinoApp::getInstance()->getMode() != NeutrinoMessages::mode_radio) {
		if (scrambledNoSig)
			sIcon = NEUTRINO_ICON_SCRAMBLED2_BLANK;
		else {	
			if (fta)
				sIcon = NEUTRINO_ICON_SCRAMBLED2_GREY;
			else
				sIcon = (scrambledErr) ? NEUTRINO_ICON_SCRAMBLED2_RED : NEUTRINO_ICON_SCRAMBLED2;
		}
	}
	else
#endif
		sIcon = (fta) ? NEUTRINO_ICON_SCRAMBLED2_GREY : NEUTRINO_ICON_SCRAMBLED2;
	showBBIcons(CInfoViewerBB::ICON_CA, sIcon);
}

void CInfoViewerBB::showIcon_Tuner()
{
	if (CFEManager::getInstance()->getEnabledCount() <= 1 || !g_settings.infobar_show_tuner)
		return;

	std::string icon_name;
	switch (CFEManager::getInstance()->getLiveFE()->getNumber()) {
		case 1:
			icon_name = NEUTRINO_ICON_TUNER_2;
			break;
		case 2:
			icon_name = NEUTRINO_ICON_TUNER_3;
			break;
		case 3:
			icon_name = NEUTRINO_ICON_TUNER_4;
			break;
		case 0:
		default:
			icon_name = NEUTRINO_ICON_TUNER_1;
			break;
	}
	showBBIcons(CInfoViewerBB::ICON_TUNER, icon_name);
}

void CInfoViewerBB::showSysfsHdd()
{
	if (g_settings.infobar_show_sysfs_hdd) {
		//sysFS info
		int percent = 0;
		uint64_t t, u;
		if (get_fs_usage("/", t, u))
			percent = (int)((u * 100ULL) / t);
		showBarSys(percent);

		showBarHdd(cHddStat::getInstance()->getPercent());
	}
}

void CInfoViewerBB::showBarSys(int percent)
{	
	if (is_visible){
		sysscale->setDimensionsAll(bbIconMinX, BBarY + InfoHeightY_Info / 2 - 2 - 6, hddwidth, 6);
		sysscale->setValues(percent, 100);
		sysscale->paint();
	}
}

void CInfoViewerBB::showBarHdd(int percent)
{
	if (is_visible) {
		if (percent >= 0){
			hddscale->setDimensionsAll(bbIconMinX, BBarY + InfoHeightY_Info / 2 + 2 + 0, hddwidth, 6);
			hddscale->setValues(percent, 100);
			hddscale->paint();
		}else {
			frameBuffer->paintBoxRel(bbIconMinX, BBarY + InfoHeightY_Info / 2 + 2 + 0, hddwidth, 6, COL_INFOBAR_BUTTONS_BACKGROUND);
			hddscale->reset();
		}
	}
}

void CInfoViewerBB::paint_ca_icons(int caid, char * icon, int &icon_space_offset)
{
	char buf[20];
	int endx = g_InfoViewer->BoxEndX -3;
	int py = g_InfoViewer->BoxEndY + 2; /* hand-crafted, should be automatic */
	int px = 0;
	static map<int, std::pair<int,const char*> > icon_map;
	const int icon_space = 2, icon_number = 10;

	static int icon_offset[icon_number] = {0,0,0,0,0,0,0,0,0,0};
	static int icon_sizeW [icon_number] = {0,0,0,0,0,0,0,0,0,0};
	static bool init_flag = false;

	if (!init_flag) {
		init_flag = true;
		int icon_sizeH = 0, index = 0;
		map<int, std::pair<int,const char*> >::const_iterator it;

		icon_map[0x0E00] = std::make_pair(index++,"powervu");
		icon_map[0x4A00] = std::make_pair(index++,"d");
		icon_map[0x2600] = std::make_pair(index++,"biss");
		icon_map[0x0600] = std::make_pair(index++,"ird");
		icon_map[0x0100] = std::make_pair(index++,"seca");
		icon_map[0x0500] = std::make_pair(index++,"via");
		icon_map[0x1800] = std::make_pair(index++,"nagra");
		icon_map[0x0B00] = std::make_pair(index++,"conax");
		icon_map[0x0D00] = std::make_pair(index++,"cw");
		icon_map[0x0900] = std::make_pair(index  ,"nds");

		for (it=icon_map.begin(); it!=icon_map.end(); ++it) {
			snprintf(buf, sizeof(buf), "%s_%s", (*it).second.second, icon);
			frameBuffer->getIconSize(buf, &icon_sizeW[(*it).second.first], &icon_sizeH);
		}

		for (int j = 0; j < icon_number; j++) {
			for (int i = j; i < icon_number; i++) {
				icon_offset[j] += icon_sizeW[i] + icon_space;
			}
		}
	}
	caid &= 0xFF00;

	if (icon_offset[icon_map[caid].first] == 0)
		return;

	if (g_settings.casystem_display == 0) {
		px = endx - (icon_offset[icon_map[caid].first] - icon_space );
	} else {
		icon_space_offset += icon_sizeW[icon_map[caid].first];
		px = endx - icon_space_offset;
		icon_space_offset += 2;
	}

	if (px) {
		snprintf(buf, sizeof(buf), "%s_%s", icon_map[caid].second, icon);
		if ((px >= (endx-8)) || (px <= 0))
			printf("#####[%s:%d] Error paint icon %s, px: %d,  py: %d, endx: %d, icon_offset: %d\n", 
				__FUNCTION__, __LINE__, buf, px, py, endx, icon_offset[icon_map[caid].first]);
		else
			frameBuffer->paintIcon(buf, px, py);
	}
}

void CInfoViewerBB::showIcon_CA_Status(int /*notfirst*/)
{
	if (!is_visible)
		return;

	if (g_settings.casystem_display == 3)
		return;
	if(NeutrinoMessages::mode_ts == CNeutrinoApp::getInstance()->getMode() && !CMoviePlayerGui::getInstance().timeshift){
		if (g_settings.casystem_display == 2) {
			fta = true;
			showOne_CAIcon();
		}
		return;
	}

	const int caids[] = {  0x900, 0xD00, 0xB00, 0x1800, 0x0500, 0x0100, 0x600,  0x2600, 0x4a00, 0x0E00 };
	const char * white = (char *) "white";
	const char * yellow = (char *) "yellow";
	const char * green = (char *) "green";
	int icon_space_offset = 0;
	const char *ecm_info_f = "/tmp/ecm.info";
	if(!g_InfoViewer->chanready) {
		if (g_settings.infoviewer_ecm_info == 1)
			frameBuffer->paintBackgroundBoxRel(g_InfoViewer->BoxEndX-220, g_InfoViewer->BoxStartY-185, 225, g_InfoViewer->ChanHeight+105);
		else if (g_settings.infoviewer_ecm_info == 2)
			frameBuffer->paintBackgroundBoxRel(g_InfoViewer->BoxEndX-220, g_InfoViewer->BoxStartY-185, 225, g_InfoViewer->ChanHeight+105);

		unlink(ecm_info_f);

		if (g_settings.casystem_display == 2) {
			fta = true;
			showOne_CAIcon();
		}
		else if(g_settings.casystem_display == 0) {
			for (int i = 0; i < (int)(sizeof(caids)/sizeof(int)); i++) {
				paint_ca_icons(caids[i], (char *) white, icon_space_offset);
			}
		}
		return;
	}

	CZapitChannel * channel = CZapit::getInstance()->GetCurrentChannel();
	if(!channel)
		return;

	if (g_settings.casystem_display == 2) {
		fta = channel->camap.empty();
		showOne_CAIcon();
		return;
	}
		emu = 0;
	if(file_exists("/var/etc/.mgcamd"))
		emu = 1;
	else if(file_exists("/var/etc/.gbox"))
		emu = 2;
	else if(file_exists("/var/etc/.oscam"))
		emu = 3;
	if(file_exists("/var/etc/.osemu"))
		emu = 4;
	else if(file_exists("/var/etc/.wicard"))
		emu = 5;
	else if(file_exists("/var/etc/.camd3"))
		emu = 6;

	if ( (file_exists(ecm_info_f)) && ((g_settings.infoviewer_ecm_info == 1) || (g_settings.infoviewer_ecm_info == 2)) )
		paintECM();


	if ((g_settings.casystem_display == 0) || (g_settings.casystem_display == 1))
	{
		FILE* fd = fopen (ecm_info_f, "r");
		int ecm_caid = 0;
		int decMode = 0;
		bool mgcamd_emu = emu==1 ? true:false;
		char ecm_pid[16] = {0};
		if (fd)
		{
			char *buffer = NULL, *card = NULL;
			size_t len = 0;
			ssize_t read;
			char decode[16] = {0};
			while ((read = getline(&buffer, &len, fd)) != -1)
			{
				if ((sscanf(buffer, "=%*[^9-0]%x", &ecm_caid) == 1) || (sscanf(buffer, "caid: %x", &ecm_caid) == 1)){
					if (mgcamd_emu && ((ecm_caid & 0xFF00) == 0x1700)){
						sscanf(buffer, "=%*[^','], pid %6s",ecm_pid);
					}
					continue;
				}
				else if ((sscanf(buffer, "decode:%15s", decode) == 1) || (sscanf(buffer, "source:%15s", decode) == 1) || (sscanf(buffer, "from: %15s", decode) == 1))
				{
					card = strstr(buffer, "127.0.0.1");
					break;
				}
			}
			fclose (fd);
			if (buffer)
				free (buffer);
			if (strncasecmp(decode, "net", 3) == 0)
			  decMode = (card == NULL) ? 1 : 3; // net == 1, card == 3
			else if ((strncasecmp(decode, "emu", 3) == 0) || (strncasecmp(decode, "int", 3) == 0) || (strncasecmp(decode, "cache", 5) == 0) || (strstr(decode, "/" ) != NULL))
			  decMode = 2; //emu
			else if ((strncasecmp(decode, "emu", 3) == 0) || (strncasecmp(decode, "slot", 4) == 0) || (strncasecmp(decode, "local", 5) == 0))
			  decMode = 3; //card
		}
		if (mgcamd_emu && ((ecm_caid & 0xFF00) == 0x1700)){
			const char *pid_info_f = "/tmp/pid.info";
			FILE* pidinfo = fopen (pid_info_f, "r");
			if (pidinfo){
				char *buf_mg = NULL;
				size_t mg_len = 0;
				ssize_t mg_read;
				while ((mg_read = getline(&buf_mg, &mg_len, pidinfo)) != -1){
					if(strcasestr(buf_mg, ecm_pid)){
						int pidnagra = 0;
						sscanf(buf_mg, "%*[^':']: CaID: %x *", &pidnagra);
						ecm_caid = pidnagra;
					}
				}
				fclose (pidinfo);
				if (buf_mg)
					free (buf_mg);
			}
		}
		if ( (ecm_caid & 0xFF00) == 0x1700 ){
			bool nagra_found = false;
			bool beta_found = false;
			for(casys_map_iterator_t it = channel->camap.begin(); it != channel->camap.end(); ++it) {
				int caid = (*it) & 0xFF00;
				if(caid == 0x1800)
					nagra_found = true;
				if (caid == 0x1700)
					beta_found = true;
			}
			if(beta_found)
				ecm_caid = 0x600;
			else if(!beta_found && nagra_found)
				ecm_caid = 0x1800;
		}

#if 0
		static int icon_space_offset = 0;
		if ((g_settings.casystem_display == 1) && (icon_space_offset)) {
			paintCA_bar(0,icon_space_offset);
			icon_space_offset = 0;
		}
#endif
		paintEmuIcons(decMode);

		for (int i = 0; i < (int)(sizeof(caids)/sizeof(int)); i++) {
			bool found = false;
			for(casys_map_iterator_t it = channel->camap.begin(); it != channel->camap.end(); ++it) {
				int caid = (*it) & 0xFF00;
				if (caid == 0x1700)
					caid = 0x0600;
				if((found = (caid == caids[i])))
					break;
			}
			if(g_settings.casystem_display == 0)
				paint_ca_icons(caids[i], (char *) (found ? (caids[i] == (ecm_caid & 0xFF00) ? green : yellow) : white), icon_space_offset);
			else if(found)
				paint_ca_icons(caids[i], (char *) ( caids[i] == (ecm_caid & 0xFF00) ? green : yellow), icon_space_offset);
		}
	}
}


void CInfoViewerBB::paintCA_bar(int left, int right)
{
	int xcnt = (g_InfoViewer->BoxEndX - g_InfoViewer->ChanInfoX) / 4;
	int ycnt = bottom_bar_offset / 4;
	if (right)
		right = xcnt - ((right/4)+1);
	if (left)
		left =  xcnt - ((left/4)-1);

	frameBuffer->paintBox(g_InfoViewer->ChanInfoX + (right*4), g_InfoViewer->BoxEndY, g_InfoViewer->BoxEndX - (left*4), g_InfoViewer->BoxEndY + bottom_bar_offset, (g_settings.dotmatrix == 1) ? COL_BLACK : COL_INFOBAR_PLUS_0);

	if (g_settings.dotmatrix == 1)
	if (left)
		left -= 1;

	for (int i = 0  + right; i < xcnt - left; i++) {
		for (int j = 0; j < ycnt; j++) {
			frameBuffer->paintBoxRel((g_InfoViewer->ChanInfoX + 2) + i*4, g_InfoViewer->BoxEndY + 2 + j*4, 2, 2, COL_INFOBAR_PLUS_1);
		}
	}
}

void CInfoViewerBB::changePB()
{
	hddwidth = frameBuffer->getScreenWidth(true) * ((g_settings.screen_preset == 1) ? 10 : 8) / 128; /* 80(CRT)/100(LCD) pix if screen is 1280 wide */
	if (!hddscale) {
		hddscale = new CProgressBar();
		hddscale->setType(CProgressBar::PB_REDRIGHT);
	}
	
	if (!sysscale) {
		sysscale = new CProgressBar();
		sysscale->setType(CProgressBar::PB_REDRIGHT);
	}
}

void CInfoViewerBB::reset_allScala()
{
	hddscale->reset();
	sysscale->reset();
	//lasthdd = lastsys = -1;
}

void CInfoViewerBB::setBBOffset()
{
	bottom_bar_offset = (g_settings.casystem_display < 2) ? 22 : 0;
}

void* CInfoViewerBB::scrambledThread(void *arg)
{
 	pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, 0);
	CInfoViewerBB *infoViewerBB = static_cast<CInfoViewerBB*>(arg);
	while(1) {
		if (infoViewerBB->is_visible)
			infoViewerBB->scrambledCheck();
		usleep(500*1000);
	}
	return 0;
}

void CInfoViewerBB::scrambledCheck(bool force)
{
	scrambledErr = false;
	scrambledNoSig = false;
	if (videoDecoder->getBlank()) {
		if (videoDecoder->getPlayState())
			scrambledErr = true;
		else
			scrambledNoSig = true;
	}
	
	if ((scrambledErr != scrambledErrSave) || (scrambledNoSig != scrambledNoSigSave) || force) {
		showIcon_CA_Status(0);
		showIcon_Resolution();
		scrambledErrSave = scrambledErr;
		scrambledNoSigSave = scrambledNoSig;
	}
}

void CInfoViewerBB::paintEmuIcons(int decMode)
{
	int attackMode = 0;
	struct stat stat_buf;
	if(::stat("/tmp/atack.txt", &stat_buf) == 0)
	{
		if (stat_buf.st_size > 0)
		{
			attackMode = 1;
		}
		else
		{
			attackMode = 0;
		}
	}
	else
	{
		attackMode = 0;
	}

	int gsmsMode = 0;
	if (file_exists("/tmp/gsms.log"))
		gsmsMode = 1;
	else
		gsmsMode = 0;

	char buf[20];
	int py = g_InfoViewer->BoxEndY + 2; /* hand-crafted, should be automatic */

	const char emu_green[] = "green";
	const char emu_gray[] = "white";
	const char emu_yellow[] = "yellow";
	enum E{
		GBOX,MGCAMD,OSCAM,OSEMU,WICARD,CAMD3,NEWCS,CS2GBOX,NET,EMU,CARD,ATTACK,GSMS
	};
	static int emus_icon_sizeW[GSMS+1] = {0};
	const char *icon_emu[GSMS+1] = {"gbox", "mgcamd", "oscam", "net", "emu", "card"};
	int icon_sizeH = 0;
	static int ga = g_InfoViewer->ChanInfoX+30+16;
	if (emus_icon_sizeW[GBOX] == 0)
	{
		for (E e=GBOX; e <= GSMS; e = E(e+1))
		{
			snprintf(buf, sizeof(buf), "%s_%s", icon_emu[e], emu_green);
			frameBuffer->getIconSize(buf, &emus_icon_sizeW[e], &icon_sizeH);
			if(e < ATTACK)
				ga+=emus_icon_sizeW[e];
		}
	}
	struct stat sb;
	int icon_emuX = g_InfoViewer->ChanInfoX ;
	static int icon_offset = 0;
	int icon_flag = 0; // gray = 0, yellow = 1, green = 2

	if ((g_settings.casystem_display == 1) && (icon_offset))
	{
		paintCA_bar(icon_offset, 0);
		icon_offset = 0;
	}
	bool emuMG = (emu==1 || emu == 3 || file_exists("/var/etc/.no_attack_gsms_icon") ) ;
	for (E e = GBOX; e <= GSMS; e = E(e+1))
	{
		switch (e)
		{
			case GBOX:
			case MGCAMD:
			case OSCAM:
			case OSEMU:
			case CAMD3:
			case WICARD:
			case NEWCS:
			case CS2GBOX:
			snprintf(buf, sizeof(buf), "/var/etc/.%s", icon_emu[e]);
			icon_flag = (stat(buf, &sb) == -1) ? 0 : decMode ? 2 : 1;
			break;
			case NET:
			if (g_settings.casystem_display == 0)
				icon_emuX += 15 ;

			icon_flag = (decMode == 1) ? 2 : 0;
			break;
			case EMU:
			icon_flag = (decMode == 2) ? 2 : 0;
			break;
			case CARD:
			icon_flag = (decMode == 3) ? 2 : 0;
			break;
			case ATTACK:
			icon_flag = (attackMode == 1) ? 2 : 0;
			break;
			case GSMS:
			icon_flag = (gsmsMode == 1) ? 2 : 0;
			break;
			default:
			break;
		}
		if ( (g_settings.casystem_display == 1 && icon_flag != 0) || (g_settings.casystem_display == 0 && (e> CS2GBOX ||  icon_flag != 0)) )
		{
			snprintf(buf, sizeof(buf), "%s_%s", icon_emu[e], (icon_flag == 0) ? emu_gray : (icon_flag == 1) ? emu_yellow : emu_green);
			if(e==ATTACK){
				if(emuMG)
					continue;
				frameBuffer->paintIcon(buf,ga , py);
			}else if(e==GSMS){
				if(emuMG)
				      continue;
				frameBuffer->paintIcon(buf,ga+emus_icon_sizeW[ATTACK] , py);
			}
			else
			{
				frameBuffer->paintIcon(buf, icon_emuX, py);
				if (g_settings.casystem_display == 1)
				{
					icon_offset += emus_icon_sizeW[e] + 2;
					icon_emuX   += icon_offset;
				}
				else
					icon_emuX += emus_icon_sizeW[e] + 2;
			}
		}
	}
}

void CInfoViewerBB::painttECMInfo(int xa, const char *info, char *caid, char *decode, char *response, char *prov)
{
	frameBuffer->paintBoxRel(g_InfoViewer->BoxEndX-215, g_InfoViewer->BoxStartY-120, 220, g_InfoViewer->ChanHeight+40, COL_INFOBAR_SHADOW_PLUS_0, g_settings.rounded_corners ? CORNER_RADIUS_MID : 0);
	frameBuffer->paintBoxRel(g_InfoViewer->BoxEndX-220, g_InfoViewer->BoxStartY-125, 220, g_InfoViewer->ChanHeight+40, COL_INFOBAR_PLUS_0, g_settings.rounded_corners ? CORNER_RADIUS_MID : 0);
	g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(g_InfoViewer->BoxEndX-190, g_InfoViewer->BoxStartY-100, xa, info, COL_INFOBAR_TEXT, 0, true);
	g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(g_InfoViewer->BoxEndX-215, g_InfoViewer->BoxStartY-80, 80, "CaID:", COL_INFOBAR_TEXT, 0, true);
	g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(g_InfoViewer->BoxEndX-150, g_InfoViewer->BoxStartY-80, 130, caid, COL_INFOBAR_TEXT, 0, true);
	g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(g_InfoViewer->BoxEndX-215, g_InfoViewer->BoxStartY-60, 80, "Decode:", COL_INFOBAR_TEXT, 0, true);
	g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(g_InfoViewer->BoxEndX-150, g_InfoViewer->BoxStartY-60, 70, decode, COL_INFOBAR_TEXT, 0, true);
	if(response[0] != 0 )
	{
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(g_InfoViewer->BoxEndX-90, g_InfoViewer->BoxStartY-60, 15, "in", COL_INFOBAR_TEXT, 0, true);
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(g_InfoViewer->BoxEndX-70, g_InfoViewer->BoxStartY-60, 45, response, COL_INFOBAR_TEXT, 0, true);
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(g_InfoViewer->BoxEndX-25, g_InfoViewer->BoxStartY-60, 10, "s", COL_INFOBAR_TEXT, 0, true);
	}
	g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(g_InfoViewer->BoxEndX-215, g_InfoViewer->BoxStartY-40, 80, "Provider:", COL_INFOBAR_TEXT, 0, true);
	g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(g_InfoViewer->BoxEndX-150, g_InfoViewer->BoxStartY-40, 130, prov, COL_INFOBAR_TEXT, 0, true);
}

void CInfoViewerBB::paintECM()
{
	char caid1[5] = {0};
	char pid1[5] = {0};
	char prov1[8] = {0};
	char cw0_[8][3];
	char cw1_[8][3];
	char source1[30] = {0};
	char response1[10] = {0};
	char reader[20]  = {0};
	char protocol[20]  = {0};

	char tmp;
//	static int cw0=0,cw1=0;
//	int tmp_cw0=0,tmp_cw1=0;
	const char *ecm_info = "/tmp/ecm.info";
	FILE* ecminfo = fopen (ecm_info, "r");
	bool ecmInfoEmpty = true;
	if (ecminfo)
	{
		char *buffer = NULL;
		size_t len = 0;
		ssize_t read;

		while ((read = getline(&buffer, &len, ecminfo)) != -1)
		{
			ecmInfoEmpty = false;
			if(emu == 1 || emu == 2){
				sscanf(buffer, "%*s %*s ECM on CaID 0x%4s, pid 0x%4s", caid1, pid1);										// gbox, mgcamd
				sscanf(buffer, "prov: %06[^',',(]", prov1);														// gbox, mgcamd
			}
			if(emu == 2){
				sscanf(buffer, "decode:%8s", source1);														// gbox
				sscanf(buffer, "response:%05s", response1);													// gbox
				sscanf(buffer, "provider: %02s", prov1);													// gbox
			}
			if(emu == 1)
				sscanf(buffer, "source: %08s", source1);													// mgcamd
			if(emu == 3){
				sscanf(buffer, "caid: 0x%4s", caid1);														// oscam
				sscanf(buffer, "pid: 0x%4s", pid1);														// oscam
				sscanf(buffer, "from: %29s", source1);														// oscam
				sscanf(buffer, "prov: 0x%6s", prov1);														// oscam
				sscanf(buffer, "ecm time: %9s",response1);													// oscam

				sscanf(buffer, "reader: %18s", reader);													// oscam
				sscanf(buffer, "protocol: %18s", protocol);													// oscam
			}
			sscanf(buffer, "%c%c0: %02s %02s %02s %02s %02s %02s %02s %02s",&tmp,&tmp, cw0_[0], cw0_[1], cw0_[2], cw0_[3], cw0_[4], cw0_[5], cw0_[6], cw0_[7]);	// gbox, mgcamd oscam
			sscanf(buffer, "%c%c1: %02s %02s %02s %02s %02s %02s %02s %02s",&tmp,&tmp, cw1_[0], cw1_[1], cw1_[2], cw1_[3], cw1_[4], cw1_[5], cw1_[6], cw1_[7]);	// gbox, mgcamd oscam
		}
		fclose (ecminfo);
		if (buffer)
			free (buffer);
		if(ecmInfoEmpty)
			return;

		if(emu == 3){
			std::string kname = source1;
			size_t pos1 = kname.find_last_of("/")+1;
			size_t pos2 = kname.find_last_of(".");
			if(pos2>pos1)
				kname=kname.substr(pos1, pos2-pos1);
			snprintf(source1,sizeof(source1),"%s",kname.c_str());
		}
		if(emu == 2 && response1[0] != 0){
			char tmp_res[10] = "";
			memcpy(tmp_res,response1,sizeof(tmp_res));
			if(response1[3] != 0){
				snprintf(response1,sizeof(response1),"%c.%s",tmp_res[0],&tmp_res[1]);
			}else
				snprintf(response1,sizeof(response1),"0.%s",tmp_res);
		}
// 		tmp_cw0=(cw0_[0][0]<<8)+cw0_[0][1];
// 		tmp_cw1=(cw1_[0][0]<<8)+cw1_[0][1];
// 		if((tmp_cw0+tmp_cw1) != (cw0+cw1)){
// 			cw0=tmp_cw0;
// 			cw1=tmp_cw1;
// 		}
	}
	else
	{
		if (g_settings.infoviewer_ecm_info == 1)
			frameBuffer->paintBackgroundBoxRel(g_InfoViewer->BoxEndX-220, g_InfoViewer->BoxStartY-185, 225, g_InfoViewer->ChanHeight+105);
		else
			frameBuffer->paintBackgroundBoxRel(g_InfoViewer->BoxEndX-220, g_InfoViewer->BoxStartY-185, 225, g_InfoViewer->ChanHeight+105);

		return;
	}

	if (prov1[strlen(prov1)-1] == '\n')
		prov1[strlen(prov1)-1] = '\0';

	char share_at[32] = {0};
	char share_card[5] = {0};
	char share_id[5] = {0};
	int share_net = 0;

	const char *share_info = "/tmp/share.info";
	FILE* shareinfo = fopen (share_info, "r");
	if (shareinfo)
	{
		char *buffer = NULL;
		size_t len = 0;
		ssize_t read;
		while ((read = getline(&buffer, &len, shareinfo)) != -1)
		{
			sscanf(buffer, "CardID %*s at %s Card %s Sl:%*s Lev:%*s dist:%*s id:%s", share_at, share_card, share_id);
			if ((strncmp(caid1, share_card, 4) == 0) && (strncmp(prov1, share_id, 4) == 0))
			{
				share_net = 1;
				break;
			}
		}
		fclose (shareinfo);
		if (buffer)
			free (buffer);
	}
	const char *gbox_info = "<< Gbox-ECM-Info >>";
	const char *mgcamd_info = "<< Mgcamd-ECM-Info >>";
	const char *oscam_info = "<< OScam-ECM-Info >>";
	if (g_settings.infoviewer_ecm_info == 1)
	{
		if (emu == 2)
		{

			painttECMInfo(160, gbox_info, caid1, source1, response1, prov1);

			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(g_InfoViewer->BoxEndX-215, g_InfoViewer->BoxStartY-20, 80, "From:", COL_INFOBAR_TEXT, 0, true);
			if (strstr(source1, "Net" ) != NULL)
			{
				if (share_net == 1)
					g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(g_InfoViewer->BoxEndX-150, g_InfoViewer->BoxStartY-20, 130, share_at, COL_INFOBAR_TEXT, 0, true);
				else
					g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(g_InfoViewer->BoxEndX-150, g_InfoViewer->BoxStartY-20, 130, "N/A", COL_INFOBAR_TEXT, 0, true);
			}
			else
				g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(g_InfoViewer->BoxEndX-150, g_InfoViewer->BoxStartY-20, 130, "127.0.0.1", COL_INFOBAR_TEXT, 0, true);
		}
		else if ((emu == 1) || (emu == 3))
		{
			painttECMInfo((emu == 1) ?180:190,(emu == 1)? mgcamd_info:oscam_info, caid1, source1, response1, prov1);
			if(emu == 3){
				g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(g_InfoViewer->BoxEndX-215, g_InfoViewer->BoxStartY-20, 80, "Reader:", COL_INFOBAR_TEXT, 0, true);
				g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(g_InfoViewer->BoxEndX-150, g_InfoViewer->BoxStartY-20, 130, reader, COL_INFOBAR_TEXT, 0, true);
			}

		}
	}

	if (g_settings.infoviewer_ecm_info == 2)
	{
		bool gboxECM = false;
		int gboxoffset = 0,i=0;
		if (emu == 2){
			gboxECM = true;
			gboxoffset = 20;
		}
		frameBuffer->paintBoxRel(g_InfoViewer->BoxEndX-215, g_InfoViewer->BoxStartY-180, 220, g_InfoViewer->ChanHeight+80+gboxoffset, COL_INFOBAR_SHADOW_PLUS_0, g_settings.rounded_corners ? CORNER_RADIUS_MID : 0);
		frameBuffer->paintBoxRel(g_InfoViewer->BoxEndX-220, g_InfoViewer->BoxStartY-185, 220, g_InfoViewer->ChanHeight+80+gboxoffset, COL_INFOBAR_PLUS_0, g_settings.rounded_corners ? CORNER_RADIUS_MID : 0);

		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(g_InfoViewer->BoxEndX-215, g_InfoViewer->BoxStartY-140, 80, "CaID:", COL_INFOBAR_TEXT, 0, true);
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(g_InfoViewer->BoxEndX-215, g_InfoViewer->BoxStartY-120, 80, "PID:", COL_INFOBAR_TEXT, 0, true);
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(g_InfoViewer->BoxEndX-215, g_InfoViewer->BoxStartY-100, 80, "Decode:", COL_INFOBAR_TEXT, 0, true);
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(g_InfoViewer->BoxEndX-215, g_InfoViewer->BoxStartY-80, 80, "Provider:", COL_INFOBAR_TEXT, 0, true);

		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(g_InfoViewer->BoxEndX-215, g_InfoViewer->BoxStartY-60, 42, "CW0:", COL_INFOBAR_TEXT, 0, true);
		g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(g_InfoViewer->BoxEndX-215, g_InfoViewer->BoxStartY-40, 42, "CW1:", COL_INFOBAR_TEXT, 0, true);

		for(i=0;i<8;i++){
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(g_InfoViewer->BoxEndX-(173-(i*21)), g_InfoViewer->BoxStartY-60, 21, cw0_[i], COL_INFOBAR_TEXT, 0, true);
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(g_InfoViewer->BoxEndX-(173-(i*21)), g_InfoViewer->BoxStartY-40, 21, cw1_[i], COL_INFOBAR_TEXT, 0, true);
		}

		if (gboxECM)
		{
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(g_InfoViewer->BoxEndX-190, g_InfoViewer->BoxStartY-160, 160, gbox_info, COL_INFOBAR_TEXT, 0, true);
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(g_InfoViewer->BoxEndX-150, g_InfoViewer->BoxStartY-140, 80, caid1, COL_INFOBAR_TEXT, 0, true);
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(g_InfoViewer->BoxEndX-150, g_InfoViewer->BoxStartY-120, 80, pid1, COL_INFOBAR_TEXT, 0, true);
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(g_InfoViewer->BoxEndX-150, g_InfoViewer->BoxStartY-100, 70, source1, COL_INFOBAR_TEXT, 0, true);
			if(response1[0] != 0)
			{
				g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(g_InfoViewer->BoxEndX-90, g_InfoViewer->BoxStartY-100, 15, "in", COL_INFOBAR_TEXT, 0, true);
				g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(g_InfoViewer->BoxEndX-70, g_InfoViewer->BoxStartY-100, 45, response1, COL_INFOBAR_TEXT, 0, true);
				g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(g_InfoViewer->BoxEndX-25, g_InfoViewer->BoxStartY-100, 10, "s", COL_INFOBAR_TEXT, 0, true);
			}

			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(g_InfoViewer->BoxEndX-150, g_InfoViewer->BoxStartY-80, 130, prov1, COL_INFOBAR_TEXT, 0, true);
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(g_InfoViewer->BoxEndX-215, g_InfoViewer->BoxStartY-20, 50, "From:", COL_INFOBAR_TEXT, 0, true);
			if (strstr(source1, "Net") != NULL)
			{
				if (share_net == 1)
					g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(g_InfoViewer->BoxEndX-173, g_InfoViewer->BoxStartY-20, 160, share_at, COL_INFOBAR_TEXT, 0, true);
				else
					g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(g_InfoViewer->BoxEndX-173, g_InfoViewer->BoxStartY-20, 160, "N/A", COL_INFOBAR_TEXT, 0, true);
			}
			else
				g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(g_InfoViewer->BoxEndX-173, g_InfoViewer->BoxStartY-20, 160, "127.0.0.1", COL_INFOBAR_TEXT, 0, true);
		}
		else if ((emu == 1) || (emu == 3))
		{
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(g_InfoViewer->BoxEndX-200, g_InfoViewer->BoxStartY-160, 190,(emu == 1)? mgcamd_info:oscam_info, COL_INFOBAR_TEXT, 0, true);
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(g_InfoViewer->BoxEndX-135, g_InfoViewer->BoxStartY-140, 80, caid1, COL_INFOBAR_TEXT, 0, true);
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(g_InfoViewer->BoxEndX-135, g_InfoViewer->BoxStartY-120, 80, pid1, COL_INFOBAR_TEXT, 0, true);
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(g_InfoViewer->BoxEndX-135, g_InfoViewer->BoxStartY-100, 70, source1, COL_INFOBAR_TEXT, 0, true);
			//g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(g_InfoViewer->BoxEndX-70, g_InfoViewer->BoxStartY-100, 20, "", COL_INFOBAR_TEXT, 0, true);
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(g_InfoViewer->BoxEndX-55, g_InfoViewer->BoxStartY-100, 70, response1, COL_INFOBAR_TEXT, 0, true);
			//g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(g_InfoViewer->BoxEndX-25, g_InfoViewer->BoxStartY-100, 20, "", COL_INFOBAR_TEXT, 0, true);
			g_Font[SNeutrinoSettings::FONT_TYPE_INFOBAR_SMALL]->RenderString(g_InfoViewer->BoxEndX-135, g_InfoViewer->BoxStartY-80, 130, prov1, COL_INFOBAR_TEXT, 0, true);
		}
	}
 }

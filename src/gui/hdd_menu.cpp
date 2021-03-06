/*
	Neutrino-GUI  -   DBoxII-Project

	Copyright (C) 2001 Steffen Hehn 'McClean'
	Copyright (C) 2010-2015 Stefan Seyfried
	Copyright (C) 2013-2014 martii
	Copyright (C) 2009-2014 CoolStream International Ltd

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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <errno.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/swap.h>
#include <sys/vfs.h>
#include <dirent.h>
#include <dlfcn.h>
#include <sys/mount.h>
#include <sys/wait.h>

#include <global.h>
#include <neutrino.h>
#include <neutrino_menue.h>
#include "hdd_menu.h"

#include <gui/filebrowser.h>
#include <gui/widget/icons.h>
#include <gui/widget/stringinput.h>
#include <gui/widget/messagebox.h>
#include <gui/widget/hintbox.h>
#include <gui/widget/progresswindow.h>
#include <gui/widget/keyboard_input.h>

#include <system/helpers.h>
#include <system/settings.h>
#include <system/debug.h>

#include <mymenu.h>
#include <driver/screen_max.h>
#include <driver/record.h>
#include <driver/display.h>


#define e2fsckBinary   "/sbin/e2fsck"
#define ext3FsckBinary "/sbin/fsck.ext3"
#define ext4FsckBinary "/sbin/fsck.ext4"
#define ext2FsckBinary "/sbin/fsck.ext2"
#define jfsFsckBinary  "/sbin/fsck.jfs"
#define ext3MkfsBinary "/sbin/mkfs.ext3"
#define ext4MkfsBinary "/sbin/mkfs.ext4"
#define ext2MkfsBinary "/sbin/mkfs.ext2"
#define jfsMkfsBinary  "/sbin/mkfs.jfs"
#define blkidBinary    "/sbin/blkid"


#define HDD_NOISE_OPTION_COUNT 4
const CMenuOptionChooser::keyval HDD_NOISE_OPTIONS[HDD_NOISE_OPTION_COUNT] =
{
	{ 0,   LOCALE_OPTIONS_OFF },
	{ 128, LOCALE_HDD_SLOW },
	{ 190, LOCALE_HDD_MIDDLE },
	{ 254, LOCALE_HDD_FAST }
};

#define HDD_FILESYS_OPTION_COUNT 4
const CMenuOptionChooser::keyval_ext HDD_FILESYS_OPTIONS[HDD_FILESYS_OPTION_COUNT] =
{
	{ fs_ext3, NONEXISTANT_LOCALE, "ext3" },
	{ fs_ext4, NONEXISTANT_LOCALE, "ext4" },
	{ fs_ext2, NONEXISTANT_LOCALE, "ext2" },
	{ fs_jfs, NONEXISTANT_LOCALE, "jfs" }
};

#define HDD_SLEEP_OPTION_COUNT 6
const CMenuOptionChooser::keyval HDD_SLEEP_OPTIONS[HDD_SLEEP_OPTION_COUNT] =
{
	{ 0,   LOCALE_OPTIONS_OFF },
	//{ 12,  LOCALE_HDD_1MIN },
	{ 60,  LOCALE_HDD_5MIN },
	{ 120, LOCALE_HDD_10MIN },
	{ 240, LOCALE_HDD_20MIN },
	{ 241, LOCALE_HDD_30MIN },
	{ 242, LOCALE_HDD_60MIN }
};

static int my_filter(const struct dirent *d)
{
	if ((d->d_name[0] == 's' || d->d_name[0] == 'h') && d->d_name[1] == 'd')
		return 1;
	return 0;
}

std::string getFmtType(const char* name, int num)
{
	pid_t pid;
	std::string ret = "";
	std::string blkid = find_executable("blkid");
	if (blkid.empty())
		return ret;
	std::string pcmd = blkidBinary + (std::string)" -s TYPE /dev/" + (std::string)name + to_string(num);
	dprintf(DEBUG_INFO, ">>>>>[%s #%d] pcmd: %s\n", __func__, __LINE__, pcmd.c_str());
	FILE* f = my_popen(pid, pcmd.c_str(), "r");
	if (f != NULL) {
		char buff[512];
		if (!fgets(buff, sizeof(buff), f))
			buff[0] = '\0';
		fclose(f);
		ret = buff;
		std::string search = "TYPE=\"";
		size_t pos = ret.find(search);
		if (pos == std::string::npos)
			return "";
		ret = ret.substr(pos + search.length());
		pos = ret.find("\"");
		if (pos != std::string::npos)
			ret = ret.substr(0, pos);
	}
	return ret;
}

CHDDMenuHandler::CHDDMenuHandler()
{
	width = 58;
}

CHDDMenuHandler::~CHDDMenuHandler()
{

}

static bool is_mounted(const char *dev)
{
	bool res = false;
	char devpath[40];
	snprintf(devpath, sizeof(devpath), "/dev/%s", dev);
	int devpathlen = strlen(devpath);
	char buffer[255];
	FILE *f = fopen("/proc/mounts", "r");
	if(f) {
		while (!res && fgets (buffer, sizeof(buffer), f))
			if (!strncmp(buffer, devpath, devpathlen)
			 && (buffer[devpathlen] == ' ' || buffer[devpathlen] == '\t'))
				res = true;
		fclose(f);
	}
	return res;
}

int CHDDMenuHandler::exec(CMenuTarget* parent, const std::string &actionkey)
{
	if (parent)
		parent->hide();
	for (std::vector<hdd_s>::iterator it = hdd_list.begin(); it != hdd_list.end(); ++it)
		if (it->devname == actionkey) {
			std::string cmd = "ACTION=" + std::string((it->mounted ? "remove" : "add"))
			+ " MOUNTBASE=/tmp MDEV=" + it->devname + " /etc/mdev/mdev-mount.sh";
			system(cmd.c_str());
			it->mounted = is_mounted(it->devname.c_str());
                	it->cmf->setOption(g_Locale->getText(it->mounted ? LOCALE_HDD_UMOUNT : LOCALE_HDD_MOUNT));
			return menu_return::RETURN_REPAINT;
		}

	return doMenu();
}

int CHDDMenuHandler::doMenu()
{
	FILE * f;
	int fd;
	struct dirent **namelist;
	int ret;
	struct stat s;
	int root_dev = -1;

	bool ext4MkfsBinaryExist = !find_executable("mkfs.ext4").empty();
	bool blkidBinaryExist    = !find_executable("blkid").empty();

	bool hdd_found = 0;
	int n = scandir("/sys/block", &namelist, my_filter, alphasort);

	if (n < 0) {
		perror("CHDDMenuHandler::doMenu: scandir(\"/sys/block\") failed");
		return menu_return::RETURN_REPAINT;
	}


	CMenuWidget* hddmenu = new CMenuWidget(LOCALE_MAINMENU_SETTINGS, NEUTRINO_ICON_SETTINGS, width, MN_WIDGET_ID_DRIVESETUP);

	//if no drives found, select 'back'
	if (hdd_found == 0 && hddmenu->getSelected() != -1)
		hddmenu->setSelected(2);

	hddmenu->addIntroItems(LOCALE_HDD_SETTINGS);
	CHDDFmtExec fmtexec;
	CHDDChkExec chkexec;

	CHDDDestExec hddexec;
	CMenuForwarder * mf = new CMenuForwarder(LOCALE_HDD_ACTIVATE, true, NULL, &hddexec, NULL, CRCInput::RC_red);
	mf->setHint("", LOCALE_MENU_HINT_HDD_APPLY);
	hddmenu->addItem(mf);

	hddmenu->addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_HDD_EXTENDED_SETTINGS));

	CMenuOptionChooser * mc = new CMenuOptionChooser(LOCALE_HDD_SLEEP, &g_settings.hdd_sleep, HDD_SLEEP_OPTIONS, HDD_SLEEP_OPTION_COUNT, true);
	mc->setHint("", LOCALE_MENU_HINT_HDD_SLEEP);
	hddmenu->addItem(mc);

	const char hdparm[] = "/sbin/hdparm";
	struct stat stat_buf;
	bool have_nonbb_hdparm = !::lstat(hdparm, &stat_buf) && !S_ISLNK(stat_buf.st_mode);
	if (have_nonbb_hdparm) {
		mc = new CMenuOptionChooser(LOCALE_HDD_NOISE, &g_settings.hdd_noise, HDD_NOISE_OPTIONS, HDD_NOISE_OPTION_COUNT, true);
		mc->setHint("", LOCALE_MENU_HINT_HDD_NOISE);
		hddmenu->addItem(mc);
	}

	//if(n > 0)
	hddmenu->addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_HDD_MANAGE));

#if HAVE_SPARK_HARDWARE || HAVE_DUCKBOX_HARDWARE
	hdd_list.clear();
#endif
	ret = stat("/", &s);
	int drive_mask = 0xfff0;
	if (ret != -1) {
		if ((s.st_dev & drive_mask) == 0x0300) /* hda, hdb,... has max 63 partitions */
			drive_mask = 0xffc0; /* hda: 0x0300, hdb: 0x0340, sda: 0x0800, sdb: 0x0810 */
		root_dev = (s.st_dev & drive_mask);
	}
	printf("HDD: root_dev: 0x%04x\n", root_dev);
	std::string tmp_str[n];
	CMenuWidget * tempMenu[n];
	for(int i = 0; i < n;i++) {
		tempMenu[i] = NULL;
		char str[256];
		char sstr[256];
		char vendor[128], model[128];
		int64_t bytes;
		int64_t megabytes;
		int removable = 0;
		bool oldkernel = false;
		bool isroot = false;

		printf("HDD: checking /sys/block/%s\n", namelist[i]->d_name);
		snprintf(str, sizeof(str), "/dev/%s", namelist[i]->d_name);
		fd = open(str, O_RDONLY);
		if(fd < 0) {
			printf("Cant open %s\n", str);
			continue;
		}
		if (ioctl(fd, BLKGETSIZE64, &bytes))
			perror("BLKGETSIZE64");

		ret = fstat(fd, &s);
		if (ret != -1) {
			if ((int)(s.st_rdev & drive_mask) == root_dev) {
				isroot = true;
				/* dev_t is different sized on different architectures :-( */
				printf("-> root device is on this disk 0x%04x, skipping\n", (int)s.st_rdev);
			}
		}
		close(fd);

		megabytes = bytes/1000000;

		snprintf(str, sizeof(str), "/sys/block/%s/device/vendor", namelist[i]->d_name);
		f = fopen(str, "r");
		if(!f) {
			oldkernel = true;
			strcpy(vendor, "");
		} else {
			fscanf(f, "%s", vendor);
			fclose(f);
			strcat(vendor, "-");
		}

		if (oldkernel)
			snprintf(str, sizeof(str), "/proc/ide/%s/model", namelist[i]->d_name);
		else
			snprintf(str, sizeof(str), "/sys/block/%s/device/model", namelist[i]->d_name);
		f = fopen(str, "r");
		if(!f) {
			printf("Cant open %s\n", str);
			continue;
		}
		fscanf(f, "%s", model);
		fclose(f);

		snprintf(str, sizeof(str), "/sys/block/%s/removable", namelist[i]->d_name);
		f = fopen(str, "r");
		if(!f) {
			printf("Cant open %s\n", str);
			continue;
		}
		fscanf(f, "%d", &removable);
		removable = 0;
		fclose(f);

		bool enabled = !CNeutrinoApp::getInstance()->recordingstatus && !removable && !isroot;

		std::string fmt_type = "";
		if (blkidBinaryExist)
			fmt_type = getFmtType(namelist[i]->d_name, 1);
		std::string tmpType = (fmt_type == "") ? "" : " (" + fmt_type + (std::string)")";

		snprintf(str, sizeof(str), "%s %s %ld %s%s", vendor, model, (long)(megabytes < 10000 ? megabytes : megabytes/1000), megabytes < 10000 ? "MB" : "GB", tmpType.c_str());
		printf("HDD: %s\n", str);
		tmp_str[i]=str;
		tempMenu[i] = new CMenuWidget(str, NEUTRINO_ICON_SETTINGS);
		tempMenu[i]->addIntroItems();
		if (fmt_type == "ext3")
			g_settings.hdd_fs = fs_ext3;
		else if (fmt_type == "ext4")
			g_settings.hdd_fs = fs_ext4;
		else if (fmt_type == "ext2")
			g_settings.hdd_fs = fs_ext2;
		else if (fmt_type == "jfs")
			g_settings.hdd_fs = fs_jfs;
		else
			g_settings.hdd_fs = fs_ext3;
		mc = new CMenuOptionChooser(LOCALE_HDD_FS, &g_settings.hdd_fs, HDD_FILESYS_OPTIONS, HDD_FILESYS_OPTION_COUNT, true);
		mc->setHint("", LOCALE_MENU_HINT_HDD_FMT);
		tempMenu[i]->addItem(mc);

		mf = new CMenuForwarder(LOCALE_HDD_FORMAT, true, "", &fmtexec, namelist[i]->d_name);
		mf->setHint("", LOCALE_MENU_HINT_HDD_FORMAT);
		tempMenu[i]->addItem(mf);

		mf = new CMenuForwarder(LOCALE_HDD_CHECK, true, "", &chkexec, namelist[i]->d_name);
		mf->setHint("", LOCALE_MENU_HINT_HDD_CHECK);
		tempMenu[i]->addItem(mf);

		snprintf(sstr, sizeof(sstr), "%s (%s)", g_Locale->getText(LOCALE_HDD_REMOVABLE_DEVICE),  namelist[i]->d_name);
		mf = new CMenuForwarder((removable ? sstr : namelist[i]->d_name), enabled, tmp_str[i], tempMenu[i]);
		mf->setHint("", LOCALE_MENU_HINT_HDD_TOOLS);
		hddmenu->addItem(mf);

		hdd_found = 1;
		free(namelist[i]);
	}
	if (n >= 0)
		free(namelist);

	if(!hdd_found)
		hddmenu->addItem(new CMenuForwarder(LOCALE_HDD_NOT_FOUND, false));
	else {
		hdd_list.clear();
		FILE *b = popen("/sbin/blkid", "r");
		if (b) {
			char buf[255];
			while (fgets (buf, sizeof(buf), b)) {
				if (strncmp(buf, "/dev/sd", 7) && strncmp(buf, "/dev/hd", 7))
					continue;
				char *e = strstr(buf + 7, ":");
				if (!e)
					continue;
				*e = 0;
				hdd_s hdd;
				hdd.devname = std::string(buf + 5);
				hdd.mounted = is_mounted(buf + 5);
				hdd_list.push_back(hdd);
			}
			fclose(b);
		}
		if (!hdd_list.empty()) {
			int shortcut = 1;
			hddmenu->addItem(new CMenuSeparator(CMenuSeparator::LINE | CMenuSeparator::STRING, LOCALE_HDD_MOUNT_UMOUNT));
			for (std::vector<hdd_s>::iterator it = hdd_list.begin(); it != hdd_list.end(); ++it) {
				std::string tmp(g_Locale->getText(it->mounted ?  LOCALE_HDD_UMOUNT : LOCALE_HDD_MOUNT));
				it->cmf = new CMenuForwarder(it->devname, true, tmp.c_str(), this,
					it->devname.c_str(), CRCInput::convertDigitToKey(shortcut++));
				hddmenu->addItem(it->cmf);
			}
		}
	}

	ret = hddmenu->exec(NULL, "");
	for(int i = 0; i < n;i++) {
		if( hdd_found && tempMenu[i] != NULL ){
			delete tempMenu[i];
		}
	}

	hddmenu=NULL;
	delete hddmenu;
	return ret;
}

void CHDDMenuHandler::showError(neutrino_locale_t err)
{
	ShowMsg(LOCALE_MESSAGEBOX_ERROR, g_Locale->getText(err), CMessageBox::mbrOk, CMessageBox::mbOk);
}

int CHDDDestExec::exec(CMenuTarget* /*parent*/, const std::string&)
{
	char str[256];
	FILE * f;
	int removable = 0;
	struct dirent **namelist;
	int n = scandir("/sys/block", &namelist, my_filter, alphasort);

	if (n < 0)
		return menu_return::RETURN_NONE;

	const char hdidle[] = "/sbin/hd-idle";
	bool have_hdidle = !access(hdidle, X_OK);

	if (g_settings.hdd_sleep > 0 && g_settings.hdd_sleep < 60)
		g_settings.hdd_sleep = 60;

	if (have_hdidle) {
		system("kill $(pidof hd-idle)");
		int sleep_seconds = g_settings.hdd_sleep;
		switch (sleep_seconds) {
			case 241:
					sleep_seconds = 30 * 60;
					break;
			case 242:
					sleep_seconds = 60 * 60;
					break;
			default:
					sleep_seconds *= 5;
		}
		if (sleep_seconds)
			my_system(3, hdidle, "-i", to_string(sleep_seconds).c_str());
	}

	const char hdparm[] = "/sbin/hdparm";
	bool have_hdparm = !access(hdparm, X_OK);
	if (!have_hdparm)
		return menu_return::RETURN_NONE;

	struct stat stat_buf;
	bool have_nonbb_hdparm = !::lstat(hdparm, &stat_buf) && !S_ISLNK(stat_buf.st_mode);

	for (int i = 0; i < n; i++) {
		removable = 0;
		printf("CHDDDestExec: checking /sys/block/%s\n", namelist[i]->d_name);

		sprintf(str, "/sys/block/%s/removable", namelist[i]->d_name);
		f = fopen(str, "r");
		if (!f) {
			printf("Can't open %s\n", str);
			continue;
		}
		fscanf(f, "%d", &removable);
		fclose(f);

		if (removable) {
			printf("CHDDDestExec: /dev/%s is not a hdd, no sleep needed\n", namelist[i]->d_name);
		} else {
		printf("CHDDDestExec: noise %d sleep %d /dev/%s\n",
			 g_settings.hdd_noise, g_settings.hdd_sleep, namelist[i]->d_name);
		char M_opt[50],S_opt[50], opt[100];
		snprintf(S_opt, sizeof(S_opt), "-S%d", g_settings.hdd_sleep);
		snprintf(M_opt, sizeof(M_opt), "-M%d", g_settings.hdd_noise);
		snprintf(opt, sizeof(opt), "/dev/%s",namelist[i]->d_name);

		if (have_hdidle)
			my_system(3, hdparm, M_opt, opt);
		else if (have_nonbb_hdparm)
			my_system(4, hdparm, M_opt, S_opt, opt);
		else // busybox hdparm doesn't support "-M"
			my_system(3, hdparm, S_opt, opt);
		}
		free(namelist[i]);
	}
	free(namelist);
	return menu_return::RETURN_NONE;
}

static int dev_umount(char *dev)
{
	char buffer[255];
	FILE *f = fopen("/proc/mounts", "r");
	if(f == NULL)
		return -1;
	while (fgets (buffer, 255, f) != NULL) {
		char *p = buffer + strlen(dev);
		if (strstr(buffer, dev) == buffer && *p == ' ') {
			p++;
			char *q = strchr(p, ' ');
			if (q == NULL)
				continue;
			*q = 0x0;
			fclose(f);
			printf("dev_umount %s: umounting %s\n", dev, p);
			return umount(p);
		}
	}
#ifndef ASSUME_MDEV
	/* with mdev, we hopefully don't have to umount anything here... */
	printf("dev_umount %s: not found\n", dev);
#endif
	errno = ENOENT;
	fclose(f);
	return -1;
}

/* unmounts all partitions of a given block device, dev can be /dev/sda, sda or sda4 */
static int umount_all(const char *dev)
{
	char buffer[255];
	int i;
	char *d = strdupa(dev);
	char *p = d + strlen(d) - 1;
	while (isdigit(*p))
		p--;
	*++p = 0x0;
	if (strstr(d, "/dev/") == d)
		d += strlen("/dev/");
	printf("HDD: %s dev = '%s' d = '%s'\n", __func__, dev, d);
	for (i = 1; i < 16; i++)
	{
		sprintf(buffer, "/dev/%s%d", d, i);
		// printf("checking for '%s'\n", buffer);
		if (access(buffer, R_OK))
			continue;	/* device does not exist? */
#ifdef ASSUME_MDEV
		/* we can't use a 'remove' uevent, as that would also remove the device node
		 * which we certainly need for formatting :-) */
		if (! access("/etc/mdev/mdev-mount.sh", X_OK)) {
			sprintf(buffer, "MDEV=%s%d ACTION=remove /etc/mdev/mdev-mount.sh block", d, i);
			printf("-> running '%s'\n", buffer);
			my_system(3, "/bin/sh", "-c", buffer);
		}
#endif
		sprintf(buffer, "/dev/%s%d", d, i);
		/* just to make sure */
		swapoff(buffer);
		if (dev_umount(buffer) && errno != ENOENT)
			fprintf(stderr, "could not umount %s: %m\n", buffer);
	}
	return 0;
}

/* triggers a uevent for all partitions of a given blockdev, dev can be /dev/sda, sda or sda4 */
static int mount_all(const char *dev)
{
	int i, ret = -1;
	char *d = strdupa(dev);
	char *p = d + strlen(d) - 1;
	while (isdigit(*p))
		p--;
	if (strstr(d, "/dev/") == d)
		d += strlen("/dev/");
	*++p = 0x0;
	printf("HDD: %s dev = '%s' d = '%s'\n", __func__, dev, d);
	for (i = 1; i < 16; i++)
	{
#ifdef ASSUME_MDEV
		char buffer[255];
		sprintf(buffer, "/sys/block/%s/%s%d/uevent", d, d, i);
		if (!access(buffer, W_OK)) {
			FILE *f = fopen(buffer, "w");
			if (!f)
				fprintf(stderr, "HDD: %s could not open %s: %m\n", __func__, buffer);
			else {
				printf("-> triggering add uevent in %s\n", buffer);
				fprintf(f, "add\n");
				fclose(f);
				ret = 0;
			}
		}
#endif
	}
	return ret;
}

#ifdef ASSUME_MDEV
static void waitfordev(const char *src, int maxwait)
{
	int waitcount = 0;
	/* wait for the device to show up... */
	while (access(src, W_OK)) {
		if (!waitcount)
			printf("CHDDFmtExec: waiting for %s", src);
		else
			printf(".");
		fflush(stdout);
		waitcount++;
		if (waitcount > maxwait) {
			fprintf(stderr, "CHDDFmtExec: device %s did not appear!\n", src);
			break;
		}
		sleep(1);
	}
	if (waitcount && waitcount <= maxwait)
		printf("\n");
}
#else
static void waitfordev(const char *, int)
{
}
#endif

#if 0
static int check_and_umount(char * dev, char * path)
{
	char buffer[255];
	FILE *f = fopen("/proc/mounts", "r");
	if(f == NULL)
		return -1;
	while (fgets (buffer, 255, f) != NULL) {
		if(strstr(buffer, dev)) {
			printf("HDD: %s mounted\n", path);
			fclose(f);
			return umount(path);
		}
	}
	fclose(f);
	printf("HDD: %s not mounted\n", path);
	return 0;
}
#endif

int CHDDFmtExec::exec(CMenuTarget* /*parent*/, const std::string& key)
{
	char cmd[100];
	char cmd2[100];
	CHintBox * hintbox;
	int res;
	FILE * f;
	char src[128], dst[128];
	CProgressWindow * progress;

	snprintf(src, sizeof(src), "/dev/%s1", key.c_str());
	snprintf(dst, sizeof(dst), "/media/%s1", key.c_str());

	printf("CHDDFmtExec: key %s\n", key.c_str());

	res = ShowMsg(LOCALE_HDD_FORMAT, g_Locale->getText(LOCALE_HDD_FORMAT_WARN), CMessageBox::mbrNo, CMessageBox::mbYes | CMessageBox::mbNo );
	if(res != CMessageBox::mbrYes)
		return menu_return::RETURN_REPAINT;

	bool srun = my_system(3, "killall", "-9", "smbd");

	res = umount_all(key.c_str());
	printf("CHDDFmtExec: umount res %d\n", res);

	if(res) {
		hintbox = new CHintBox(LOCALE_HDD_FORMAT, g_Locale->getText(LOCALE_HDD_UMOUNT_WARN));
		hintbox->paint();
		sleep(2);
		delete hintbox;
		goto _return;
	}

#ifndef ASSUME_MDEV
	f = fopen("/proc/sys/kernel/hotplug", "w");
	if(f) {
		fprintf(f, "none\n");
		fclose(f);
	}
#else
	creat("/tmp/.nomdevmount", 00660);
#endif

	progress = new CProgressWindow();
	progress->setTitle(LOCALE_HDD_FORMAT);
	progress->exec(NULL,"");
	progress->showStatusMessageUTF("Executing fdisk");
	progress->showGlobalStatus(0);

	if (access("/sbin/sfdisk", X_OK) == 0) {
		snprintf(cmd, sizeof(cmd), "/sbin/sfdisk -f -uM /dev/%s", key.c_str());
		strcpy(cmd2, "0,\n;\n;\n;\ny\n");
	} else {
		snprintf(cmd, sizeof(cmd), "/sbin/fdisk -u /dev/%s", key.c_str());
		strcpy(cmd2, "o\nn\np\n1\n2048\n\nw\n");
	}
#ifdef ASSUME_MDEV
	/* mdev will create it and waitfordev will wait for it... */
	unlink(src);
#endif
	printf("CHDDFmtExec: executing %s\n", cmd);
	f=popen(cmd, "w");
	if (!f) {
		hintbox = new CHintBox(LOCALE_HDD_FORMAT, g_Locale->getText(LOCALE_HDD_FORMAT_FAILED));
		hintbox->paint();
		sleep(2);
		delete hintbox;
		goto _remount;
	}

	fprintf(f, "%s", cmd2);
	pclose(f);
	//sleep(1);

	switch(g_settings.hdd_fs) {
		case fs_ext3:
			snprintf(cmd, sizeof(cmd), "%s -L RECORD -T largefile -m0 %s", ext3MkfsBinary, src);
			break;
		case fs_ext4:
			snprintf(cmd, sizeof(cmd), "%s -L RECORD -T largefile -m0 %s", ext4MkfsBinary, src);
			break;
		case fs_ext2:
			snprintf(cmd, sizeof(cmd), "%s -L RECORD -T largefile -m0 %s", ext2MkfsBinary, src);
			break;
		case fs_jfs:
			snprintf(cmd, sizeof(cmd), "%s -L RECORD -q %s", jfsMkfsBinary, src);
			break;
		default:
			return 0;
	}

	waitfordev(src, 30);

	printf("CHDDFmtExec: executing %s\n", cmd);

	f=popen(cmd, "r");
	if (!f) {
		hintbox = new CHintBox(LOCALE_HDD_FORMAT, g_Locale->getText(LOCALE_HDD_FORMAT_FAILED));
		hintbox->paint();
		sleep(2);
		delete hintbox;
		goto _remount;
	}

	char buf[256];
	setbuf(f, NULL);
	int n, t, in, pos, stage;
	pos = 0;
	stage = 0;
	while (true)
	{
		in = fgetc(f);
		if (in == EOF)
			break;

		buf[pos] = (char)in;
		pos++;
		buf[pos] = 0;
		if (in == '\b' || in == '\n')
			pos = 0; /* start a new line */
		//printf("%s", buf);
		switch (stage) {
			case 0:
				if (strcmp(buf, "Writing inode tables:") == 0) {
					stage++;
					progress->showGlobalStatus(20);
					progress->showStatusMessageUTF(buf);
				}
				break;
			case 1:
				if (in == '\b' && sscanf(buf, "%d/%d\b", &n, &t) == 2) {
					if (t == 0)
						t = 1;
					int percent = 100 * n / t;
					progress->showLocalStatus(percent);
					progress->showGlobalStatus(20 + percent / 5);
				}
				if (strstr(buf, "done")) {
					stage++;
					pos = 0;
				}
				break;
			case 2:
				if (strstr(buf, "blocks):") && sscanf(buf, "Creating journal (%d blocks):", &n) == 1) {
					progress->showLocalStatus(0);
					progress->showGlobalStatus(60);
					progress->showStatusMessageUTF(buf);
					pos = 0;
				}
				if (strstr(buf, "done")) {
					stage++;
					pos = 0;
				}
				break;
			case 3:
				if (strcmp(buf, "Writing superblocks and filesystem accounting information:") == 0) {
					progress->showGlobalStatus(80);
					progress->showStatusMessageUTF(buf);
					pos = 0;
				}
				break;
			default:
				// printf("unknown stage! %d \n\t", stage);
				break;
		}
	}
	progress->showLocalStatus(100);
	pclose(f);
	progress->showGlobalStatus(100);
	sleep(2);

	waitfordev(src, 30); /* mdev can somtimes takes long to create devices, especially after mkfs? */

	if (g_settings.hdd_fs != 3) {
	printf("CHDDFmtExec: executing %s %s\n","/sbin/tune2fs -r 0 -c 0 -i 0", src);
	my_system(8, "/sbin/tune2fs", "-r", "0", "-c", "0", "-i", "0", src);
	}

_remount:
	unlink("/tmp/.nomdevmount");
	progress->hide();
	delete progress;

	if ((res = mount_all(key.c_str())))
	{
        switch(g_settings.hdd_fs) {
			case fs_ext3:
				safe_mkdir(dst);
				res = mount(src, dst, "ext3", 0, NULL);
				break;
			case fs_ext4:
				safe_mkdir(dst);
				res = mount(src, dst, "ext4", 0, NULL);
				break;
			case fs_ext2:
				safe_mkdir(dst);
				res = mount(src, dst, "ext2", 0, NULL);
				break;
			case fs_jfs:
				safe_mkdir(dst);
				res = mount(src, dst, "jfs", 0, NULL);
				break;
			default:
				break;
		}
	}
#ifndef ASSUME_MDEV
	f = fopen("/proc/sys/kernel/hotplug", "w");
	if(f) {
#ifdef ASSUME_MDEV
		fprintf(f, "/sbin/mdev\n");
#else
		fprintf(f, "/sbin/hotplug\n");
#endif
		fclose(f);
	}
#else
	/* mounting is asynchronous via mdev, so wait for the directory to appear */
	for (int i = 0; i < 20; i++) {
		struct statfs s;
		if (::statfs(dst, &s) == 0) {
			if (s.f_type == 0xEF53)		/* EXT3_SUPER_MAGIC */
				break;
			if (s.f_type == 0x52654973)	/* REISERFS_SUPER_MAGIC */
				break;
		}
		if (i == 0)
			printf("CHDDFmtExec: waiting for %s to be mounted\n", dst);
		sleep(1);
	}
#endif

	if(!res) {
		snprintf(cmd, sizeof(cmd), "%s/timeshift", dst);
		safe_mkdir((char *) cmd);
		snprintf(cmd, sizeof(cmd), "%s/movies", dst);
		safe_mkdir(cmd);
		snprintf(cmd, sizeof(cmd), "%s/pictures", dst);
		safe_mkdir(cmd);
		snprintf(cmd, sizeof(cmd), "%s/epg", dst);
		safe_mkdir(cmd);
		snprintf(cmd, sizeof(cmd), "%s/music", dst);
		safe_mkdir(cmd);
		snprintf(cmd, sizeof(cmd), "%s/logos", dst);
		safe_mkdir(cmd);
		snprintf(cmd, sizeof(cmd), "%s/plugins", dst);
		safe_mkdir(cmd);
		sync();
#if HAVE_TRIPLEDRAGON
		/* on the tripledragon, we mount via fstab, so we need to add an
		   fstab entry for dst */
		FILE *g;
		char *line = NULL;
		unlink("/etc/fstab.new");
		g = fopen("/etc/fstab.new", "w");
		f = fopen("/etc/fstab", "r");
		if (!g)
			perror("open /etc/fstab.new");
		else {
			if (f) {
				int ret;
				while (true) {
					size_t dummy;
					ret = getline(&line, &dummy, f);
					if (ret < 0)
						break;
					/* remove lines that start with the same disk we formatted
					   src is "/dev/xda1", we only compare "/dev/xda" */
					if (strncmp(line, src, strlen(src)-1) != 0)
						fprintf(g, "%s", line);
				}
				free(line);
				fclose(f);
			}
			/* now add our new entry */
			fprintf(g, "%s %s auto defaults 0 0\n", src, dst);
			fclose(g);
			rename("/etc/fstab", "/etc/fstab.old");
			rename("/etc/fstab.new", "/etc/fstab");
		}
#endif
	}
_return:
	if (!srun) my_system(1, "smbd");
	return menu_return::RETURN_REPAINT;
}

int CHDDChkExec::exec(CMenuTarget* /*parent*/, const std::string& key)
{
	char cmd[100];
	CHintBox * hintbox;
	int res;
	char src[128], dst[128];
	FILE * f;
	CProgressWindow * progress;
	int oldpass = 0, pass, step, total;
	int percent = 0, opercent = 0;

	bool ext4FsckBinaryExist = (!access(ext4FsckBinary, X_OK));
	bool ext2FsckBinaryExist = (!access(ext2FsckBinary, X_OK));
	bool jfsFsckBinaryExist = (!access(jfsFsckBinary, X_OK));
	bool e2fsckBinaryExist   = (!access(e2fsckBinary, X_OK));
	bool blkidBinaryExist    = (!access(blkidBinary, X_OK));

	if (blkidBinaryExist) {
		std::string fmt_type = getFmtType(key.c_str(), 1);
		if (((fmt_type != "ext2") && (fmt_type != "ext3") && (fmt_type != "ext4")) || 
			((fmt_type == "ext4") && (!ext4FsckBinaryExist) && (!e2fsckBinaryExist))) {

			char msg1[512], msg2[512];
			if (fmt_type.empty())
				fmt_type = g_Locale->getText(LOCALE_HDD_FS_UNKNOWN);
			snprintf(msg1, sizeof(msg1)-1, "%s", g_Locale->getText(LOCALE_HDD_CHECK_FORMAT_BAD));
			snprintf(msg2, sizeof(msg2)-1, msg1, fmt_type.c_str());
			hintbox = new CHintBox(LOCALE_HDD_CHECK, msg2);
			hintbox->paint();
			sleep(3);
			delete hintbox;
			return menu_return::RETURN_REPAINT;
		}
	}

	snprintf(src, sizeof(src), "/dev/%s1", key.c_str());
	snprintf(dst, sizeof(dst), "/media/%s1", key.c_str());

printf("CHDDChkExec: key %s\n", key.c_str());

	bool srun = my_system(3, "killall", "-9", "smbd");

	//res = check_and_umount(dst);
	res = umount_all(key.c_str());
	printf("CHDDChkExec: umount res %d\n", res);
	if(res) {
		hintbox = new CHintBox(LOCALE_HDD_CHECK, g_Locale->getText(LOCALE_HDD_UMOUNT_WARN));
		hintbox->paint();
		sleep(2);
		delete hintbox;
		return menu_return::RETURN_REPAINT;
	}

	if (e2fsckBinaryExist) {
		snprintf(cmd, sizeof(cmd), "%s -C 1 -f -y %s", e2fsckBinary, src);
	} else {
		snprintf(cmd, sizeof(cmd), "%s -C 1 -f -y %s", ext3FsckBinary, src);
		if ((ext4FsckBinaryExist) && (g_settings.hdd_fs == fs_ext4))
			snprintf(cmd, sizeof(cmd), "%s -C 1 -f -y %s", ext4FsckBinary, src);
		else if ((ext2FsckBinaryExist) && (g_settings.hdd_fs == fs_ext2))
			snprintf(cmd, sizeof(cmd), "%s -C 1 -f -y %s", ext2FsckBinary, src);
		else if ((jfsFsckBinaryExist) && (g_settings.hdd_fs == fs_jfs))
			snprintf(cmd, sizeof(cmd), "%s -a -f -p %s", jfsFsckBinary, src);

#if 0
	switch(g_settings.hdd_fs) {
			case fs_ext3:
				snprintf(cmd, sizeof(cmd), "%s -C 1 -f -y %s", ext3FsckBinary, src);
			break;
			case fs_ext4:
				snprintf(cmd, sizeof(cmd), "%s -C 1 -f -y %s", ext4FsckBinary, src);
			break;
			case fs_ext2:
				snprintf(cmd, sizeof(cmd), "%s -C 1 -f -y %s", ext2FsckBinary, src);
			break;
			case fs_jfs:
				snprintf(cmd, sizeof(cmd), "%s -a -f -p %s", jfsFsckBinary, src);
			break;
		default:
			return 0;
		}
#endif
	}

	printf("CHDDChkExec: Executing %s\n", cmd);
	f=popen(cmd, "r");
	if(!f) {
		hintbox = new CHintBox(LOCALE_HDD_CHECK, g_Locale->getText(LOCALE_HDD_CHECK_FAILED));
		hintbox->paint();
		sleep(2);
		delete hintbox;
		goto ret1;
	}

	progress = new CProgressWindow();
	progress->setTitle(LOCALE_HDD_CHECK);
	progress->exec(NULL,"");

	char buf[256];
	while(fgets(buf, 255, f) != NULL)
	{
		if(isdigit(buf[0])) {
			sscanf(buf, "%d %d %d\n", &pass, &step, &total);
			if(total == 0)
				total = 1;
			if(oldpass != pass) {
				oldpass = pass;
				progress->showGlobalStatus(pass > 0 ? (pass-1)*20: 0);
			}
			percent = (step * 100) / total;
			if(opercent != percent) {
				opercent = percent;
//printf("CHDDChkExec: pass %d : %d\n", pass, percent);
				progress->showLocalStatus(percent);
			}
		}
		else {
			char *t = strrchr(buf, '\n');
			if (t)
				*t = 0;
			if(!strncmp(buf, "Pass", 4)) {
				progress->showStatusMessageUTF(buf);
			}
		}
	}
//printf("CHDDChkExec: %s\n", buf);
	pclose(f);
	progress->showGlobalStatus(100);
	progress->showStatusMessageUTF(buf);
	sleep(2);
	progress->hide();
	delete progress;

ret1:
	if ((res = mount_all(key.c_str())))
	{
        switch(g_settings.hdd_fs) {
                case fs_ext3:
			safe_mkdir(dst);
			res = mount(src, dst, "ext3", 0, NULL);
                        break;
                case fs_ext4:
			safe_mkdir(dst);
			res = mount(src, dst, "ext4", 0, NULL);
			break;
                case fs_ext2:
			safe_mkdir(dst);
			res = mount(src, dst, "ext2", 0, NULL);
			break;
		case fs_jfs:
			safe_mkdir(dst);
			res = mount(src, dst, "jfs", 0, NULL);
			break;
		default:
			break;
		}
	}
	printf("CHDDChkExec: mount res %d\n", res);

	if (!srun) my_system(1, "smbd");
	return menu_return::RETURN_REPAINT;
}

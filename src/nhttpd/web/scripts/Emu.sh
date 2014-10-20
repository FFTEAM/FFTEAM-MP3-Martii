#!/bin/sh

echo $0 $1 >/dev/console

case "$1" in
	camd_start)
		echo "camd---start" >/dev/console
		[ -e /var/etc/.mgcamd ] && /var/emu/mgcamd >/dev/console &
        [ -e /var/etc/.gbox ] && /var/emu/gbox >/dev/console &
		sleep 5
        pzapit -rz
	;;
	camd_stop)
		echo "camd---stop" >/dev/console
        touch /tmp/gbox.kill
		killall mgcamd
		sleep 3
        killall -9 gbox
        killall -9 mgcamd
        [ -e /tmp/ecm.info ] && rm -rf /tmp/ecm.info
        [ -e /tmp/pid.info ] && rm -rf /tmp/pid.info
        [ -e /tmp/mgcamd.pid ] && rm -rf /tmp/mg*
        [ -e /tmp/debug.txt ] && rm -rf /tmp/*.txt
        [ -e /tmp/share.info ] && rm -rf /tmp/share.*
        [ -e /tmp/sc.info ] && rm -rf /tmp/sc*
	;;
	camd_reset)
		$0 camd_stop
		sleep 3
		$0 camd_start
		exit
	;;
    cardserver_start)
        echo "camd---start" >/dev/console
        [ -e /var/etc/.oscam ] && /var/emu/oscam -c /var/keys &
        [ -e /var/etc/.newcs ] && /var/emu/newcs -c /var/keys/newcs.xml >/dev/console &
    ;;
    cardserver_stop)
        echo "cardserver---stop" >/dev/console
        killall -9 oscam
        killall -9 newcs
        [ -e /tmp/oscam.pid ] && rm -rf /tmp/os*
        [ -e /tmp/newcs.pid ] && rm -rf /tmp/new*
    ;;
    cardserver_reset)
        $0 cardserver_stop
        sleep 3
        $0 cardserver_start
        exit
    ;;
    mgcamd_start)
        echo "mgcamd---start" >/dev/console
        $0 camd_stop
        sleep 3
        [ -e /var/etc/.gbox ] && rm -rf /var/etc/.gbox
        touch /var/etc/.mgcamd
        [ -e /var/etc/-mgcamd ] && /var/emu/mgcamd >/dev/console &
        sleep 5
        pzapit -rz
    ;;
    mgcamd_stop)
        echo "mgcamd---stop" >/dev/console
        killall mgcamd
        sleep 3
        killall -9 mgcamd
        [ -e /tmp/ecm.info ] && rm -rf /tmp/ecm.info
        [ -e /tmp/pid.info ] && rm -rf /tmp/pid.info
        [ -e /tmp/mgcamd.pid ] && rm -rf /tmp/mg*
        [ -e /var/etc/.mgcamd ] && rm -rf /var/etc/.mgcamd
    ;;
    gbox_start)
        echo "gbox---start" >/dev/console
        $0 camd_stop
        sleep 3
        $0 cardserver_stop
        [ -e /var/etc/.oscam ] && rm -rf /var/etc/.oscam
        [ -e /var/etc/.newcs ] && rm -rf /var/etc/.newcs
        sleep 2
        [ -e /var/etc/.mgcamd ] && rm -rf /var/etc/.mgcamd
        touch /var/etc/.gbox
        [ -e /var/etc/.gbox ] && /var/emu/gbox >/dev/console &
        sleep 5
        pzapit -rz
    ;;
    gbox_stop)
        echo "gbox---stop" >/dev/console
        touch /tmp/gbox.kill
        sleep 3
        killall -9 gbox
        [ -e /tmp/ecm.info ] && rm -rf /tmp/ecm.info
        [ -e /tmp/pid.info ] && rm -rf /tmp/pid.info
        [ -e /tmp/share.info ] && rm -rf /tmp/share.*
        [ -e /tmp/sc.info ] && rm -rf /tmp/sc*
        [ -e /tmp/gbox.kill ] && rm -rf /tmp/gbox.*
        [ -e /var/etc/.gbox ] && rm -rf /var/etc/.gbox
    ;;
    oscam_start)
        echo "oscam---start" >/dev/console
        $0 cardserver_stop
        [ -e /var/etc/.oscam ] && rm -rf /var/etc/.oscam
        [ -e /var/etc/.newcs ] && rm -rf /var/etc/.newcs
        sleep 2
        [ -e /var/etc/.gbox ] && $0 camd_stop && rm -rf /var/etc/.gbox
        touch /var/etc/.oscam
        [ -e /var/etc/.oscam ] && /var/emu/oscam -c /var/keys &
    ;;
    oscam_stop)
        echo "oscam---stop" >/dev/console
        killall -9 oscam
        [ -e /tmp/oscam.pid ] && rm -rf /tmp/os*
        [ -e /var/etc/.oscam ] && rm -rf /var/etc/.oscam
    ;;
    newcs_start)
        echo "newcs---start" >/dev/console
        $0 cardserver_stop
        [ -e /var/etc/.oscam ] && rm -rf /var/etc/.oscam
        [ -e /var/etc/.newcs ] && rm -rf /var/etc/.newcs
        sleep 2
        [ -e /var/etc/.gbox ] && $0 camd_stop && rm -rf /var/etc/.gbox
        touch /var/etc/.newcs
        [ -e /var/etc/.newcs ] && /var/emu/newcs -c /var/keys/newcs.xml >/dev/console &
    ;;
    newcs_stop)
        echo "newcs---stop" >/dev/console
        killall -9 newcs
        [ -e /tmp/newcs.pid ] && rm -rf /tmp/newcs*
        [ -e /var/etc/.newcs ] && rm -rf /var/etc/.newcs
    ;;
	*)
		$0 camd_stop
        $0 cardserver_stop
		exit 1
	;;
esac

exit 0

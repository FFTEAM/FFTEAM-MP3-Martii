#!/bin/sh

EMU='es ist kein Emu aktiv'
SERVER1='es ist kein Cardserver aktiv'

if [ -e /var/etc/.mgcamd ] ; then
  EMU="mgcamd ist aktiv"
fi

if [ -e /var/etc/.gbox ] ; then
  EMU="gbox ist aktiv"
fi

if [ -e /var/etc/.newcs ] ; then
  SERVER1="newcs ist aktiv"
fi

if [ -e /var/etc/.oscam ] ; then
  SERVER1="oscam ist aktiv"
fi

echo "<b>Aktueller Camd   Status:</b> $EMU <br/>"
echo "<b>Aktueller Server Status:</b> $SERVER1 <br/>"
exit
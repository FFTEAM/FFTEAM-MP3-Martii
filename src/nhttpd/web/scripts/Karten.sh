#!/bin/sh

#----------configure this------------
FILE=/tmp/mg.info
FILE1=/tmp/share.info
#----------configure this------------

if [ -f $FILE ]; then
COUNT=`cat $FILE | wc -l|sed 's/^ *//'`
RECOUNT=`cat $FILE | grep -v Lev:0 | wc -l|sed 's/^ *//'`
DIST1=`cat $FILE | grep -H dist:1 | wc -l|sed 's/^ *//'`
DIST2=`cat $FILE | grep -H dist:2 | wc -l|sed 's/^ *//'`
DIST3=`cat $FILE | grep -H dist:3 | wc -l|sed 's/^ *//'`
DIST4=`cat $FILE | grep -H dist:4 | wc -l|sed 's/^ *//'`
DIST5=`cat $FILE | grep -H dist:5 | wc -l|sed 's/^ *//'`

LEV0=`cat $FILE | grep -H Lev:0 | wc -l|sed 's/^ *//'`
LEV1=`cat $FILE | grep -H Lev:1 | wc -l|sed 's/^ *//'`
LEV2=`cat $FILE | grep -H Lev:2 | wc -l|sed 's/^ *//'`
LEV3=`cat $FILE | grep -H Lev:3 | wc -l|sed 's/^ *//'`
LEV4=`cat $FILE | grep -H Lev:4 | wc -l|sed 's/^ *//'`

echo Es sind derzeit $COUNT Karten online!
echo "<pre>"
echo $RECOUNT davon koennen weitergegeben werden
echo "<pre>"
echo Level 0 : $LEV0
echo Level 1 : $LEV1
echo Level 2 : $LEV2
echo Level 3 : $LEV3
echo Level 4 : $LEV4
echo "<pre>"
echo Dist.1 :  $DIST1
echo Dist.2 :  $DIST2
echo Dist.3 :  $DIST3
echo Dist.4 :  $DIST4
echo Dist.5 :  $DIST5

fi

if [ -f $FILE1 ]; then
COUNT=`cat $FILE1 | wc -l|sed 's/^ *//'`
RECOUNT=`cat $FILE1 | grep -v Lev:0 | wc -l|sed 's/^ *//'`
DIST1=`cat $FILE1 | grep -H dist:1 | wc -l|sed 's/^ *//'`
DIST2=`cat $FILE1 | grep -H dist:2 | wc -l|sed 's/^ *//'`
DIST3=`cat $FILE1 | grep -H dist:3 | wc -l|sed 's/^ *//'`
DIST4=`cat $FILE1 | grep -H dist:4 | wc -l|sed 's/^ *//'`
DIST5=`cat $FILE1 | grep -H dist:5 | wc -l|sed 's/^ *//'`

LEV0=`cat $FILE1 | grep -H Lev:0 | wc -l|sed 's/^ *//'`
LEV1=`cat $FILE1 | grep -H Lev:1 | wc -l|sed 's/^ *//'`
LEV2=`cat $FILE1 | grep -H Lev:2 | wc -l|sed 's/^ *//'`
LEV3=`cat $FILE1 | grep -H Lev:3 | wc -l|sed 's/^ *//'`
LEV4=`cat $FILE1 | grep -H Lev:4 | wc -l|sed 's/^ *//'`

echo Es sind derzeit $COUNT Karten online!
echo "<pre>"
echo $RECOUNT davon koennen weitergegeben werden
echo "<pre>"
echo Level 0 : $LEV0
echo Level 1 : $LEV1
echo Level 2 : $LEV2
echo Level 3 : $LEV3
echo Level 4 : $LEV4
echo "<pre>"
echo Dist.1 :  $DIST1
echo Dist.2 :  $DIST2
echo Dist.3 :  $DIST3
echo Dist.4 :  $DIST4
echo Dist.5 :  $DIST5

fi

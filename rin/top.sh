#!/bin/bash

DATE=`date +%m%d%H%M`
mkdir $DATE # make dir names start time
while true
do
	FNAME=$DATE/`date +%H%M%s`.txt # file name 
	top -b -n 1 > $FNAME # only once
	msgdata=`sed -z -e 's/:/;/g' -e 's/%/ /g' -e 's/\n/ /g' $FNAME`
	#rostopic pub -1 /topmsg std_msgs/String "data: $msgdata"
	rostopic pub -1 /topmsg std_msgs/String "data: $FNAME"
	sleep 1
done

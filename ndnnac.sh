#!/bin/sh

CCN_DIR=~/Downloads/ccnx-0.6.0
OS=`uname`
IP="" # store IP
DST="255.255.255.255"
INFORM_DIR=inform
INFORM_FILE=`pwd`/$INFORM_DIR/inform
NAC_FILE=./nac.conf
NAC_DIR=nac
NAC_CLT=`pwd`/$NAC_DIR/ccnnacnode
CCN_START=$CCN_DIR/bin/ccndstart

#assuming that there is only one IP address
case $OS in
   Linux) IP=`ifconfig  | grep 'inet addr:'| grep -v '127.0.0.1' | cut -d: -f2 | awk '{ print $1}'`;;
   FreeBSD|OpenBSD) IP=`ifconfig  | grep -E 'inet.[0-9]' | grep -v '127.0.0.1' | awk '{ print $2}'` ;;
   SunOS) IP=`ifconfig -a | grep inet | grep -v '127.0.0.1' | awk '{ print $2} '` ;;
   *) IP="Unknown";;
esac

#now we get the IP, if it is not empty, we need to launch INFORM
if [ x$IP != x ]; then
   #run dhclient, and check if the file is empty
   #if the file is empty, means need to check the DNS to get the nac server's addr
   #if still cannot get the nac server's addr, give up, and inform users
   #if can get the addr, use the result to query the nac server directly
   #remove the existing nac.conf file first
   rm -fr $NAC_FILE
   sudo $INFORM_FILE -a $IP -s $DST
   $CCN_START
   if [ -r $NAC_FILE ]; then
       #we need to launch the another program to construct the FIB now
       #first we must get gw's ip and port, then the config file
       $NAC_CLT -a $IP -p 9695 -f $NAC_FILE
   fi
fi

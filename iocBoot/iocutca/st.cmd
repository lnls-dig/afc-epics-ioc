#!../../bin/linux-x86_64/utca

## You may have to change utca to something else
## everywhere it appears in this file

< envPaths
< set_env.cmd

cd "${TOP}"

## Register all support components
dbLoadDatabase "dbd/utca.dbd"
utca_registerRecordDeviceDriver pdbbase

var dbRecordsOnceOnly 1

## Run this to trace the stages of iocInit
#traceIocInit

pcie("${SLOT}")

< "iocBoot/${IOC}/acq.cmd"
< "iocBoot/${IOC}/fofb_cc.cmd"
# rtmlamp has to be after acq
< "iocBoot/${IOC}/rtmlamp.cmd"
< "iocBoot/${IOC}/sysid.cmd"
# fofb_processing has to be after rtmlamp
< "iocBoot/${IOC}/fofb_processing.cmd"
< "iocBoot/${IOC}/triggers.cmd"

cd "${TOP}/iocBoot/${IOC}"
iocInit

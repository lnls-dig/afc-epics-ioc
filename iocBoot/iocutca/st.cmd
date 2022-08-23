#!../../bin/linux-x86_64/utca

## You may have to change utca to something else
## everywhere it appears in this file

< envPaths

cd "${TOP}"

## Register all support components
dbLoadDatabase "dbd/utca.dbd"
utca_registerRecordDeviceDriver pdbbase

## Run this to trace the stages of iocInit
#traceIocInit

cd "${TOP}/iocBoot/${IOC}"
iocInit
#!../../bin/linux-x86_64/utca

## You may have to change utca to something else
## everywhere it appears in this file

< envPaths
< set_env.cmd

cd "${TOP}"

## Register all support components
dbLoadDatabase "dbd/utca.dbd"
utca_registerRecordDeviceDriver pdbbase

## Run this to trace the stages of iocInit
#traceIocInit

pcie("2")
RtmLamp(0)

dbLoadRecords("db/rtmlamp.template", "P=p:, R=r:, S=s:, RTM_CHAN=chan0:, PORT=RTMLAMP-0, ADDR=0")
dbLoadRecords("db/rtmlamp.template", "P=p:, R=r:, S=s:, RTM_CHAN=chan1:, PORT=RTMLAMP-0, ADDR=1")
dbLoadRecords("db/rtmlamp.template", "P=p:, R=r:, S=s:, RTM_CHAN=chan2:, PORT=RTMLAMP-0, ADDR=2")
dbLoadRecords("db/rtmlamp.template", "P=p:, R=r:, S=s:, RTM_CHAN=chan3:, PORT=RTMLAMP-0, ADDR=3")
dbLoadRecords("db/rtmlamp.template", "P=p:, R=r:, S=s:, RTM_CHAN=chan4:, PORT=RTMLAMP-0, ADDR=4")

cd "${TOP}/iocBoot/${IOC}"
iocInit

#!../../bin/linux-x86_64/utca

< common.cmd

epicsEnvSet("S", "${RTM_PREFIX}")

serial("${SERIAL}")

< "iocBoot/$(IOC)/build_info.cmd"

RtmLamp(0)
dbLoadTemplate("db/rtmlamp.substitutions", "P=${P}, R=${R}, S=${S}, NELM=${NELM}, SCAN_PERIOD=1, ACQ_NAME=LAMP, PORT=RTMLAMP-0, ACQ_PORT=ACQ-0")

iocInit

< "iocBoot/${IOC}/apply_asg.cmd"

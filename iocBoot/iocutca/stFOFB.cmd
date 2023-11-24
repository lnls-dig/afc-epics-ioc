#!../../bin/linux-x86_64/utca

< common.cmd

epicsEnvSet("S", "${RTM_PREFIX}")

pcie("${SLOT}")

< "iocBoot/$(IOC)/build_info.cmd"
< "iocBoot/${IOC}/fofb_acq.cmd"
< "iocBoot/${IOC}/fofb_fofb_cc.cmd"
# rtmlamp has to be after acq
< "iocBoot/${IOC}/rtmlamp.cmd"
< "iocBoot/${IOC}/sysid.cmd"
# fofb_processing has to be after rtmlamp
< "iocBoot/${IOC}/fofb_processing.cmd"
< "iocBoot/${IOC}/triggers.cmd"

< "iocBoot/${IOC}/autosave_pre.cmd"

iocInit

< "iocBoot/${IOC}/apply_asg.cmd"

create_monitor_set("fofb_ioc.req", 30, "P=${P}, R=${R}, S=${S}")
set_savefile_name("fofb_ioc.req", "${P}${R}_settings.sav")

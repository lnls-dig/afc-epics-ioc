#!../../bin/linux-x86_64/utca

< common.cmd

epicsEnvSet("S", "${RTM_PREFIX}")

pcie("${SLOT}")

< "iocBoot/$(IOC)/build_info.cmd"
< "iocBoot/${IOC}/acq.cmd"
< "iocBoot/${IOC}/fofb_cc.cmd"
# rtmlamp has to be after acq
< "iocBoot/${IOC}/rtmlamp.cmd"
< "iocBoot/${IOC}/sysid.cmd"
# fofb_processing has to be after rtmlamp
< "iocBoot/${IOC}/fofb_processing.cmd"
< "iocBoot/${IOC}/triggers.cmd"

< "iocBoot/${IOC}/autosave_pre.cmd"

iocInit

< "iocBoot/${IOC}/autosave_post.cmd"

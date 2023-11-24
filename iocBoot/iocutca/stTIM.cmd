#!../../bin/linux-x86_64/utca

< common.cmd

pcie("${SLOT}")

< "iocBoot/$(IOC)/build_info.cmd"

AfcTiming(0)
dbLoadTemplate("db/afc_timing.substitutions", "P=${P}, R=${R}, PORT=AFC_TIMING-0")

< "iocBoot/${IOC}/autosave_pre.cmd"

iocInit

< "iocBoot/${IOC}/apply_asg.cmd"

create_monitor_set("tim_ioc.req", 30, "P=${P}, R=${R}")
set_savefile_name("tim_ioc.req", "${P}${R}_settings.sav")

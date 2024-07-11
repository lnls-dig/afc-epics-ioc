#!../../bin/linux-x86_64/utca

< common.cmd

pcie("${SLOT}")

epicsEnvSet("P1", "${AREA_PREFIX_1}")
epicsEnvSet("P2", "${AREA_PREFIX_2}")

epicsEnvSet("R1", "${DEVICE_PREFIX_1}")
epicsEnvSet("R2", "${DEVICE_PREFIX_2}")

epicsEnvSet("P", "${P1}")
epicsEnvSet("R", "${R1}")

asSetSubstitutions("P=$(P),R=$(R)")

< "iocBoot/$(IOC)/build_info.cmd"
< "iocBoot/${IOC}/bpm_fofb_cc.cmd"
< "iocBoot/${IOC}/orbit_intlk.cmd"
< "iocBoot/${IOC}/triggers.cmd"

var reToolsVerbose 0
reAddAlias "${P1}${R1}(GwInfo.*)" "${P2}${R2}$1"
reAddAlias "${P1}${R1}(DCCP2P.*)" "${P2}${R2}$1"
reAddAlias "${P1}${R1}(Intlk.*)" "${P2}${R2}$1"
reAddAlias "${P1}${R1}(TRIGGER[[:digit:]].*)" "${P2}${R2}$1"

< "iocBoot/${IOC}/bpm_acq.cmd"
< "iocBoot/${IOC}/bpm_swap.cmd"
< "iocBoot/${IOC}/bpm_pos.cmd"
< "iocBoot/${IOC}/bpm_fmc250m_4ch.cmd"

iocInit

< "iocBoot/${IOC}/apply_asg.cmd"

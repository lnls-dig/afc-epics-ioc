Acq(0)
TriggerMux(0)
dbLoadTemplate("db/acq.substitutions", "P=${P}, R=${R}, ACQ_NAME=LAMP, NELM=${NELM}, INDEX=0")
Acq(1)
TriggerMux(1)
dbLoadTemplate("db/acq.substitutions", "P=${P}, R=${R}, ACQ_NAME=SYSID, NELM=${NELM}, INDEX=1")
dbLoadRecords("db/raw_data.template", "P=${P}, R=${R}, ACQ_NAME=SYSID, NELM=${NELM}, PORT=ACQ-1")

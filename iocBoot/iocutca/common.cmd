< envPaths

epicsEnvSet("P", "${AREA_PREFIX}")
epicsEnvSet("R", "${DEVICE_PREFIX}")

epicsEnvSet("NELM", "1000000")

cd "${TOP}"

## Register all support components
dbLoadDatabase "dbd/utca.dbd"
utca_registerRecordDeviceDriver pdbbase

var dbRecordsOnceOnly 1

asSetFilename("$(TOP)/db/accessSecurityFile.acf")

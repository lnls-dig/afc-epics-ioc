TOP=../..

include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS BELOW HERE

# xxxRecord.h will be created from xxxRecord.dbd
DBDINC +=

# Install xxxSupport.dbd into <top>/dbd
DBD +=

# Build an IOC support library
LIBRARY_IOC +=


utca_DBD += base.dbd

utca_LIBS += asyn
utca_DBD += asyn.dbd

utca_SRCS += pcie-single.cpp fofb_cc.cpp rtmlamp.cpp trigger_iface.cpp trigger_mux.cpp
utca_DBD += drivers.dbd

UHAL = $(EPICS_BASE)/../uhal
UHAL_LIBS = $(UHAL)/lib/x86_64-linux-gnu

utca_LIBS += uhal-modules uhal-utilities
uhal-utilities_DIR = $(UHAL_LIBS)
uhal-modules_DIR = $(UHAL_LIBS)

OP_SYS_INCLUDES += -I$(UHAL)/include/uhal
USR_CXXFLAGS += -std=c++17 -Wall -Wextra

# Build the IOC application
PROD_IOC = utca

# utca.dbd will be created and installed
DBD += utca.dbd

# utca.dbd will include these files:
utca_DBD += base.dbd

# utca_registerRecordDeviceDriver.cpp derives from utca.dbd
utca_SRCS += utca_registerRecordDeviceDriver.cpp

# Build the main IOC entry point where needed
utca_SRCS_DEFAULT += utcaMain.cpp
utca_SRCS_vxWorks += -nil-

# Finally link IOC to the EPICS Base libraries
utca_LIBS += $(EPICS_BASE_IOC_LIBS)

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD EXTRA GNUMAKE RULES BELOW HERE


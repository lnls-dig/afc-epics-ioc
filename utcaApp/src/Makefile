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
utca_LIBS += autosave
utca_DBD += asSupport.dbd
utca_LIBS += retools
utca_DBD += retools.dbd

utca_SRCS += pcie-single.cpp acq.cpp afc_timing.cpp bpm_swap.cpp build_info.cpp fmcpico1m_4ch.cpp fofb_cc.cpp fofb_processing.cpp fofb_shaper_filt.cpp orbit_intlk.cpp pos_calc.cpp rtmlamp.cpp trigger_iface.cpp trigger_mux.cpp si57x_ctrl.cpp sysid.cpp
utca_SRCS += fmc_adc_common.cpp
utca_SRCS += fmc_active_clk.cpp
utca_SRCS += ad9510.cpp
utca_DBD += drivers.dbd

utca_SRCS += asub-routines.cpp
utca_DBD += aSubRoutines.dbd

utca_DBD += menuScan.dbd

utca_LIBS += uhal-modules uhal-utilities
uhal-utilities_DIR = $(UHAL_LIBS)
uhal-modules_DIR = $(UHAL_LIBS)

OP_SYS_INCLUDES += -I$(UHAL)/include/uhal
USR_CXXFLAGS += -std=c++20 -Wall -Wextra

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

TESTPROD_HOST += asubRoutinesTest
asubRoutinesTest_SRCS += asub-routines-test.cpp
asubRoutinesTest_LIBS += asyn $(EPICS_BASE_IOC_LIBS)
TESTS += asubRoutinesTest

TESTSCRIPTS_HOST += $(TESTS:%=%.t)

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD EXTRA GNUMAKE RULES BELOW HERE


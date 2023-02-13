TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS BELOW HERE

# Install databases, templates & substitutions like this
DB += scan_task.template
DB += acq.template raw_data.template
DB += fofb_cc.template fofb_cc_channel.template fofb_cc.substitutions
DB += rtmlamp.template rtmlamp_data.template rtmlamp.substitutions
DB += trigger_iface.template trigger_iface.substitutions
DB += trigger_mux.template trigger_mux.substitutions

# If <anyname>.db template is not named <anyname>*.template add
# <anyname>_TEMPLATE = <templatename>

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD EXTRA GNUMAKE RULES BELOW HERE


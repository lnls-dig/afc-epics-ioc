TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS BELOW HERE

# Install databases, templates & substitutions like this
DB += build_info.template

DB += scan_task.template
DB += acq.template raw_data.template acq.substitutions
DB += afc_timing.template afc_timing_clock.template afc_timing_trigger.template afc_timing.substitutions
DB += fofb_cc.template fofb_cc_channel.template fofb_cc.substitutions
DB += fofb_processing.template fofb_processing_channel.template fofb_processing.substitutions
DB += rtmlamp.template rtmlamp_channel.template rtmlamp_data.template rtmlamp.substitutions
DB += sysid.template sysid_channel.template sysid_pos.template sysid_data.template sysid_pos_channel_data.template sysid_sp_channel_data.template sysid.substitutions
DB += trigger_iface.template trigger_iface.substitutions
DB += trigger_mux.template

DB += $(AUTOSAVE)/asApp/Db/save_restoreStatus.db

# If <anyname>.db template is not named <anyname>*.template add
# <anyname>_TEMPLATE = <templatename>

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD EXTRA GNUMAKE RULES BELOW HERE


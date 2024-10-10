TOP=../..
include $(TOP)/configure/CONFIG
#----------------------------------------
#  ADD MACRO DEFINITIONS BELOW HERE

# Install databases, templates & substitutions like this
DB += build_info.template

DB += scan_task.template
DB += acq.template raw_data.template acq.substitutions
DB += ad9510.template
DB += afc_timing.template afc_timing_clock.template afc_timing_trigger.template afc_timing.substitutions
DB += bpm_data.template bpm_pos_data.template bpm_data.substitutions
DB += bpm_swap.template bpm_swap.substitutions
DB += fmc_active_clk.template fmc_active_clk.substitutions
DB += fmc_adc_common.template fmc_adc_common.substitutions
DB += fmcpico1m_4ch.template fmcpico1m_4ch.substitutions
DB += fmc250m.template
DB += fofb_cc.template fofb_cc_channel.template fofb_cc.substitutions
DB += fofb_processing.template fofb_processing_channel.template fofb_processing.substitutions
DB += fofb_shaper_filt.template fofb_shaper_filt_channel.template fofb_shaper_filt.substitutions
DB += orbit_intlk.template orbit_intlk.substitutions
DB += pos_calc.template pos_calc_adc.template pos_calc_rate.template pos_calc.substitutions
DB += rtmlamp.template rtmlamp_channel.template rtmlamp_data.template rtmlamp.substitutions
DB += si57x_ctrl.template
DB += sysid.template sysid_channel.template sysid_pos.template sysid_data.template sysid_pos_channel_data.template sysid_sp_channel_data.template sysid.substitutions
DB += trigger_iface.template trigger_iface.substitutions
DB += trigger_mux.template

DB += $(AUTOSAVE)/asApp/Db/save_restoreStatus.db

DB += accessSecurityFile.acf

# If <anyname>.db template is not named <anyname>*.template add
# <anyname>_TEMPLATE = <templatename>

include $(TOP)/configure/RULES
#----------------------------------------
#  ADD EXTRA GNUMAKE RULES BELOW HERE


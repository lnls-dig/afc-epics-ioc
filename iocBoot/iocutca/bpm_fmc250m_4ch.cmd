FMCActiveClk(0)
Si57x(0)
AD9510(0)
dbLoadTemplate("db/fmc_active_clk.substitutions", "P=${P1}, R=${R1}, PORT=FMC_ACTIVE_CLK-0")
dbLoadRecords("db/si57x_ctrl.template", "P=${P1}, R=${R1}, PORT=SI57X-0")
dbLoadRecords("db/ad9510.template", "P=${P1}, R=${R1}, PORT=AD9510-0")

FMCActiveClk(1)
Si57x(1)
AD9510(1)
dbLoadTemplate("db/fmc_active_clk.substitutions", "P=${P2}, R=${R2}, PORT=FMC_ACTIVE_CLK-1")
dbLoadRecords("db/si57x_ctrl.template", "P=${P2}, R=${R2}, PORT=SI57X-1")
dbLoadRecords("db/ad9510.template", "P=${P2}, R=${R2}, PORT=AD9510-1")

FMCADCCommon(0)
dbLoadTemplate("db/fmc_adc_common.substitutions", "P=${P1}, R=${R1}, PORT=FMC_ADC_COMMON-0")

FMCADCCommon(1)
dbLoadTemplate("db/fmc_adc_common.substitutions", "P=${P2}, R=${R2}, PORT=FMC_ADC_COMMON-1")

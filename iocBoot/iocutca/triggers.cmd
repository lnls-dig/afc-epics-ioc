TriggerIface(0)
dbLoadTemplate("db/trigger_iface.substitutions", "P=${P}, R=${R}, TRIGGER_NAME=TRIGGER, PORT=TRIGGER_IFACE-0")

TriggerMux(0)
dbLoadTemplate("db/trigger_mux.substitutions", "P=${P}, R=${R}, TRIGGER_NAME=TRIGGER, PORT=TRIGGER_MUX-0")
TriggerMux(1)
dbLoadTemplate("db/trigger_mux.substitutions", "P=${P}, R=${R}, TRIGGER_NAME=TRIGGER_DCCFMC, PORT=TRIGGER_MUX-1")
TriggerMux(2)
dbLoadTemplate("db/trigger_mux.substitutions", "P=${P}, R=${R}, TRIGGER_NAME=TRIGGER_DCCP2P, PORT=TRIGGER_MUX-2")

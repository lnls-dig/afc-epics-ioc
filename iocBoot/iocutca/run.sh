#!/bin/sh

set -eu

export SLOT=$1

CRATE=$(./getCrate.sh)
VSLOT=$(./getSlot.sh $SLOT)

cmd=
case "$(decode-reg build_info -q --slot ${SLOT})" in
    afc-tim-receive*)
        CRATE=${CRATE#0}

        . ./tim-slot-mapping
        val_name=CRATE_${CRATE}_TIM_RX_${VSLOT}_PV_AREA_PREFIX
        eval "export AREA_PREFIX=\$${val_name}"
        val_name=CRATE_${CRATE}_TIM_RX_${VSLOT}_PV_DEVICE_PREFIX
        eval "export DEVICE_PREFIX=\$${val_name}"

        case $(hostname) in
          ia-*) ;;
          *) export EPICS_CA_ADDR_LIST=10.0.38.59:60000
        esac

        cmd=TIM
        ;;
    afcv4_fofb_ctrl*)
        . ./fofb-slot-mapping
        val_name=CRATE_${CRATE}_FOFB_${VSLOT}_PV_AREA_PREFIX
        eval "export AREA_PREFIX=\$${val_name}"
        val_name=CRATE_${CRATE}_FOFB_${VSLOT}_PV_DEVICE_PREFIX
        eval "export DEVICE_PREFIX=\$${val_name}"
        export RTM_PREFIX=SI-${CRATE}

        cmd=FOFB
        ;;
esac

if [ -n "$cmd" ]; then
    exec procServ -f -n afc-ioc-${SLOT} -L- -i ^C^D -P $(expr 1700 + ${SLOT%-*}) ./st${cmd}.cmd
fi

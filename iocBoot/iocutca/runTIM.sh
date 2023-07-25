#!/bin/sh

set -eu

export SLOT=$1

CRATE=$(./getCrate.sh)
VSLOT=$(./getSlot.sh $SLOT)

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

exec procServ -f -n tim${SLOT} -L- -i ^C^D -P $(expr 1700 + ${VSLOT}) ./stTIM.cmd

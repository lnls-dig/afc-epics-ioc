#!/bin/sh

set -eu

export SLOT=$1

CRATE=$(./getCrate.sh)
VSLOT=$(./getSlot.sh $SLOT)

. ./fofb-slot-mapping
val_name=CRATE_${CRATE}_FOFB_${VSLOT}_PV_AREA_PREFIX
eval "export AREA_PREFIX=\$${val_name}"
val_name=CRATE_${CRATE}_FOFB_${VSLOT}_PV_DEVICE_PREFIX
eval "export DEVICE_PREFIX=\$${val_name}"
export RTM_PREFIX=SI-${CRATE}

exec procServ -f -n fofb${SLOT} -L- -i ^C^D -P $(expr 1700 + ${VSLOT}) ./stFOFB.cmd

#!/bin/sh
set -eu

DEV=$1

devslot=
for slot in /sys/bus/pci/slots/* ; do
    ADDR=$(cat ${slot}/address)
    if [ "${ADDR}.0" = "${DEV}" ]; then
        devslot=$(basename ${slot})
        break
    fi
done

if [ -n "${devslot}" -a "${devslot}" = "2-1" ]; then
    systemctl restart erics-fofb@${devslot}.service
fi

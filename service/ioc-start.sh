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

case "$(decode-reg build_info -q --slot ${devslot})" in
    afc-tim-receive*|afcv4_fofb_ctrl*)
        systemctl restart afc-ioc@${devslot}.service
        ;;
esac

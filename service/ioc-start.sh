#!/bin/sh
set -eu

DEV=$1

setpci -s $DEV COMMAND=0x2

resource_files=""
for suffix in 0 2 2_wc 4; do
    resource_files="$resource_files /sys/bus/pci/devices/$DEV/resource$suffix"
done
resource_files_exist() {
    for f in $resource_files; do
        if [ ! -f $f ]; then
            return 1
        fi
    done

    return 0
}

count=0
until resource_files_exist; do
    if [ $count -eq 10 ]; then
        exit 1
    fi
    count=$((count + 1))
    sleep .1
done
chmod 666 $resource_files

devslot=
for slot in /sys/bus/pci/slots/* ; do
    ADDR=$(cat ${slot}/address)
    if [ "${ADDR}.0" = "${DEV}" ]; then
        devslot=$(basename ${slot})
        break
    fi
done

start_afc_ioc=false

synthesis="$(decode-reg build_info -q --slot ${devslot})"
if [ -z "$synthesis" ]; then
    decode-reg reset --slot ${devslot}
    synthesis="$(decode-reg build_info -q --slot ${devslot})"
fi

case "$synthesis" in
    afc-tim-receive*|afcv4_fofb_ctrl*|pbpm-gw*)
        start_afc_ioc=true
        ;;
    bpm-gw*)
        start_afc_ioc=true

        VSLOT1=$(/opt/afc-epics-ioc/iocBoot/iocutca/getSlot.sh $devslot)
        VSLOT2=$(( VSLOT1 + 1 ))

        services="rffe-ioc@${VSLOT1}.service"
        [ "$VSLOT2" -lt 24 ] && services="$services rffe-ioc@${VSLOT2}.service"

        systemctl start $services || true
        ;;
esac

if $start_afc_ioc; then
    systemctl restart afc-ioc@${devslot}.service
fi

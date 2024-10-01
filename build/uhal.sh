#!/bin/sh

set -eux

OLD_PWD=$PWD

if [ ! -d /tmp/uhal/ ]; then
	git clone --recursive --jobs $(nproc) https://github.com/lnls-dig/uhal.git /tmp/uhal
else
	git -C /tmp/uhal fetch
fi

cd /tmp/uhal
git checkout $UHAL_VERSION

if [ ! -d build/ ]; then
	LDFLAGS=-static meson setup --buildtype debugoptimized --optimization 2 -Dpcie_opt=true build/
fi

samu -C build/
meson install -C build/ --skip-subprojects

install -D /usr/local/bin/decode-reg $OLD_PWD/bin/$(perl $EPICS_BASE_PATH/src/tools/EpicsHostArch.pl)/decode-reg

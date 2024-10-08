# AFC EPICS IOC

This project implements IOCs for the AFC boards supported by the [μHAL
library](https://github.com/lnls-dig/uhal).

## How to build

This project uses the EPICS build system, and it depends on EPICS 7, and the
Asyn and Autosave modules; and the μHAL library. The `configure/RELEASE.local`
file must define the paths for `EPICS_BASE`, `ASYN`, `AUTOSAVE`, and `RETOOLS`;
and the μHAL library is expected to be installed to `/usr/local`.

### Scripted build

Using the containerized build under `build/` to generate a release tarball
removes the need for manual steps.

```
$ cd build/
$ docker compose up
```

## Licensing

It is released according to the GNU GPL, version 3 or any later version.

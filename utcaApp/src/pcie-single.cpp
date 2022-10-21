#include <stdexcept>

#include <iocsh.h>
#include <epicsExport.h>

#include "pcie-open.h"

#include "pcie-single.h"

struct pcie_bars bars;

extern "C" {
    static const iocshArg initArg0 = {"slotName", iocshArgString};
    static const iocshArg *initArgs[] = {&initArg0};
    static constexpr iocshFuncDef initFuncDef {
        "pcie",
        1,
        initArgs
#ifdef IOCSHFUNCDEF_HAS_USAGE
        ,"Connect to PCIe slot slotName\n"
#endif
    };
    static void initCallFunc(const iocshArgBuf *args)
    {
        try {
            std::string slot_name = args[0].sval;
            dev_open_slot(bars, slot_name.c_str());
        } catch (std::runtime_error &e) {
            /* FIXME: do something with this exception,
             * so far we are only avoiding propagating it through C */
            fprintf(stderr, "init pcie error: %s\n", e.what());
            return;
        }
    }

    static void registerPcie(void)
    {
        iocshRegister(&initFuncDef, initCallFunc);
    }

    epicsExportRegistrar(registerPcie);
}

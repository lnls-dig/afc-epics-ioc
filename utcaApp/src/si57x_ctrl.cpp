#include <stdexcept>
#include <string>
#include <tuple>

#include <modules/si57x_ctrl.h>

#include "pcie-single.h"
#include "udriver.h"

namespace {
    const double fstartup = 155'520'000;
}

class Si57x: public UDriver {
    si57x_ctrl::Core dec;
    si57x_ctrl::Controller ctl;

    int p_freq;

  public:
    Si57x(int port_number):
      UDriver(
          "SI57X", port_number, &dec, 1,
          { },
          { },
          &ctl),
      dec(bars),
      ctl(bars, fstartup)
    {
        auto v = find_device(port_number);
        dec.set_devinfo(v);
        ctl.set_devinfo(v);

        createParam("FREQ", asynParamFloat64, &p_freq);

        read_parameters();

        /* use try/catch block because read_startup_regs() can throw as well */
        try {
            if (!ctl.read_startup_regs())
                throw std::runtime_error("couldn't obtain startup registers");
        } catch (std::runtime_error &e) {
            /* guarantee writes to frequency aren't applied */
            p_freq = -1;

            fprintf(stderr, "couldn't start up Si57x properly: %s\n", e.what());
        }
    }

    asynStatus writeFloat64Impl(asynUser *, const int function, const int, epicsFloat64 value)
    {
        if (function == p_freq) {
            if (ctl.set_freq(value)) {
                if (ctl.apply_config()) {
                    setDoubleParam(function, value);
                    do_callbacks();
                    return asynSuccess;
                }
            }
        }

        return asynError;
    }
};

constexpr char name[] = "Si57x";
UDriverFn<Si57x, name> iocsh_init;

extern "C" {
    static void registerSi57x()
    {
        iocshRegister(&iocsh_init.init_func_def, iocsh_init.init_call_func);
    }

    epicsExportRegistrar(registerSi57x);
}

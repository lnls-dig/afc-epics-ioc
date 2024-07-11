#include <stdexcept>
#include <string>
#include <tuple>

#include <modules/ad9510.h>

#include "pcie-single.h"
#include "udriver.h"

class AD9510: public UDriver {
    ad9510::Controller ctl;

    int p_a_div, p_b_div, p_r_div;

  public:
    AD9510(int port_number):
      UDriver(
          "AD9510", port_number, nullptr, 1,
          { }, { }),
      ctl(bars)
    {
        auto v = find_device(port_number, [this](auto &d){ return ctl.match_devinfo(d); });
        ctl.set_devinfo(v);

        createParam("A_DIV", asynParamInt32, &p_a_div);
        createParam("B_DIV", asynParamInt32, &p_b_div);
        createParam("R_DIV", asynParamInt32, &p_r_div);

        ctl.set_defaults();
    }

    asynStatus writeInt32Impl(asynUser *, const int function, const int, epicsInt32 value)
    {
        if (function == p_a_div || function == p_b_div || function == p_r_div) {
            bool rv;
            if (function == p_a_div)
                rv = ctl.set_a_div(value);
            else if (function == p_b_div)
                rv = ctl.set_b_div(value);
            else if (function == p_r_div)
                rv = ctl.set_r_div(value);

            if (!rv)
                return asynError;

            setIntegerParam(function, value);
            do_callbacks();
            return asynSuccess;
        }

        return asynError;
    }
};

constexpr char name[] = "AD9510";
UDriverFn<AD9510, name> iocsh_init;

extern "C" {
    static void registerAD9510()
    {
        iocshRegister(&iocsh_init.init_func_def, iocsh_init.init_call_func);
    }

    epicsExportRegistrar(registerAD9510);
}

#include <algorithm>

#include <modules/fofb_shaper_filt.h>

#include "pcie-single.h"
#include "udriver.h"

namespace {
    const unsigned number_of_channels = 12;
}

class FOFBShaperFilt: public UDriver {
    fofb_shaper_filt::Core dec;
    fofb_shaper_filt::Controller ctl;

    int p_coefficients;

  public:
    FOFBShaperFilt(int port_number):
      UDriver(
          "FOFB_SHAPER_FILT", port_number, &dec,
          ::number_of_channels,
          {
              {"NUM_BIQUADS", p_read_only},
          },
          { }),
      dec(bars),
      ctl(bars)
    {
        auto v = find_device(port_number);
        dec.set_devinfo(v);
        ctl.set_devinfo(v);

        createParam("COEFF", asynParamFloat64Array, &p_coefficients);

        read_parameters();
    }

    asynStatus writeFloat64Array(asynUser *pasynUser, epicsFloat64 *value, size_t nElements)
    {
        int function = pasynUser->reason, addr;
        getAddress(pasynUser, &addr);

        if (function == p_coefficients) {
            std::copy_n(value, nElements, ctl.coefficients.values.at(addr).begin());
        } else {
            return asynPortDriver::writeFloat64Array(pasynUser, value, nElements);
        }

        auto rv = write_params(pasynUser, ctl);

        if (function == p_coefficients) {
            auto &readback = dec.coefficients.values.at(addr);
            doCallbacksFloat64Array(readback.data(), readback.size(), function, addr);
        }

        return rv;
    }
};

constexpr char name[] = "FOFBShaperFilt";
UDriverFn<FOFBShaperFilt, name> iocsh_init;

extern "C" {
    static void registerFOFBShaperFilt()
    {
        iocshRegister(&iocsh_init.init_func_def, iocsh_init.init_call_func);
    }

    epicsExportRegistrar(registerFOFBShaperFilt);
}

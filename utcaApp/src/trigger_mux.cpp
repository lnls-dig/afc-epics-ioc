#include <stdexcept>
#include <string>
#include <tuple>

#include "trigger_mux.h"

#include "pcie-single.h"
#include "udriver.h"

namespace {
    const unsigned number_of_channels = 24;
}

class TriggerMux: public UDriver {
    trigger_mux::Core dec;
    trigger_mux::Controller ctl;

    int p_rcv_src, p_rcv_in_sel, p_transm_src, p_transm_out_sel;

  public:
    TriggerMux(int port_number):
      UDriver(
          "TRIGGER_MUX", port_number, &dec,
          ::number_of_channels,
          { },
          {
              {"RCV_SRC", p_rcv_src},
              {"RCV_IN_SEL", p_rcv_in_sel},
              {"TRANSM_SRC", p_transm_src},
              {"TRANSM_OUT_SEL", p_transm_out_sel},
          }),
      dec(bars),
      ctl(bars)
    {
        if (auto v = read_sdb(&bars, ctl.device_match, port_number)) {
            dec.set_devinfo(*v);
            ctl.set_devinfo(*v);
        } else {
            throw std::runtime_error("couldn't find trigger_mux module");
        }

        read_parameters();
    }

    asynStatus writeInt32Impl(asynUser *pasynUser, const int function, const int addr, const char *param_name, epicsInt32 value)
    {
        if (function == p_rcv_src) ctl.parameters[addr].rcv_src = value;
        if (function == p_rcv_in_sel) ctl.parameters[addr].rcv_in_sel = value;
        if (function == p_transm_src) ctl.parameters[addr].transm_src = value;
        if (function == p_transm_out_sel) ctl.parameters[addr].transm_out_sel = value;

        try {
            ctl.write_params();
        } catch (std::runtime_error &e) {
            epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                "writeInt32: %s: %s", param_name, e.what());

            return asynError;
        }

        /* refresh parameters */
        read_parameters();

        return asynSuccess;
    }
};

constexpr char name[] = "TriggerMux";
UDriverFn<TriggerMux, name> iocsh_init;

extern "C" {
    static void registerTriggerMux()
    {
        iocshRegister(&iocsh_init.init_func_def, iocsh_init.init_call_func);
    }

    epicsExportRegistrar(registerTriggerMux);
}

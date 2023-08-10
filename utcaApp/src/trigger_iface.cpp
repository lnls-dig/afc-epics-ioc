#include <stdexcept>
#include <string>
#include <tuple>

#include <modules/trigger_iface.h>

#include "pcie-single.h"
#include "udriver.h"

namespace {
    const unsigned number_of_channels = 24;
}

class TriggerIface: public UDriver {
    trigger_iface::Core dec;
    trigger_iface::Controller ctl;

    int p_dir, p_dir_pol, p_rcv_count_rst, p_transm_count_rst, p_rcv_len, p_transm_len, p_rcv_count, p_transm_count;

  public:
    TriggerIface(int port_number):
      UDriver(
          "TRIGGER_IFACE", port_number, &dec,
          ::number_of_channels,
          { },
          {
              {"DIR", p_dir},
              {"DIR_POL", p_dir_pol},
              {"RCV_COUNT_RST", p_rcv_count_rst},
              {"TRANSM_COUNT_RST", p_transm_count_rst},
              {"RCV_LEN", p_rcv_len},
              {"TRANSM_LEN", p_transm_len},
              {"RCV_COUNT", p_rcv_count},
              {"TRANSM_COUNT", p_transm_count},
          }),
      dec(bars),
      ctl(bars)
    {
        auto v = find_device(port_number);
        dec.set_devinfo(v);
        ctl.set_devinfo(v);

        write_only = {p_rcv_count_rst, p_transm_count_rst};

        read_parameters();
    }

    asynStatus writeInt32Impl(asynUser *pasynUser, const int function, const int addr, const char *param_name, epicsInt32 value)
    {
        if (function == p_dir) ctl.parameters[addr].direction = value;
        if (function == p_dir_pol) ctl.parameters[addr].direction_polarity = value;
        if (function == p_rcv_count_rst) ctl.parameters[addr].rcv_count_rst = value;
        if (function == p_transm_count_rst) ctl.parameters[addr].transm_count_rst = value;
        if (function == p_rcv_len) ctl.parameters[addr].rcv_len = value;
        if (function == p_transm_len) ctl.parameters[addr].transm_len = value;

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

constexpr char name[] = "TriggerIface";
UDriverFn<TriggerIface, name> iocsh_init;

extern "C" {
    static void registerTriggerIface()
    {
        iocshRegister(&iocsh_init.init_func_def, iocsh_init.init_call_func);
    }

    epicsExportRegistrar(registerTriggerIface);
}

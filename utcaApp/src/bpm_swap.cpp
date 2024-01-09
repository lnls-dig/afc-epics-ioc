#include <alarm.h>
#include <epicsString.h>

#include <modules/bpm_swap.h>

#include "pcie-single.h"
#include "udriver.h"

class BPMSwap: public UDriver {
    bpm_swap::Core dec;
    bpm_swap::Controller ctl;

    int p_reset, p_mode, p_swap_div_f_cnt_en, p_swap_div_f, p_deswap_delay;

  public:
    BPMSwap(int port_number):
      UDriver(
          "BPM_SWAP", port_number, &dec,
          1,
          {
              {"RESET", p_reset},
              {"MODE", p_mode},
              {"DIV_F_CNT_EN", p_swap_div_f_cnt_en},
              {"DIV_F", p_swap_div_f},
              {"DLY", p_deswap_delay},
          },
          { }),
      dec(bars),
      ctl(bars)
    {
        auto v = find_device(port_number);
        dec.set_devinfo(v);
        ctl.set_devinfo(v);

        write_only = {p_reset};

        read_parameters();
    }

    asynStatus readEnum(asynUser *pasynUser, char *strings[], int values[],
        int severities[], size_t, size_t *nIn)
    {
        const int function = pasynUser->reason;
        if (function == p_mode)
            return fill_enum(strings, values, severities, nIn, ctl.mode_list);
        return asynError;
    }

    asynStatus writeInt32Impl(asynUser *pasynUser, const int function, const int, epicsInt32 value)
    {
        if (function == p_reset) ctl.reset = value;
        else if (function == p_mode) ctl.mode = ctl.mode_list[value];
        else if (function == p_swap_div_f_cnt_en) ctl.swap_div_f_cnt_en = value;
        else if (function == p_swap_div_f) ctl.swap_div_f = value;
        else if (function == p_deswap_delay) ctl.deswap_delay = value;

        return write_params(pasynUser, ctl);
    }
};

constexpr char name[] = "BPMSwap";
UDriverFn<BPMSwap, name> iocsh_init;

extern "C" {
    static void registerBpmSwap()
    {
        iocshRegister(&iocsh_init.init_func_def, iocsh_init.init_call_func);
    }

    epicsExportRegistrar(registerBpmSwap);
}

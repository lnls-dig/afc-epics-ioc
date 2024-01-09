#include <stdexcept>
#include <string>
#include <tuple>

#include <modules/lamp.h>

#include "pcie-single.h"
#include "udriver.h"

namespace {
    const unsigned number_of_channels = 12;
}

class RtmLamp: public UDriver {
    lamp::Core dec;
    lamp::Controller ctl;

    int p_mode;

  public:
    RtmLamp(int port_number):
      UDriver(
          "RTMLAMP", port_number, &dec,
          ::number_of_channels,
          {
              {"PS_STATUS", p_read_only},
          },
          {
              {"AMP_STATUS", p_read_only},
              {"AMP_STATUS_LATCH", p_read_only},
              {"AMP_EN", p_decoder_controller},
              {"MODE", p_mode},
              {"TRIG_EN", p_decoder_controller},
              {"RST_LATCH", p_decoder_controller},
              {"PI_KP", p_decoder_controller},
              {"PI_TI", p_decoder_controller},
              {"CNT", p_decoder_controller},
              {"LIMIT_A", p_decoder_controller},
              {"LIMIT_B", p_decoder_controller},
              {"PI_SP", p_decoder_controller},
              {"DAC", p_decoder_controller},
              {"ADC_INST", p_read_only},
              {"DAC_EFF", p_read_only},
              {"SP_EFF", p_read_only},
          },
          &ctl),
      dec(bars),
      ctl(bars)
    {
        auto v = find_device(port_number);
        dec.set_devinfo(v);
        ctl.set_devinfo(v);

        decoder_controller.insert(p_mode);

        read_parameters();
    }

    asynStatus readEnum(asynUser *pasynUser, char *strings[], int values[],
        int severities[], size_t, size_t *nIn)
    {
        const int function = pasynUser->reason;
        if (function == p_mode)
            return fill_enum(strings, values, severities, nIn, lamp::mode_list);
        return asynError;
    }
};

constexpr char name[] = "RtmLamp";
UDriverFn<RtmLamp, name> iocsh_init;

extern "C" {
    static void registerRtmLamp()
    {
        iocshRegister(&iocsh_init.init_func_def, iocsh_init.init_call_func);
    }

    epicsExportRegistrar(registerRtmLamp);
}

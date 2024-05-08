#include <modules/fmcpico1m_4ch.h>

#include "pcie-single.h"
#include "udriver.h"

namespace {
    const unsigned number_of_channels = 4;
}

class FMCPico: public UDriver {
    fmcpico1m_4ch::Core dec;
    fmcpico1m_4ch::Controller ctl;

    int p_range;

  public:
    FMCPico(int port_number):
      UDriver(
          "FMC_PICO", port_number, &dec,
          ::number_of_channels,
          { },
          {
              {"RANGE", p_range},
          },
          &ctl),
      dec(bars),
      ctl(bars)
    {
        auto v = find_device(port_number);
        dec.set_devinfo(v);
        ctl.set_devinfo(v);

        parameter_props.at(p_range).write_decoder_controller = true;

        read_parameters();
    }

    asynStatus readEnum(asynUser *pasynUser, char *strings[], int values[],
        int severities[], size_t, size_t *nIn)
    {
        const int function = pasynUser->reason;
        if (function == p_range)
            return fill_enum(strings, values, severities, nIn, fmcpico1m_4ch::range_list);
        return asynError;
    }
};

constexpr char name[] = "FMCPico";
UDriverFn<FMCPico, name> iocsh_init;

extern "C" {
    static void registerFMCPico()
    {
        iocshRegister(&iocsh_init.init_func_def, iocsh_init.init_call_func);
    }

    epicsExportRegistrar(registerFMCPico);
}

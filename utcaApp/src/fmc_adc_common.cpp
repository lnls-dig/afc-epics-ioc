#include <stdexcept>
#include <string>
#include <tuple>

#include <modules/fmc_adc_common.h>

#include "pcie-single.h"
#include "udriver.h"

class FMCADCCommon: public UDriver {
    fmc_adc_common::Core dec;
    fmc_adc_common::Controller ctl;

  public:
    FMCADCCommon(int port_number):
      UDriver(
          "FMC_ADC_COMMON", port_number, &dec, 1,
          {
              {"MMCM_LOCKED", p_read_only},
              {"DIR", p_decoder_controller},
              {"TERM", p_decoder_controller},
              {"TEST_DATA_EN", p_decoder_controller},
              {"MMCM_RST", p_decoder_controller},
          },
          { },
          &ctl),
      dec(bars),
      ctl(bars)
    {
        auto v = find_device(port_number);
        dec.set_devinfo(v);
        ctl.set_devinfo(v);

        read_parameters();
    }
};

constexpr char name[] = "FMCADCCommon";
UDriverFn<FMCADCCommon, name> iocsh_init;

extern "C" {
    static void registerFMCADCCommon()
    {
        iocshRegister(&iocsh_init.init_func_def, iocsh_init.init_call_func);
    }

    epicsExportRegistrar(registerFMCADCCommon);
}

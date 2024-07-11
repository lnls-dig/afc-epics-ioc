#include <stdexcept>
#include <string>
#include <tuple>

#include <modules/fmc_active_clk.h>

#include "pcie-single.h"
#include "udriver.h"

class FMCActiveClk: public UDriver {
    fmc_active_clk::Core dec;
    fmc_active_clk::Controller ctl;

  public:
    FMCActiveClk(int port_number):
      UDriver(
          "FMC_ACTIVE_CLK", port_number, &dec, 1,
          {
              {"SI571_OE", p_decoder_controller},
              {"PLL_FUNCTION", p_decoder_controller},
              {"PLL_STATUS", p_read_only},
              {"CLK_SEL", p_decoder_controller},
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

        /* make sure we aren't resetting the PLL during startup */
        ctl.write_general("PLL_FUNCTION", 1);
        ctl.write_params();
        read_parameters();
    }
};

constexpr char name[] = "FMCActiveClk";
UDriverFn<FMCActiveClk, name> iocsh_init;

extern "C" {
    static void registerFMCActiveClk()
    {
        iocshRegister(&iocsh_init.init_func_def, iocsh_init.init_call_func);
    }

    epicsExportRegistrar(registerFMCActiveClk);
}

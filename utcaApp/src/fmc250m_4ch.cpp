#include <modules/fmc250m_4ch.h>

#include "pcie-single.h"
#include "udriver.h"

class FMC250M: public UDriver {
    fmc250m_4ch::Core dec;
    fmc250m_4ch::Controller ctl;

  public:
    FMC250M(int port_number):
      UDriver(
          "FMC250M", port_number, &dec, 1,
          {
              {"RST_ADCS", p_decoder_controller},
              {"RST_DIV_ADCS", p_decoder_controller},
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

        /* make sure the ADCs aren't sleeping */
        ctl.write_general("SLEEP_ADCS", 0);
        ctl.write_params();
        read_parameters();
    }
};

constexpr char name[] = "FMC250M";
UDriverFn<FMC250M, name> iocsh_init;

extern "C" {
    static void registerFMC250M()
    {
        iocshRegister(&iocsh_init.init_func_def, iocsh_init.init_call_func);
    }

    epicsExportRegistrar(registerFMC250M);
}

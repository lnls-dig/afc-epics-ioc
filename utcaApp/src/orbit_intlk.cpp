#include <alarm.h>
#include <epicsString.h>

#include <modules/orbit_intlk.h>

#include "pcie-single.h"
#include "udriver.h"

class OrbitIntlk: public UDriver {
    orbit_intlk::Core dec;
    orbit_intlk::Controller ctl;

    int p_enable, p_clear,
        p_min_sum_en,
        p_pos_en, p_pos_clear, p_ang_en, p_ang_clear,
        p_min_sum,
        p_pos_max_x, p_pos_max_y, p_ang_max_x, p_ang_max_y,
        p_pos_min_x, p_pos_min_y, p_ang_min_x, p_ang_min_y;

  public:
    OrbitIntlk(int port_number):
      UDriver(
          "ORBIT_INTLK", port_number, &dec,
          1,
          {
              {"EN", p_enable},
              ParamInit{"CLR", p_clear}.set_wo(),
              {"MIN_SUM_EN", p_min_sum_en},
              {"POS_EN", p_pos_en},
              ParamInit{"POS_CLR", p_pos_clear}.set_wo(),
              {"ANG_EN", p_ang_en},
              ParamInit{"ANG_CLR", p_ang_clear}.set_wo(),

              {"POS_UPPER_X", p_read_only},
              {"POS_UPPER_Y", p_read_only},
              {"POS_UPPER_LTC_X", p_read_only},
              {"POS_UPPER_LTC_Y", p_read_only},
              {"ANG_UPPER_X", p_read_only},
              {"ANG_UPPER_Y", p_read_only},
              {"ANG_UPPER_LTC_X", p_read_only},
              {"ANG_UPPER_LTC_Y", p_read_only},
              {"INTLK", p_read_only},
              {"INTLK_LTC", p_read_only},
              {"POS_LOWER_X", p_read_only},
              {"POS_LOWER_Y", p_read_only},
              {"POS_LOWER_LTC_X", p_read_only},
              {"POS_LOWER_LTC_Y", p_read_only},
              {"ANG_LOWER_X", p_read_only},
              {"ANG_LOWER_Y", p_read_only},
              {"ANG_LOWER_LTC_X", p_read_only},
              {"ANG_LOWER_LTC_Y", p_read_only},

              {"MIN_SUM", p_min_sum},
              {"POS_MAX_X", p_pos_max_x},
              {"POS_MAX_Y", p_pos_max_y},
              {"ANG_MAX_X", p_ang_max_x},
              {"ANG_MAX_Y", p_ang_max_y},
              {"POS_MIN_X", p_pos_min_x},
              {"POS_MIN_Y", p_pos_min_y},
              {"ANG_MIN_X", p_ang_min_x},
              {"ANG_MIN_Y", p_ang_min_y},

              {"POS_X_INST", p_read_only},
              {"POS_Y_INST", p_read_only},
              {"ANG_X_INST", p_read_only},
              {"ANG_Y_INST", p_read_only},
          },
          { }),
      dec(bars),
      ctl(bars)
    {
        auto v = find_device(port_number);
        dec.set_devinfo(v);
        ctl.set_devinfo(v);

        read_parameters();
    }

    asynStatus writeInt32Impl(asynUser *pasynUser, const int function, const int, epicsInt32 value)
    {
        if (function == p_enable) ctl.enable = value;
        else if (function == p_clear) ctl.clear = value;
        else if (function == p_min_sum_en) ctl.min_sum_enable = value;
        else if (function == p_pos_en) ctl.pos_enable = value;
        else if (function == p_pos_clear) ctl.pos_clear = value;
        else if (function == p_ang_en) ctl.ang_enable = value;
        else if (function == p_ang_clear) ctl.ang_clear = value;
        else if (function == p_min_sum) ctl.min_sum = value;
        else if (function == p_pos_max_x) ctl.pos_max_x = value;
        else if (function == p_pos_max_y) ctl.pos_max_y = value;
        else if (function == p_ang_max_x) ctl.ang_max_x = value;
        else if (function == p_ang_max_y) ctl.ang_max_y = value;
        else if (function == p_pos_min_x) ctl.pos_min_x = value;
        else if (function == p_pos_min_y) ctl.pos_min_y = value;
        else if (function == p_ang_min_x) ctl.ang_min_x = value;
        else if (function == p_ang_min_y) ctl.ang_min_y = value;

        return write_params(pasynUser, ctl);
    }
};

constexpr char name[] = "OrbitIntlk";
UDriverFn<OrbitIntlk, name> iocsh_init;

extern "C" {
    static void registerOrbitIntlk()
    {
        iocshRegister(&iocsh_init.init_func_def, iocsh_init.init_call_func);
    }

    epicsExportRegistrar(registerOrbitIntlk);
}

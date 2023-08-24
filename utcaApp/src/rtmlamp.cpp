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
    lamp::CoreV2 dec;
    lamp::ControllerV2 ctl;

    int p_psstatus;
    int p_ampstatus, p_ampstatus_latch,
        p_pwrstate, p_opmode, p_trigen, p_reset_latch,
        p_loopkp, p_loopti,
        p_testwaveperiod, p_testlim_a, p_testlim_b,
        p_pi_sp, p_dac, p_eff_adc, p_eff_dac, p_eff_sp;

  public:
    RtmLamp(int port_number):
      UDriver(
          "RTMLAMP", port_number, &dec,
          ::number_of_channels,
          {
              {"PS_STATUS", p_psstatus},
          },
          {
              {"AMP_STATUS", p_ampstatus},
              {"AMP_STATUS_LATCH", p_ampstatus_latch},
              {"AMP_EN", p_pwrstate},
              {"MODE", p_opmode},
              {"TRIG_EN", p_trigen},
              {"RST_LATCH", p_reset_latch},
              {"PI_KP", p_loopkp},
              {"PI_TI", p_loopti},
              {"CNT", p_testwaveperiod},
              {"LIMIT_A", p_testlim_a},
              {"LIMIT_B", p_testlim_b},
              {"PI_SP", p_pi_sp},
              {"DAC", p_dac},
              {"ADC_INST", p_eff_adc},
              {"DAC_EFF", p_eff_dac},
              {"SP_EFF", p_eff_sp},
          }),
      dec(bars),
      ctl(bars)
    {
        auto v = find_device(port_number);
        dec.set_devinfo(v);
        ctl.set_devinfo(v);

        /* XXX: initialize this one specifically to avoid an exception */
        for (unsigned addr = 0; addr < number_of_channels; addr++) {
            setIntegerParam(addr, p_opmode, 0);
        }

        write_only = {p_reset_latch};

        read_parameters();
    }

    asynStatus writeInt32Impl(asynUser *pasynUser, const int function, const int addr, const char *param_name, epicsInt32 value)
    {
        setIntegerParam(addr, function, value);

        ctl.channel = addr;

        auto write_param = [addr, this](auto fn, auto &ctl_value) {
            epicsInt32 tmp;
            /* we expect success, unless the parameter hasn't been initialized yet */
            if (getIntegerParam(addr, fn, &tmp) == asynSuccess)
                ctl_value = tmp;
            else
                ctl_value = 0;
        };

        write_param(p_pwrstate, ctl.amp_enable);
        write_param(p_opmode, ctl.mode_numeric);
        write_param(p_loopkp, ctl.pi_kp);
        write_param(p_loopti, ctl.pi_ti);
        write_param(p_testwaveperiod, ctl.cnt);
        write_param(p_testlim_a, ctl.limit_a);
        write_param(p_testlim_b, ctl.limit_b);
        write_param(p_pi_sp, ctl.pi_sp);
        write_param(p_dac, ctl.dac);
        /* XXX: if someone writes 1 into trigen and it's not supported by the hardware,
         * we'll error out continuously until zeroed-out again */
        write_param(p_trigen, ctl.trigger_enable);
        write_param(p_reset_latch, ctl.reset_latch);

        return write_params(pasynUser, ctl);
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

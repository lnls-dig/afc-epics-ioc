#include <stdexcept>
#include <string>
#include <tuple>

#include <iocsh.h>
#include <epicsExport.h>
#include <cantProceed.h>

#include <asynPortDriver.h>

#include "lamp.h"

#include "pcie-single.h"
#include "udriver.h"

namespace {
    const unsigned number_of_channels = 12;
}

class RtmLamp: public UDriver {
    lamp::CoreV2 dec;
    lamp::ControllerV2 ctl;

    int p_psstatus;
    int p_overcurr_l, p_overtemp_l, p_overcurr_r, p_overtemp_r,
        p_pwrstate, p_opmode, p_trigen, p_loopkp, p_loopti,
        p_testwaveperiod, p_testlim_a, p_testlim_b,
        p_pi_sp, p_dac, p_eff_adc, p_eff_dac, p_eff_sp;

  public:
    RtmLamp(int port_number):
      UDriver(
          "RTMLAMP", port_number, &dec,
          ::number_of_channels, p_psstatus, p_psstatus, p_overcurr_l, p_eff_sp,
          {
              {"PS_STATUS", p_psstatus},
          },
          {
              {"AMP_IFLAG_L", p_overcurr_l},
              {"AMP_TFLAG_L", p_overtemp_l},
              {"AMP_IFLAG_R", p_overcurr_r},
              {"AMP_TFLAG_R", p_overtemp_r},
              {"AMP_EN", p_pwrstate},
              {"MODE", p_opmode},
              {"TRIG_EN", p_trigen},
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
        if (auto v = read_sdb(&bars, ctl.device_match, port_number)) {
            dec.set_devinfo(*v);
            ctl.set_devinfo(*v);
        } else {
            throw std::runtime_error("couldn't find lamp module");
        }

        /* XXX: initialize this one specifically to avoid an exception */
        for (unsigned addr = 0; addr < number_of_channels; addr++) {
            setIntegerParam(addr, p_opmode, 0);
        }
    }

    asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value)
    {
        const int function = pasynUser->reason;
        int addr;
        const char *param_name;

        getAddress(pasynUser, &addr);
        getParamName(function, &param_name);

        /* general + channel cover our whole range */
        if (function < first_general_parameter || function > last_channel_parameter) {
            return asynPortDriver::writeInt32(pasynUser, value);
        }

        /* general parameters should have addr=0 */
        if (addr != 0 && function <= last_general_parameter) {
            epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                "writeInt32: %s: general parameter with addr=%d (should be =0)", param_name, addr);

            return asynError;
        }

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

        try {
            ctl.write_params();
        } catch (std::runtime_error &e) {
            epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                "writeInt32: %s: %s", param_name, e.what());

            return asynError;
        }

        return asynSuccess;
    }
};

/* Configuration routine.  Called directly, or from the iocsh function below */
extern "C" {
    /* EPICS iocsh shell commands */
    static const iocshArg initArg0 = {"portNumber", iocshArgInt};
    static const iocshArg *initArgs[] = {&initArg0};
    static const iocshFuncDef initFuncDef = {"RtmLamp", 1, initArgs};
    static void initCallFunc(const iocshArgBuf *args)
    {
        try {
            new RtmLamp(args[0].ival);
        } catch (std::exception &e) {
            cantProceed("error creating rtmlamp: %s\n", e.what());
        }
    }

    static void registerRtmLamp(void)
    {
        iocshRegister(&initFuncDef, initCallFunc);
    }

    epicsExportRegistrar(registerRtmLamp);
}

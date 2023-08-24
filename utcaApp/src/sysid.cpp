#include <algorithm>

#include <modules/sysid.h>

#include "pcie-single.h"
#include "udriver.h"

namespace {
    const unsigned number_of_channels = 12;
}

class SysId: public UDriver {
    sys_id::Core dec;
    sys_id::Controller ctl;

    int p_base_bpm_id, p_prbs_rst, p_prbs_step_duration, p_prbs_lfsr_length,
        p_prbs_bpm_pos_distort_en, p_prbs_setpoint_distort_en, p_sp_movavg;

    int p_setpoint_distortion_lvl0, p_setpoint_distortion_lvl1;
    int p_posx_distortion, p_posy_distortion;

  public:
    /* 2 channels: prbs_0 and prbs_1 in the same parameter */
    SysId(int port_number):
      UDriver(
          "SYSID", port_number, &dec,
          ::number_of_channels,
          {
              {"BASE_BPM_ID", p_base_bpm_id},
              {"PRBS_CTL_RST", p_prbs_rst},
              {"PRBS_CTL_STEP_DURATION", p_prbs_step_duration},
              {"PRBS_CTL_LFSR_LENGTH", p_prbs_lfsr_length},
              {"PRBS_CTL_BPM_POS_DISTORT_EN", p_prbs_bpm_pos_distort_en},
              {"PRBS_CTL_SP_DISTORT_EN", p_prbs_setpoint_distort_en},
              {"SP_DISTORT_MOV_AVG_NUM_TAPS_SEL", p_sp_movavg},
          },
          { }),
      dec(bars),
      ctl(bars)
    {
        auto v = find_device(port_number);
        dec.set_devinfo(v);
        ctl.set_devinfo(v);

        createParam("SP_DISTORT_LVL0", asynParamInt32, &p_setpoint_distortion_lvl0);
        createParam("SP_DISTORT_LVL1", asynParamInt32, &p_setpoint_distortion_lvl1);

        createParam("POS_X_DISTORT", asynParamInt16Array, &p_posx_distortion);
        createParam("POS_Y_DISTORT", asynParamInt16Array, &p_posy_distortion);

        read_parameters();
    }

    asynStatus read_parameters(bool only_monitors=false)
    {
        UDriver::read_parameters(only_monitors);

        for (unsigned addr = 0; addr < number_of_channels; addr++) {
            setIntegerParam(addr, p_setpoint_distortion_lvl0, dec.setpoint_distortion.prbs_0.at(addr));
            setIntegerParam(addr, p_setpoint_distortion_lvl1, dec.setpoint_distortion.prbs_1.at(addr));
        }

        do_callbacks();

        return asynSuccess;
    }

    asynStatus writeInt32Impl(asynUser *pasynUser, const int function,
        [[maybe_unused]] const int addr, const char *param_name, epicsInt32 value)
    {
        if (function == p_base_bpm_id) ctl.base_bpm_id = value;
        else if (function == p_prbs_rst) ctl.prbs_reset = value;
        else if (function == p_prbs_step_duration) ctl.step_duration = value;
        else if (function == p_prbs_lfsr_length) ctl.lfsr_length = value;
        else if (function == p_prbs_bpm_pos_distort_en) ctl.bpm_pos_distort_en = value;
        else if (function == p_prbs_setpoint_distort_en) ctl.sp_distort_en = value;
        else if (function == p_sp_movavg) ctl.sp_mov_avg_samples = value;
        else if (function == p_setpoint_distortion_lvl0) ctl.setpoint_distortion.prbs_0.at(addr) = value;
        else if (function == p_setpoint_distortion_lvl1) ctl.setpoint_distortion.prbs_1.at(addr) = value;

        return write_params(pasynUser, ctl);
    }

    asynStatus writeInt16Array(asynUser *pasynUser, epicsInt16 *value, size_t nElements)
    {
        int function = pasynUser->reason, addr;
        getAddress(pasynUser, &addr);

        const bool is_distortion = function == p_posx_distortion || function == p_posy_distortion;

        auto get_prbs_array =
          [p_posx_distortion = this->p_posx_distortion,
           p_posy_distortion = this->p_posy_distortion](int function, int addr, auto &dec_or_ctl) -> auto & {
            sys_id::distortion_levels *dp;
            if (function == p_posx_distortion) dp = &dec_or_ctl.posx_distortion;
            else if (function == p_posy_distortion) dp = &dec_or_ctl.posy_distortion;

            return addr == 0 ? dp->prbs_0 : dp->prbs_1;
        };

        if (is_distortion) {
            auto &prbs = get_prbs_array(function, addr, ctl);
            std::copy_n(value, nElements, prbs.begin());
        } else {
            return asynPortDriver::writeInt16Array(pasynUser, value, nElements);
        }

        ctl.write_params();
        read_parameters();

        if (is_distortion) {
            auto &prbs = get_prbs_array(function, addr, dec);
            doCallbacksInt16Array(prbs.data(), prbs.size(), function, addr);
        }

        return asynSuccess;
    }
};

constexpr char name[] = "SysId";
UDriverFn<SysId, name> iocsh_init;

extern "C" {
    static void registerSysId()
    {
        iocshRegister(&iocsh_init.init_func_def, iocsh_init.init_call_func);
    }

    epicsExportRegistrar(registerSysId);
}

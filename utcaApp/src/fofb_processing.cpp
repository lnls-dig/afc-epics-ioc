#include <algorithm>

#include <modules/fofb_processing.h>
#include <util_sdb.h>

#include "pcie-single.h"
#include "udriver.h"

namespace {
    const unsigned number_of_channels = 12;
}

class FofbProcessing: public UDriver {
    fofb_processing::Core dec;
    fofb_processing::Controller ctl;

    int p_intlk_clr, p_intlk_en_orb_distort, p_intlk_en_packet_loss,
        p_intlk_sta, p_intlk_orb_distort_limit, p_intlk_min_num_packets,
        p_sp_decim_ratio_max;
    int p_acc_clr, p_acc_freeze,
        p_sp_limits_max, p_sp_limits_min,
        p_sp_decim_data, p_sp_decim_ratio;

    int p_reforb_x, p_reforb_y;
    int p_acc_gain, p_coeffs_x, p_coeffs_y;

  public:
    FofbProcessing(int port_number):
      UDriver(
          "FOFB_PROCESSING", port_number, &dec,
          ::number_of_channels,
          {
              {"INTLK_CTL_CLR", p_intlk_clr},
              {"INTLK_CTL_SRC_EN_ORB_DISTORT", p_intlk_en_orb_distort},
              {"INTLK_CTL_SRC_EN_PACKET_LOSS", p_intlk_en_packet_loss},
              {"INTLK_STA", p_intlk_sta},
              {"INTLK_ORB_DISTORT_LIMIT", p_intlk_orb_distort_limit},
              {"INTLK_MIN_NUM_PACKETS", p_intlk_min_num_packets},
              {"SP_DECIM_RATIO_MAX", p_sp_decim_ratio_max},
          },
          {
              {"CH_ACC_CTL_CLEAR", p_acc_clr},
              {"CH_ACC_CTL_FREEZE", p_acc_freeze},
              {"CH_ACC_LIMITS_MAX", p_sp_limits_max},
              {"CH_ACC_LIMITS_MIN", p_sp_limits_min},
              {"CH_SP_DECIM_DATA", p_sp_decim_data},
              {"CH_SP_DECIM_RATIO", p_sp_decim_ratio},
          }),
      dec(bars),
      ctl(bars)
    {
        if (auto v = read_sdb(&bars, ctl.device_match, port_number)) {
            dec.set_devinfo(*v);
            ctl.set_devinfo(*v);
        } else {
            throw std::runtime_error("couldn't find fofb_processing module");
        }

        createParam("REF_ORBIT_X", asynParamInt32Array, &p_reforb_x);
        createParam("REF_ORBIT_Y", asynParamInt32Array, &p_reforb_y);

        createParam("CH_ACC_GAIN", asynParamFloat64, &p_acc_gain);
        createParam("CH_COEFFS_X", asynParamFloat64Array, &p_coeffs_x);
        createParam("CH_COEFFS_Y", asynParamFloat64Array, &p_coeffs_y);

        write_only = {p_intlk_clr, p_acc_clr};

        read_parameters();
    }

    asynStatus read_parameters()
    {
        UDriver::read_parameters();

        for (unsigned addr = 0; addr < ::number_of_channels; addr++)
            setDoubleParam(addr, p_acc_gain, dec.get_channel_data<double>("CH_ACC_GAIN", addr));

        return asynSuccess;
    }

    asynStatus writeInt32Impl(asynUser *pasynUser, const int function, const int addr, const char *param_name, epicsInt32 value)
    {
        if (function == p_intlk_clr) ctl.intlk_sta_clr = value;
        else if (function == p_intlk_en_orb_distort) ctl.intlk_en_orb_distort = value;
        else if (function == p_intlk_en_packet_loss) ctl.intlk_en_packet_loss = value;
        else if (function == p_intlk_orb_distort_limit) ctl.orb_distort_limit = value;
        else if (function == p_intlk_min_num_packets) ctl.min_num_packets = value;
        /* per-channel parameters */
        else if (function == p_acc_clr) ctl.parameters[addr].acc_clear = value;
        else if (function == p_acc_freeze) ctl.parameters[addr].acc_freeze = value;
        else if (function == p_sp_limits_max) ctl.parameters[addr].sp_limit_max = value;
        else if (function == p_sp_limits_min) ctl.parameters[addr].sp_limit_min = value;
        else if (function == p_sp_decim_ratio) ctl.parameters[addr].sp_decim_ratio = value;

        try {
            ctl.write_params();
        } catch (std::runtime_error &e) {
            epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                "writeInt32: %s: %s", param_name, e.what());

            return asynError;
        }

        /* refresh parameters */
        read_parameters();

        return asynSuccess;
    }

    asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value)
    {
        int function = pasynUser->reason, addr;
        getAddress(pasynUser, &addr);

        if (function == p_acc_gain) {
            ctl.parameters[addr].acc_gain = value;

        } else {
            return asynPortDriver::writeFloat64(pasynUser, value);
        }

        ctl.write_params();
        read_parameters();

        return asynSuccess;
    }

    asynStatus writeInt32Array(asynUser *pasynUser, epicsInt32 *value, size_t nElements)
    {
        int function = pasynUser->reason, addr;
        getAddress(pasynUser, &addr);

        if ((function == p_reforb_x || function == p_reforb_y) && addr != 0)
            throw std::logic_error("reference orbit parameter with addr!=0");

        if (function == p_reforb_x) {
            std::copy_n(value, nElements, ctl.ref_orb_x.begin());
        } else if (function == p_reforb_y) {
            std::copy_n(value, nElements, ctl.ref_orb_y.begin());
        } else {
            return asynPortDriver::writeInt32Array(pasynUser, value, nElements);
        }

        ctl.write_params();
        read_parameters();

        if (function == p_reforb_x)
            doCallbacksInt32Array(dec.ref_orb_x.data(), dec.ref_orb_x.size(), p_reforb_x, 0);
        if (function == p_reforb_y)
            doCallbacksInt32Array(dec.ref_orb_y.data(), dec.ref_orb_y.size(), p_reforb_y, 0);

        return asynSuccess;
    }

    asynStatus writeFloat64Array(asynUser *pasynUser, epicsFloat64 *value, size_t nElements)
    {
        int function = pasynUser->reason, addr;
        getAddress(pasynUser, &addr);

        if (function == p_coeffs_x) {
            std::copy_n(value, nElements, ctl.parameters[addr].coefficients_x.begin());
        } else if (function == p_coeffs_y) {
            std::copy_n(value, nElements, ctl.parameters[addr].coefficients_y.begin());
        } else {
            return asynPortDriver::writeFloat64Array(pasynUser, value, nElements);
        }

        ctl.write_params();
        read_parameters();

        if (function == p_coeffs_x)
            doCallbacksFloat64Array(dec.coefficients_x[addr].data(), dec.coefficients_x[addr].size(), p_coeffs_x, addr);
        if (function == p_coeffs_y)
            doCallbacksFloat64Array(dec.coefficients_y[addr].data(), dec.coefficients_y[addr].size(), p_coeffs_y, addr);

        return asynSuccess;
    }
};

constexpr char name[] = "FofbProcessing";
UDriverFn<FofbProcessing, name> iocsh_init;

extern "C" {
    static void registerFofbProcessing()
    {
        iocshRegister(&iocsh_init.init_func_def, iocsh_init.init_call_func);
    }

    epicsExportRegistrar(registerFofbProcessing);
}

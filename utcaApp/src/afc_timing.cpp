#include <alarm.h>
#include <epicsString.h>

#include <modules/afc_timing.h>

#include "pcie-single.h"
#include "udriver.h"

namespace {
  constexpr unsigned number_of_channels = 18;
}

class AFCTiming: public UDriver {
    afc_timing::Core dec;
    afc_timing::Controller ctl;

    int p_first_lock, p_last_lock;
    int p_ch_src;
    int p_freq;

  public:
    AFCTiming(int port_number):
      UDriver(
          "AFC_TIMING", port_number, &dec,
          ::number_of_channels,
          {
              {"LINK", p_read_only},
              {"RXEN", p_read_only},
              {"EVREN", p_decoder_controller},
              {"LOCKED_AFC_FREQ", p_first_lock},
              {"LOCKED_AFC_PHASE", p_read_only},
              {"LOCKED_RTM_FREQ", p_read_only},
              {"LOCKED_RTM_PHASE", p_read_only},
              {"LOCKED_GT0", p_read_only},
              {"LOCKED_AFC_FREQ_LTC", p_read_only},
              {"LOCKED_AFC_PHASE_LTC", p_read_only},
              {"LOCKED_RTM_FREQ_LTC", p_read_only},
              {"LOCKED_RTM_PHASE_LTC", p_read_only},
              {"LOCKED_GT0_LTC", p_last_lock},
              {"ALIVE", p_read_only},
              ParamInit{"RST_LOCKED_LTCS", p_decoder_controller}.set_wo(),
          },
          {
              ParamInit{"RFREQ_HI", p_read_only}.set_nc(2),
              ParamInit{"RFREQ_LO", p_read_only}.set_nc(2),
              ParamInit{"N1", p_read_only}.set_nc(2),
              ParamInit{"HS_DIV", p_read_only}.set_nc(2),
              ParamInit{"FREQ_KP", p_decoder_controller}.set_nc(2),
              ParamInit{"FREQ_KI", p_decoder_controller}.set_nc(2),
              ParamInit{"PHASE_KP", p_decoder_controller}.set_nc(2),
              ParamInit{"PHASE_KI", p_decoder_controller}.set_nc(2),
              ParamInit{"MAF_NAVG", p_decoder_controller}.set_nc(2),
              ParamInit{"MAF_DIV_EXP", p_decoder_controller}.set_nc(2),

              {"CH_EN", p_decoder_controller},
              {"CH_POL", p_decoder_controller},
              {"CH_LOG", p_decoder_controller},
              {"CH_ITL", p_decoder_controller},
              {"CH_SRC", p_ch_src},
              {"CH_DIR", p_decoder_controller},
              {"CH_PULSES", p_decoder_controller},
              ParamInit{"CH_COUNT_RST", p_decoder_controller}.set_wo(),
              {"CH_COUNT", p_read_only},
              {"CH_EVT", p_decoder_controller},
              {"CH_DLY", p_decoder_controller},
              {"CH_WDT", p_decoder_controller},
          }, &ctl),
      dec(bars),
      ctl(bars)
    {
        auto v = find_device(port_number);
        dec.set_devinfo(v);
        ctl.set_devinfo(v);

        parameter_props.at(p_ch_src).write_decoder_controller = true;

        createParam("FREQ", asynParamFloat64, &p_freq);

        read_parameters();
    }

    asynStatus readEnum(asynUser *pasynUser, char *strings[], int values[],
        int severities[], size_t, size_t *nIn)
    {
        const int function = pasynUser->reason;

        if (function == p_ch_src) {
            return fill_enum(strings, values, severities, nIn, ctl.sources_list);
        } else if (function >= p_first_lock && function <= p_last_lock) {
            static const char *off_on[] = {"off", "on"};
            for (auto &&[i, name]: enumerate(off_on)) {
                strings[i] = epicsStrDup(name);
                values[i] = i;
                severities[i] = i == 0 ? MAJOR_ALARM : NO_ALARM;
                *nIn = i+1;
            }
        } else {
            return asynError;
        }

        return asynSuccess;
    }

    asynStatus writeFloat64Impl(asynUser *pasynUser, const int function, const int addr, epicsFloat64 value)
    {
        if (function == p_freq) {
            bool rv;
            if (addr == 0) rv = ctl.set_rtm_freq(value);
            else rv = ctl.set_afc_freq(value);

            if (!rv) return asynError;

            setDoubleParam(addr, function, value);
            callParamCallbacks(addr);
        }

        return write_params(pasynUser, ctl);
    }
};

constexpr char name[] = "AfcTiming";
UDriverFn<AFCTiming, name> iocsh_init;

extern "C" {
    static void registerAfcTiming()
    {
        iocshRegister(&iocsh_init.init_func_def, iocsh_init.init_call_func);
    }

    epicsExportRegistrar(registerAfcTiming);
}

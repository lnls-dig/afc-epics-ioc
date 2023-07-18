#include <alarm.h>
#include <epicsString.h>

#include <modules/afc_timing.h>
#include <util_sdb.h>

#include "pcie-single.h"
#include "udriver.h"

namespace {
  constexpr unsigned number_of_channels = 18;
}

class AFCTiming: public UDriver {
    afc_timing::Core dec;
    afc_timing::Controller ctl;

    int p_link, p_rxen, p_refclock_lock, p_evren, p_alive;
    int p_rfreq_hi, p_rfreq_lo, p_n1, p_hs_div,
        p_freq_kp, p_freq_ki, p_phase_kp, p_phase_ki,
        p_maf_navg, p_maf_divexp;
    int p_ch_en, p_ch_pol, p_ch_log, p_ch_itl, p_ch_src, p_ch_dir, p_ch_pulses,
        p_ch_count_rst, p_ch_count, p_ch_evt, p_ch_dly, p_ch_wdt;

    int p_freq;

  public:
    AFCTiming(int port_number):
      UDriver(
          "AFC_TIMING", port_number, &dec,
          ::number_of_channels,
          {
              {"STA_LINK", p_link},
              {"STA_RXEN", p_rxen},
              {"STA_REFCLKLOCK", p_refclock_lock},
              {"STA_EVREN", p_evren},
              {"ALIVE", p_alive},
          },
          {
              {"RFREQ_HI", p_rfreq_hi},
              {"RFREQ_LO", p_rfreq_lo},
              {"N1", p_n1},
              {"HS_DIV", p_hs_div},
              {"FREQ_KP", p_freq_kp},
              {"FREQ_KI", p_freq_ki},
              {"PHASE_KP", p_phase_kp},
              {"PHASE_KI", p_phase_ki},
              {"MAF_NAVG", p_maf_navg},
              {"MAF_DIV_EXP", p_maf_divexp},

              {"CH_EN", p_ch_en},
              {"CH_POL", p_ch_pol},
              {"CH_LOG", p_ch_log},
              {"CH_ITL", p_ch_itl},
              {"CH_SRC", p_ch_src},
              {"CH_DIR", p_ch_dir},
              {"CH_PULSES", p_ch_pulses},
              {"CH_COUNT_RST", p_ch_count_rst},
              {"CH_COUNT", p_ch_count},
              {"CH_EVT", p_ch_evt},
              {"CH_DLY", p_ch_dly},
              {"CH_WDT", p_ch_wdt},
          }),
      dec(bars),
      ctl(bars)
    {
        if (auto v = read_sdb(&bars, ctl.device_match, port_number)) {
            dec.set_devinfo(*v);
            ctl.set_devinfo(*v);
        } else {
            throw std::runtime_error("couldn't find afc_timing module");
        }

        createParam("FREQ", asynParamFloat64, &p_freq);

        write_only = {p_ch_count_rst};

        read_parameters();
    }

    asynStatus readEnum(asynUser *pasynUser, char *strings[], int values[],
        int severities[], [[maybe_unused]] size_t nElements, size_t *nIn)
    {
        const int function = pasynUser->reason;

        if (function == p_ch_src) {
            for (auto &&[i, name]: enumerate(ctl.sources_list)) {
                strings[i] = epicsStrDup(name.data());
                values[i] = i;
                severities[i] = NO_ALARM;

                *nIn = i+1;
            }
        } else {
            *nIn = 0;
            return asynError;
        }

        return asynSuccess;
    }

    asynStatus writeInt32Impl(asynUser *pasynUser, const int function, const int addr, const char *param_name, epicsInt32 value)
    {
        if (function == p_evren) {
            ctl.event_receiver_enable = value;
        } else if (function >= p_freq_kp && function <= p_maf_divexp) {
            auto &clock = addr == 0 ? ctl.rtm_clock : ctl.afc_clock;

            if (function == p_freq_kp) clock.freq_loop.kp = value;
            else if (function == p_freq_ki) clock.freq_loop.ki = value;
            else if (function == p_phase_kp) clock.phase_loop.kp = value;
            else if (function == p_phase_ki) clock.phase_loop.ki = value;
            else if (function == p_maf_navg) clock.ddmtd_config.navg = value;
            else if (function == p_maf_divexp) clock.ddmtd_config.div_exp = value;
        } else if (function >= p_ch_en && function <= p_ch_wdt) {
            auto &trigger = ctl.parameters.at(addr);

            if (function == p_ch_en) trigger.enable = value;
            else if (function == p_ch_pol) trigger.polarity = value;
            else if (function == p_ch_itl) trigger.interlock = value;
            else if (function == p_ch_src) trigger.source = ctl.sources_list[value];
            else if (function == p_ch_dir) trigger.direction = value;
            else if (function == p_ch_pulses) trigger.pulses = value;
            else if (function == p_ch_count_rst) trigger.count_reset = value;
            else if (function == p_ch_evt) trigger.event_code = value;
            else if (function == p_ch_dly) trigger.delay = value;
            else if (function == p_ch_wdt) trigger.width = value;
        }

        try {
            ctl.write_params();
        } catch (std::runtime_error &e) {
            epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                "writeInt32: %s: %s", param_name, e.what());

            return asynError;
        }

        read_parameters();

        return asynSuccess;
    }

    asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value)
    {
        int function = pasynUser->reason, addr;
        getAddress(pasynUser, &addr);

        if (function == p_freq) {
            bool rv;
            if (addr == 0) rv = ctl.set_rtm_freq(value);
            else rv = ctl.set_afc_freq(value);

            if (!rv) return asynError;

            setDoubleParam(addr, function, value);
            callParamCallbacks(addr);
        } else {
            return asynPortDriver::writeFloat64(pasynUser, value);
        }

        ctl.write_params();
        read_parameters();

        return asynSuccess;
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

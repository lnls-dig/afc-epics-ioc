#include <thread>

#include <epicsEvent.h>
#include <epicsThread.h>

#include <modules/pos_calc.h>

#include "pcie-single.h"
#include "udriver.h"

namespace {
    const unsigned number_of_channels = 4;
}

class PosCalc: public UDriver, public epicsThreadRunable {
    pos_calc::Core dec;
    pos_calc::Controller ctl;

    int p_monit_amp, p_ampfifo;
    int p_update_time;

    epicsEvent thread_kill_event;
    epicsThread thread;

  public:
    PosCalc(int port_number):
      UDriver(
          "POS_CALC", port_number, &dec,
          ::number_of_channels,
          {
              {"KX", p_decoder_controller},
              {"KY", p_decoder_controller},
              {"KSUM", p_decoder_controller, asynParamFloat64},
              {"TEST_DATA", p_decoder_controller},
              {"FOFB_SYNC_EN", p_decoder_controller},
              {"FOFB_DESYNC_CNT", p_read_only},
              ParamInit{"FOFB_DESYNC_CNT_RST", p_decoder_controller}.set_wo(),
              {"FOFB_DATA_MASK_EN", p_decoder_controller},
              {"FOFB_DATA_MASK_SAMPLES", p_decoder_controller},
              {"OFFSET_X", p_decoder_controller},
              {"OFFSET_Y", p_decoder_controller},
          },
          {
              ParamInit{"SYNC_EN", p_decoder_controller}.set_nc(3),
              ParamInit{"SYNC_DLY", p_decoder_controller}.set_nc(3),
              ParamInit{"DESYNC_CNT", p_read_only}.set_nc(3),
              ParamInit{"DESYNC_CNT_RST", p_decoder_controller}.set_nc(3).set_wo(),
              ParamInit{"DATA_MASK_EN", p_decoder_controller}.set_nc(3),
              ParamInit{"DATA_MASK_SAMPLES_BEG", p_decoder_controller}.set_nc(3),
              ParamInit{"DATA_MASK_SAMPLES_END", p_decoder_controller}.set_nc(3),
              {"ADC_SWCLK_INV_GAIN", p_decoder_controller, asynParamFloat64},
              {"ADC_SWCLK_DIR_GAIN", p_decoder_controller, asynParamFloat64},
              {"ADC_SWCLK_INV_OFFSET", p_decoder_controller},
              {"ADC_SWCLK_DIR_OFFSET", p_decoder_controller},
          },
          &ctl),
      dec(bars),
      ctl(bars),
      thread(*this, (std::string(portName) + "-monit").c_str(), epicsThreadGetStackSize(epicsThreadStackMedium), epicsThreadPriorityHigh)
    {
        auto v = find_device(port_number);
        dec.set_devinfo(v);
        ctl.set_devinfo(v);

        createParam("AMPFIFO_MONIT_AMP", asynParamInt32, &p_monit_amp);
        createParam("AMPFIFO", asynParamInt32, &p_ampfifo);

        createParam("UPDATE_TIME", asynParamFloat64, &p_update_time);

        read_parameters();

        thread.start();
    }

    ~PosCalc()
    {
        thread_kill_event.trigger();
    }

    asynStatus writeFloat64Impl(asynUser *pasynUser, const int, const int, epicsFloat64 value)
    {
        return asynPortDriver::writeFloat64(pasynUser, value);
    }

    void run()
    {
        uint32_t ampfifo = 1;

        auto get_update_time = [this]() {
            std::lock_guard g(*this);
            double update_time;
            getDoubleParam(p_update_time, &update_time);
            return update_time;
        };

        auto put_fifo_amps = [this, &ampfifo]() {
            std::lock_guard g(*this);

            for (unsigned addr = 0; addr < this->number_of_channels; addr++)
                setIntegerParam(addr, p_monit_amp, std::get<int32_t>(dec.get_generic_data("AMPFIFO_MONIT_AMP", addr)));

            setIntegerParam(p_ampfifo, ampfifo++);
            callParamCallbacks(0);
        };

        while (true) {
            while (dec.fifo_empty()) {
                if (thread_kill_event.tryWait()) return;
                epicsThreadSleep(get_update_time());
            }
            dec.get_fifo_amps();

            put_fifo_amps();
        }
    }
};

constexpr char name[] = "PosCalc";
UDriverFn<PosCalc, name> iocsh_init;

extern "C" {
    static void registerPosCalc()
    {
        iocshRegister(&iocsh_init.init_func_def, iocsh_init.init_call_func);
    }

    epicsExportRegistrar(registerPosCalc);
}

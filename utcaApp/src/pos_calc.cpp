#include <modules/pos_calc.h>

#include "pcie-single.h"
#include "udriver.h"

namespace {
    const unsigned number_of_channels = 4;
}

class PosCalc: public UDriver {
    pos_calc::Core dec;
    pos_calc::Controller ctl;

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
      ctl(bars)
    {
        auto v = find_device(port_number);
        dec.set_devinfo(v);
        ctl.set_devinfo(v);

        read_parameters();
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

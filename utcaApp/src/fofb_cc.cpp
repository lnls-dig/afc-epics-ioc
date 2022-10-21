#include <stdexcept>
#include <string>
#include <tuple>

#include <asynPortDriver.h>

#include "fofb_cc.h"

#include "pcie-single.h"
#include "udriver.h"

namespace {
    const unsigned number_of_channels = 8;
}

class FofbCc: public UDriver {
    fofb_cc::Core dec;
    fofb_cc::Controller ctl;

    int p_err_clr, p_cc_enable, p_tfs_override,
        p_toa_rd_en, p_toa_rd_str, p_toa_data,
        p_rcb_rd_en, p_rcb_rd_str, p_rcb_data,
        p_bpm_id, p_time_frame_len, p_mgt_powerdown, p_mgt_loopback,
        p_time_frame_delay, p_rx_polarity, p_payload_sel, p_fofb_data_sel,
        p_firmware_ver, p_sys_status, p_link_up, p_time_frame_cnt,
        p_fod_process_time, p_bpm_cnt;
    int p_link_partner, p_hard_err_cnt, p_soft_err_cnt, p_frame_err_cnt,
        p_rx_pck_cnt, p_tx_pck_cnt;

  public:
    FofbCc(int port_number):
      UDriver(
          "FOFB_CC", port_number, &dec,
          ::number_of_channels, p_err_clr, p_bpm_cnt, p_link_partner, p_tx_pck_cnt,
          {
              {"ERR_CLR", p_err_clr},
              {"CC_ENABLE", p_cc_enable},
              {"TFS_OVERRIDE", p_tfs_override},
              {"TOA_RD_EN", p_toa_rd_en},
              {"TOA_RD_STR", p_toa_rd_str},
              {"TOA_DATA", p_toa_data},
              {"RCB_RD_EN", p_rcb_rd_en},
              {"RCB_RD_STR", p_rcb_rd_str},
              {"RCB_DATA", p_rcb_data},
              {"BPM_ID", p_bpm_id},
              {"TIME_FRAME_LEN", p_time_frame_len},
              {"MGT_POWERDOWN", p_mgt_powerdown},
              {"MGT_LOOPBACK", p_mgt_loopback},
              {"TIME_FRAME_DLY", p_time_frame_delay},
              {"RX_POLARITY", p_rx_polarity},
              {"PAYLOAD_SEL", p_payload_sel},
              {"FOFB_DATA_SEL", p_fofb_data_sel},
              {"FIRMWARE_VER", p_firmware_ver},
              {"SYS_STATUS", p_sys_status},
              {"LINK_UP", p_link_up},
              {"TIME_FRAME_CNT", p_time_frame_cnt},
              {"FOD_PROCESS_TIME", p_fod_process_time},
              {"BPM_CNT", p_bpm_cnt},
          },
          {
              {"LINK_PARTNER", p_link_partner},
              {"HARD_ERR_CNT", p_hard_err_cnt},
              {"SOFT_ERR_CNT", p_soft_err_cnt},
              {"FRAME_ERR_CNT", p_frame_err_cnt},
              {"RX_PCK_CNT", p_rx_pck_cnt},
              {"TX_PCK_CNT", p_tx_pck_cnt},
          }),
      dec(bars),
      ctl(bars)
    {
        if (auto v = read_sdb(&bars, ctl.device_match, port_number)) {
            dec.set_devinfo(*v);
            ctl.set_devinfo(*v);
        } else {
            throw std::runtime_error("couldn't find fofb_cc module");
        }

        write_only = {p_err_clr, p_toa_rd_str, p_rcb_rd_str};
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

        auto write_param = [addr, this](auto fn, auto &ctl_value) {
            epicsInt32 tmp;
            /* we expect success, unless the parameter hasn't been initialized yet */
            if (getIntegerParam(addr, fn, &tmp) == asynSuccess)
                ctl_value = tmp;
            else
                ctl_value = std::nullopt;
        };

        write_param(p_err_clr, ctl.err_clr);
        write_param(p_cc_enable, ctl.cc_enable);
        write_param(p_tfs_override, ctl.tfs_override);
        write_param(p_toa_rd_en, ctl.toa_rd_en);
        write_param(p_toa_rd_str, ctl.toa_rd_str);
        write_param(p_rcb_rd_en, ctl.rcb_rd_en);
        write_param(p_rcb_rd_str, ctl.rcb_rd_str);
        write_param(p_bpm_id, ctl.bpm_id);
        write_param(p_time_frame_len, ctl.time_frame_len);
        write_param(p_mgt_powerdown, ctl.mgt_powerdown);
        write_param(p_mgt_loopback, ctl.mgt_loopback);
        write_param(p_time_frame_delay, ctl.time_frame_delay);
        write_param(p_rx_polarity, ctl.rx_polarity);
        write_param(p_payload_sel, ctl.payload_sel);
        write_param(p_fofb_data_sel, ctl.fofb_data_sel);

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

constexpr char name[] = "FofbCc";
UDriverFn<FofbCc, name> iocsh_init;

extern "C" {
    static void registerFofbCc()
    {
        iocshRegister(&iocsh_init.init_func_def, iocsh_init.init_call_func);
    }

    epicsExportRegistrar(registerFofbCc);
}

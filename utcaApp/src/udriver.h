#include <tuple>
#include <vector>

#include <cantProceed.h>
#include <epicsExport.h>
#include <iocsh.h>

#include <asynPortDriver.h>

#include "decoders.h"

class UDriver: public asynPortDriver {
  protected:
    RegisterDecoder *generic_decoder;

    unsigned number_of_channels;
    const int &first_general_parameter, &last_general_parameter;
    const int &first_channel_parameter, &last_channel_parameter;

    int p_scan_task;

    std::vector<std::tuple<const char *, int &>> name_and_index;
    std::vector<std::tuple<const char *, int &>> name_and_index_channel;

    virtual asynStatus writeInt32(asynUser *, epicsInt32) = 0;

    UDriver(
        const char *name, int port_number, RegisterDecoder *generic_decoder,
        unsigned number_of_channels, int &fgp, int &lgp, int &fcp, int &lcp,
        std::vector<std::tuple<const char *, int &>> name_and_index,
        std::vector<std::tuple<const char *, int &>> name_and_index_channel):
      asynPortDriver(
          (std::string(name) + "-" + std::to_string(port_number)).c_str(),
          number_of_channels, /* channels */
          -1, -1, /* enable all interfaces and interrupts */
          0, 1, 0, 0 /* no flags, auto connect, default priority and stack size */
      ),
      generic_decoder(generic_decoder),
      number_of_channels(number_of_channels),
      first_general_parameter(fgp), last_general_parameter(lgp),
      first_channel_parameter(fcp), last_channel_parameter(lcp),
      name_and_index(name_and_index), name_and_index_channel(name_and_index_channel)
    {
        /* here, we don't call setIntegerParam to initialize these parameters:
         * - readback ones are expected to be initialized soon via SCAN
         * - setpoint ones are expected to be initialized whenever their records
         *   are written into: this also allows us to avoid writing into fields
         *   which aren't always supported, as long as the PVs haven't been written
         *   into */

        for (auto &v: name_and_index) {
            createParam(std::get<0>(v), asynParamInt32, &std::get<1>(v));
        }

        for (auto &v: name_and_index_channel) {
            createParam(std::get<0>(v), asynParamInt32, &std::get<1>(v));
        }

        createParam("SCAN_TASK", asynParamInt32, &p_scan_task);
    }

    virtual asynStatus read_parameters()
    {
        generic_decoder->read();

        for (int p = first_general_parameter; p <= last_channel_parameter; p++) {
            const char *param_name;
            getParamName(p, &param_name);

            if (p <= last_general_parameter) {
                setIntegerParam(0, p, generic_decoder->get_general_data(param_name));
            } else {
                for (unsigned addr = 0; addr < number_of_channels; addr++)
                    setIntegerParam(addr, p, generic_decoder->get_channel_data(param_name, addr));
            }
        }

        for (unsigned addr = 0; addr < number_of_channels; addr++)
            callParamCallbacks(addr);

        return asynSuccess;
    }

    asynStatus readInt32(asynUser *pasynUser, epicsInt32 *value)
    {
        const int function = pasynUser->reason;

        *value = 0;
        if (function == p_scan_task) return read_parameters();
        else return asynPortDriver::readInt32(pasynUser, value);
    }
};

/* singleton helper for creating iocsh functions for each driver */
template <class T, const char *name>
class UDriverFn {
  public:
    static constexpr iocshArg init_arg0 {"portNumber", iocshArgInt};
    static constexpr iocshArg const *init_args[1] = {&init_arg0};
    static constexpr iocshFuncDef init_func_def {
        name,
        1,
        init_args
#ifdef IOCSHFUNCDEF_HAS_USAGE
        ,"Create a new port with portNumber index\n"
#endif
    };

    static void init_call_func(const iocshArgBuf *args)
    {
        try {
            new T(args[0].ival);
        } catch (std::exception &e) {
            cantProceed("error creating %s: %s\n", name, e.what());
        }
    }
};

#include <tuple>
#include <unordered_set>
#include <vector>

#include <cantProceed.h>
#include <epicsExport.h>
#include <iocsh.h>

#include <asynPortDriver.h>

#include "enumerate.h"
#include "decoders.h"

class UDriver: public asynPortDriver {
    RegisterDecoder *generic_decoder;

    int p_scan_task, p_counter;
    unsigned scan_counter = 0;

    int first_general_parameter = -1, last_general_parameter = -1,
        first_channel_parameter = -1, last_channel_parameter = -1;

    std::vector<std::tuple<const char *, int &>> name_and_index;
    std::vector<std::tuple<const char *, int &>> name_and_index_channel;

  protected:
    unsigned number_of_channels;
    int port_number;

    std::unordered_set<int> write_only;

    UDriver(
        const char *name, int port_number, RegisterDecoder *generic_decoder,
        unsigned number_of_channels,
        std::vector<std::tuple<const char *, int &>> name_and_index,
        std::vector<std::tuple<const char *, int &>> name_and_index_channel):
      asynPortDriver(
          (std::string(name) + "-" + std::to_string(port_number)).c_str(),
          number_of_channels, /* channels */
          -1, -1, /* enable all interfaces and interrupts */
          0, 1, 0, 0 /* no flags, auto connect, default priority and stack size */
      ),
      generic_decoder(generic_decoder),
      name_and_index(name_and_index), name_and_index_channel(name_and_index_channel),
      number_of_channels(number_of_channels), port_number(port_number)
    {
        /* here, we don't call setIntegerParam to initialize these parameters:
         * - readback ones are expected to be initialized soon via SCAN
         * - setpoint ones are expected to be initialized whenever their records
         *   are written into: this also allows us to avoid writing into fields
         *   which aren't always supported, as long as the PVs haven't been written
         *   into */

        createParam("SCAN_TASK", asynParamInt32, &p_scan_task);

        createParam("COUNTER", asynParamInt32, &p_counter);
        setIntegerParam(p_counter, 0);

        auto create_params = [this](auto const &nai, auto &first_param, auto &last_param) {
            for (auto &&[i, v]: enumerate(nai)) {
                auto &&[str, p] = v;

                createParam(str, asynParamInt32, &p);

                if (i == 0) first_param = p;
                else if (i == nai.size() - 1) last_param = p;
            }
        };

        create_params(name_and_index, first_general_parameter, last_general_parameter);
        create_params(name_and_index_channel, first_channel_parameter, last_channel_parameter);
    }

    virtual asynStatus read_parameters(bool only_monitors=false)
    {
        generic_decoder->get_data(only_monitors);

        auto get_params = [this](auto first_param, auto last_param, auto fn) {
            if (first_param < 0) return;

            for (int p = first_param; p <= last_param; p++) {
                if (write_only.count(p)) continue;

                const char *param_name;
                getParamName(p, &param_name);

                fn(p, param_name);
            }
        };

        get_params(first_general_parameter, last_general_parameter,
            [this](int p, const char *param_name)
            { setIntegerParam(0, p, generic_decoder->get_general_data<int32_t>(param_name)); });
        get_params(first_channel_parameter, last_channel_parameter,
            [this](int p, const char *param_name)
            {
                for (unsigned addr = 0; addr < number_of_channels; addr++)
                    setIntegerParam(addr, p, generic_decoder->get_channel_data<int32_t>(param_name, addr));
            });

        epicsInt32 counter;
        getIntegerParam(0, p_counter, &counter);
        setIntegerParam(0, p_counter, counter+1);

        do_callbacks();

        return asynSuccess;
    }

    void do_callbacks()
    {
        for (unsigned addr = 0; addr < number_of_channels; addr++)
            callParamCallbacks(addr);
    }

    asynStatus readInt32(asynUser *pasynUser, epicsInt32 *value) override
    {
        const int function = pasynUser->reason;

        if (function == p_scan_task) {
            *value = scan_counter++;
            return read_parameters(true);
        }
        else return asynPortDriver::readInt32(pasynUser, value);
    }

    virtual asynStatus writeInt32Impl(asynUser *pasynUser, const int function, const int addr, const char *param_name, epicsInt32 value) = 0;
    asynStatus writeInt32(asynUser *pasynUser, epicsInt32 value) override final
    {
        const int function = pasynUser->reason;
        int addr;
        const char *param_name;

        getAddress(pasynUser, &addr);
        getParamName(function, &param_name);

        if (function < p_scan_task) {
            return asynPortDriver::writeInt32(pasynUser, value);
        }

        if (first_general_parameter >= 0 && addr != 0 && function <= last_general_parameter) {
            epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                "writeInt32: %s: general parameter with addr=%d (should be 0)", param_name, addr);

            return asynError;
        }

        return writeInt32Impl(pasynUser, function, addr, param_name, value);
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

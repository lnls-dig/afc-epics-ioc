#include <tuple>
#include <unordered_set>
#include <vector>

#include <version>
#if __cpp_lib_source_location >= 201907L
# include <source_location>
# define HAS_SOURCE_LOCATION
#endif

#include <cantProceed.h>
#include <epicsExport.h>
#include <iocsh.h>

#include <asynPortDriver.h>

#include <controllers.h>
#include <decoders.h>
#include <util_sdb.h>

#include "enumerate.h"
#include "pcie-single.h"

class UDriver: public asynPortDriver {
    RegisterDecoder *generic_decoder;
    RegisterDecoderController *generic_decoder_controller;

    int p_scan_task, p_counter;
    unsigned scan_counter = 0;

    int first_general_parameter = -1, last_general_parameter = -1,
        first_channel_parameter = -1, last_channel_parameter = -1;

  protected:
    static inline int p_read_only = 0, p_decoder_controller = 0;
    unsigned number_of_channels;
    int port_number;

    std::unordered_set<int> decoder_controller;
    std::unordered_set<int> write_only;

    UDriver(
        const char *name, int port_number, RegisterDecoder *generic_decoder,
        unsigned number_of_channels,
        const std::vector<std::tuple<const char *, int &>> &name_and_index,
        const std::vector<std::tuple<const char *, int &>> &name_and_index_channel,
        RegisterDecoderController *generic_decoder_controller=nullptr):
      asynPortDriver(
          (std::string(name) + "-" + std::to_string(port_number)).c_str(),
          number_of_channels, /* channels */
          -1, -1, /* enable all interfaces and interrupts */
          0, 1, 0, 0 /* no flags, auto connect, default priority and stack size */
      ),
      generic_decoder(generic_decoder),
      generic_decoder_controller(generic_decoder_controller),
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
                int p_tmp;
                auto &&[str, p] = v;

                /* if p is p_read_only or p_decoder_controller, we use a
                 * temporary variable to avoid any issues with simultaneous
                 * access to them */
                bool use_tmp = &p == &p_read_only || &p == &p_decoder_controller;
                int *pp = use_tmp ? &p_tmp : &p;
                createParam(str, asynParamInt32, pp);

                if (&p == &p_decoder_controller) {
                    assert(this->generic_decoder_controller);
                    decoder_controller.insert(p_tmp);
                }

                if (i == 0) first_param = *pp;
                else if (i == nai.size() - 1) last_param = *pp;
            }
        };

        create_params(name_and_index, first_general_parameter, last_general_parameter);
        create_params(name_and_index_channel, first_channel_parameter, last_channel_parameter);
    }

    sdb_device_info find_device(int port_number)
    {
        if (auto v = read_sdb(&bars, generic_decoder->match_devinfo_lambda, port_number))
            return *v;
        else
            throw std::runtime_error("couldn't find module");
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

    asynStatus write_params(
        asynUser *pasynUser, RegisterController &ctl
#ifdef HAS_SOURCE_LOCATION
        , const std::source_location location = std::source_location::current()
#endif
        )
    {
        const int function = pasynUser->reason;
        const char *param_name;
        getParamName(function, &param_name);

        try {
            ctl.write_params();
        } catch (std::runtime_error &e) {
#if defined(HAS_SOURCE_LOCATION) && defined(__GNUC__)
            std::string function_name = location.function_name();
            auto fp = function_name.find('(');
            function_name.erase(fp);
            auto fs = function_name.rfind(' ');
            function_name.erase(0, fs);
#endif

            epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
#ifdef HAS_SOURCE_LOCATION
                "%s: %s: %s", function_name.c_str(), param_name, e.what()
#else
                "UDriver::write_params: %s: %s", param_name, e.what()
#endif
                );

            return asynError;
        }

        read_parameters();

        return asynSuccess;
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

    virtual asynStatus writeInt32Impl(
        [[maybe_unused]] asynUser *pasynUser,
        [[maybe_unused]] const int function,
        [[maybe_unused]] const int addr,
        [[maybe_unused]] epicsInt32 value)
    {
        throw std::logic_error("this default implementation shouldn't be called");
    }

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

        if (decoder_controller.count(function)) try {
            if (first_general_parameter >= 0 && function <= last_general_parameter)
                generic_decoder_controller->write_general(param_name, value);
            else
                generic_decoder_controller->write_channel(param_name, addr, value);

            return write_params(pasynUser, *generic_decoder_controller);
        } catch (std::exception &e) {
            fprintf(stderr, "bad decoder_controller write. param: %s. exception: %s\n", param_name, e.what());
        }

        return writeInt32Impl(pasynUser, function, addr, value);
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

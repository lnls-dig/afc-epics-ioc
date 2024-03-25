#include <optional>
#include <tuple>
#include <unordered_set>
#include <vector>

#include <version>
#if __cpp_lib_source_location >= 201907L
# include <source_location>
# define HAS_SOURCE_LOCATION
#endif

#include <alarm.h>
#include <cantProceed.h>
#include <epicsExport.h>
#include <epicsString.h>
#include <iocsh.h>

#include <asynPortDriver.h>

#include <controllers.h>
#include <decoders.h>
#include <util_sdb.h>

#include "enumerate.h"
#include "pcie-single.h"

struct ParamInit {
    const char *name;
    int &parameter;
    std::optional<unsigned> number_of_channels = std::nullopt;
    asynParamType type;
    bool write_only = false;

    ParamInit(const char *name, int &parameter, asynParamType type = asynParamInt32):
        name(name),
        parameter(parameter),
        type(type)
    {
    }

    ParamInit set_nc(unsigned new_number_of_channels)
    {
        number_of_channels = new_number_of_channels;
        return *this;
    }

    ParamInit set_wo()
    {
        write_only = true;
        return *this;
    }
};

class UDriver: public asynPortDriver {
    RegisterDecoder *generic_decoder;
    RegisterDecoderController *generic_decoder_controller;

    int p_scan_task, p_counter;
    static const unsigned UDRIVER_PARAMS = 2;
    unsigned scan_counter = 0;

  protected:
    struct parameter_props {
        unsigned number_of_channels;
        asynParamType type;
        bool is_general;
        bool write_only;
        bool write_decoder_controller;
    };
    std::vector<parameter_props> parameter_props;

    static inline int p_read_only = 0, p_decoder_controller = 0;
    unsigned number_of_channels;
    int port_number;

    UDriver(
        const char *name, int port_number, RegisterDecoder *generic_decoder,
        unsigned number_of_channels,
        const std::vector<ParamInit> &name_and_index,
        const std::vector<ParamInit> &name_and_index_channel,
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

        /* fill the positions from the first UDriver parameters */
        parameter_props.resize(UDRIVER_PARAMS);

        auto create_params = [this](auto const &nai, bool is_general) {
            for (auto &&[i, v]: enumerate(nai)) {
                int p_tmp;
                auto &&[str, p, nc, t, wo] = v;

                /* if p is p_read_only or p_decoder_controller, we use a
                 * temporary variable to avoid any issues with simultaneous
                 * access to them */
                bool use_tmp = &p == &p_read_only || &p == &p_decoder_controller;
                int *pp = use_tmp ? &p_tmp : &p;
                createParam(str, t, pp);

                if (&p == &p_decoder_controller)
                    assert(this->generic_decoder_controller);

                assert((int)parameter_props.size() == *pp);
                parameter_props.push_back({
                    .number_of_channels = nc ? *nc : this->number_of_channels,
                    .type = t,
                    .is_general = is_general,
                    .write_only = wo,
                    .write_decoder_controller = (&p == &p_decoder_controller),
                });
            }
        };

        create_params(name_and_index, true);
        create_params(name_and_index_channel, false);
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

        for (const auto &&[p, prop]: enumerate(parameter_props)) {
            if (p < UDRIVER_PARAMS) continue;
            if (prop.write_only) continue;

            const char *param_name;
            getParamName(p, &param_name);

            auto get_and_set = [this, type=prop.type](int addr, int param, auto value) {
                if (type == asynParamInt32)
                    setIntegerParam(addr, param, std::get<int32_t>(value));
            };

            if (prop.is_general)
                get_and_set(0, p, generic_decoder->get_generic_data(param_name));
            else
                for (unsigned addr = 0; addr < prop.number_of_channels; addr++)
                    get_and_set(addr, p, generic_decoder->get_generic_data(param_name, addr));
        }

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

    asynStatus fill_enum(char *strings[], int values[], int severities[], size_t *nIn, const auto string_list)
    {
        for (auto &&[i, name]: enumerate(string_list)) {
            strings[i] = epicsStrDup(name.data());
            values[i] = i;
            severities[i] = NO_ALARM;

            *nIn = i+1;
        }

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

        /* parameter we don't know about */
        if ((unsigned)function >= parameter_props.size())
            return writeInt32Impl(pasynUser, function, addr, value);

        if (parameter_props.at(function).is_general && addr != 0) {
            epicsSnprintf(pasynUser->errorMessage, pasynUser->errorMessageSize,
                "writeInt32: %s: general parameter with addr=%d (should be 0)", param_name, addr);

            return asynError;
        }

        if (parameter_props.at(function).write_decoder_controller) try {
            if (parameter_props.at(function).is_general)
                generic_decoder_controller->write_general(param_name, value);
            else
                generic_decoder_controller->write_channel(param_name, addr, value);

            return write_params(pasynUser, *generic_decoder_controller);
        } catch (std::exception &e) {
            fprintf(stderr, "bad decoder_controller write. param: %s. exception: %s\n", param_name, e.what());
        }

        return writeInt32Impl(pasynUser, function, addr, value);
        /* parameters which aren't using generic_decoder_controller */
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

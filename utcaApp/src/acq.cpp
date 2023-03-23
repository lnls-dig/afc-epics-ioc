#include <mutex>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_set>

#include <epicsMessageQueue.h>
#include <epicsString.h>
#include <epicsThread.h>

#include "acq.h"

#include "pcie-single.h"
#include "udriver.h"

namespace {
    const unsigned number_of_channels = 24;

    enum class data_type {raw, lamp, last};
    const char *const data_type_strings[(int)data_type::last] = {"raw", "lamp"};

    enum class polarity {positive, negative, last};
    const char *const polarity_strings[(int)polarity::last] = {"positive", "negative"};

    enum class channel_organizations {lamp, dcc, last};
    const char *const channel_strings[16][(int)channel_organizations::last] = {
        {"lamp", "invalid"},
        {"invalid", "fofb"},
    };

    enum class trigger {now, external, data, software, last};
    const char *const trigger_strings[(int)trigger::last] = {"now", "external", "data", "software"};

    /* TODO: implement STOP action */
    enum class acq_command {start, stop, last};
    const char *const trigger_event_strings[(int)acq_command::last] = {"start", "stop"};

    enum class repetitive_trigger {normal, repetitive, last};
    const char *const repetitive_trigger_strings[(int)repetitive_trigger::last] = {"normal", "repetitive"};

    /* message format for the queue */
    struct AcqMessage {
        acq_command command;
        data_type type;
    };
    constexpr unsigned queue_len = 10;
}

class AcqWorker: public epicsThreadRunable {
    class Acq &acq;

  public:
    AcqWorker(Acq &acq): acq(acq) { }

    epicsMessageQueue queue{queue_len, sizeof(AcqMessage)};

    void run();
};

class Acq: public UDriver {
    acq::Core dec;

    std::mutex ctl_lock;
    acq::Controller ctl;

    int p_trigger_type;

    int p_polarity, p_data_trigger_sel, p_data_trigger_filt,
        p_data_trigger_threshold, p_trigger_delay, p_number_shots,
        p_pre_samples, p_post_samples, p_channel, p_data_trigger_channel;

    /* custom logic parameters */
    int p_event, p_repetitive, p_update_time, p_data_type;
    /* parameters for data storage */
    int p_raw_data, p_lamp_current_data, p_lamp_voltage_data;

    /* contains the parameters which should be written to the parameter list */
    std::unordered_set<int> parameters_to_store;

    epicsThread thread;
    AcqWorker worker{*this};

    friend AcqWorker;

  public:
    Acq(int port_number):
      UDriver(
          "ACQ", port_number, &dec,
          ::number_of_channels,
          {
              {"HW_TRIG_POL", p_polarity},
              {"INT_TRIG_SEL", p_data_trigger_sel},
              {"THRES_FILT", p_data_trigger_filt},
              {"TRIG_DATA_THRES", p_data_trigger_threshold},
              {"TRIG_DLY", p_trigger_delay},
              {"NB", p_number_shots},
              {"PRE_SAMPLES", p_pre_samples},
              {"POST_SAMPLES", p_post_samples},
              {"WHICH", p_channel},
              {"DTRIG_WHICH", p_data_trigger_channel},
          },
          { }),
      dec(bars),
      ctl(bars),
      thread(worker, (std::string(portName) + "-worker").c_str(), epicsThreadGetStackSize(epicsThreadStackMedium), epicsThreadPriorityMedium)
    {
        if (auto v = read_sdb(&bars, ctl.device_match, port_number)) {
            dec.set_devinfo(*v);
            ctl.set_devinfo(*v);
        } else {
            throw std::runtime_error("couldn't find acq module");
        }

        /* isn't read back from hardware */
        createParam("TRIGGER", asynParamInt32, &p_trigger_type);

        createParam("EVENT", asynParamInt32, &p_event);
        createParam("REPETITIVE", asynParamInt32, &p_repetitive);
        createParam("UPDATE_TIME", asynParamFloat64, &p_update_time);
        createParam("TYPE", asynParamInt32, &p_data_type);
        /* have to be initialized for the worker thread to work properly */
        setIntegerParam(p_repetitive, (int)repetitive_trigger::normal);
        setDoubleParam(p_update_time, 0);
        setIntegerParam(p_data_type, (int)data_type::raw);

        createParam("RAW", asynParamInt32Array, &p_raw_data);
        createParam("LAMP_I", asynParamInt16Array, &p_lamp_current_data);
        createParam("LAMP_V", asynParamInt16Array, &p_lamp_voltage_data);

        thread.start();

        parameters_to_store = {
            p_trigger_type, /* so it's copied to the readback variable */
            /* used for custom logic */
            p_repetitive,
            p_update_time,
            p_data_type,
        };
    }

    asynStatus readEnum(asynUser *pasynUser, char *strings[], int values[],
        [[maybe_unused]] int severities[], [[maybe_unused]] size_t nElements, size_t *nIn)
    {
        const int function = pasynUser->reason;

        auto set_enum = [&strings, &values, nIn](auto &new_strings) {
            for (auto &&[i, s]: enumerate(new_strings)) {
                strings[i] = epicsStrDup(s);
                values[i] = i;

                *nIn = i+1;
            }
        };

        if (function == p_data_type) {
            set_enum(data_type_strings);
        } else if (function == p_polarity) {
            set_enum(polarity_strings);
        } else if (function == p_data_trigger_channel) {
            set_enum(channel_strings[(int)channel_organizations::lamp]);
        } else if (function == p_channel) {
            /* TODO: make this more generic; perhaps take into account data_type,
             * using a callback from writeInt32Impl */
            if (port_number == 0) set_enum(channel_strings[(int)channel_organizations::lamp]);
            else set_enum(channel_strings[(int)channel_organizations::dcc]);
        } else if (function == p_trigger_type) {
            set_enum(trigger_strings);
        } else if (function == p_event) {
            set_enum(trigger_event_strings);
        } else if (function == p_repetitive) {
            set_enum(repetitive_trigger_strings);
        } else {
            *nIn = 0;
            return asynError;
        }

        return asynSuccess;
    }

    asynStatus writeInt32Impl([[maybe_unused]] asynUser *pasynUser, const int function,
        [[maybe_unused]] const int addr, [[maybe_unused]] const char *param_name, epicsInt32 value)
    {
        {
            std::lock_guard g(ctl_lock);

            if (function == p_trigger_type) ctl.trigger_type = trigger_strings[value];

            if (function == p_polarity) ctl.data_trigger_polarity_neg = value;
            if (function == p_data_trigger_sel) ctl.data_trigger_sel = value;
            if (function == p_data_trigger_filt) ctl.data_trigger_filt = value;
            if (function == p_data_trigger_threshold) ctl.data_trigger_threshold = value;
            if (function == p_trigger_delay) ctl.trigger_delay = value;
            if (function == p_number_shots) ctl.number_shots = value;
            if (function == p_pre_samples) ctl.pre_samples = value;
            if (function == p_post_samples) ctl.post_samples = value;
            if (function == p_channel) ctl.channel = value;
            if (function == p_data_trigger_channel) ctl.data_trigger_channel = value;
        }

        if (parameters_to_store.count(function)) {
            setIntegerParam(function, value);
            return asynSuccess;
        }

        if (function == p_event) {
            acq_command command = (acq_command)value;

            epicsInt32 data_type_v;
            getIntegerParam(p_data_type, &data_type_v);

            AcqMessage msg{command, (data_type)data_type_v};
            worker.queue.send(&msg, sizeof msg);
        }

        read_parameters();

        return asynSuccess;
    }

    asynStatus writeFloat64(asynUser *pasynUser, epicsFloat64 value)
    {
        const int function = pasynUser->reason;

        if (parameters_to_store.count(function)) {
            setDoubleParam(function, value);
            return asynSuccess;
        }

        return asynPortDriver::writeFloat64(pasynUser, value);
    }
};

void AcqWorker::run()
{
    bool ongoing = false;

    while (1) {
        AcqMessage msg;
        /* block in queue if there is no acquisition happening */
        if ((ongoing && queue.tryReceive(&msg, sizeof msg) != -1) ||
            (!ongoing && queue.receive(&msg, sizeof msg) != -1)) {
            if (msg.command == acq_command::start && !ongoing) {
                std::lock_guard g(acq.ctl_lock);
                acq.ctl.start_acquisition();
                ongoing = true;
            }
            if (msg.command == acq_command::stop) {
                acq.ctl.stop_acquisition();
                ongoing = false;
            }
        }

        auto do_callbacks = [this](auto &vec, int parameter, int addr) -> asynStatus {
            using vec_type = std::remove_reference_t<decltype(vec)>;
            using value_type = typename vec_type::value_type;

            std::lock_guard g(acq);

            if constexpr (std::is_same_v<value_type, int32_t>)
                return acq.doCallbacksInt32Array(vec.data(), vec.size(), parameter, addr);
            else if constexpr (std::is_same_v<value_type, int16_t>)
                return acq.doCallbacksInt16Array(vec.data(), vec.size(), parameter, addr);
            else
                static_assert(!sizeof(value_type)); /* has to depend on template parameter */
        };

        if (ongoing && acq.ctl.result_async() == acq::acq_status::success) {
            ongoing = false;

            if (msg.type == data_type::raw) {
                auto rv = acq.ctl.get_result<int32_t>();

                do_callbacks(rv, acq.p_raw_data, 0);
            } else if (msg.type == data_type::lamp) {
                auto rv = acq.ctl.get_result<int16_t>();

                auto len = rv.size() / 32;
                std::vector<int16_t> scratch(len);
                for (int addr = 0; addr < 12; addr++) {
                    for (size_t i = 0; i < len; i++) {
                        scratch[i] = rv[addr + i * 32];
                    }
                    do_callbacks(scratch, acq.p_lamp_current_data, addr);

                    for (size_t i = 0; i < len; i++) {
                        scratch[i] = rv[addr + 12 + i * 32];
                    }
                    do_callbacks(scratch, acq.p_lamp_voltage_data, addr);
                }
            }

            epicsInt32 repetitive;
            epicsFloat64 update_time;
            {
                std::lock_guard g(acq);
                acq.getIntegerParam(acq.p_repetitive, &repetitive);
                acq.getDoubleParam(acq.p_update_time, &update_time);
            }
            if (repetitive == (int)repetitive_trigger::repetitive) {
                epicsThreadSleep(update_time);
                queue.send(&msg, sizeof msg);
            }
        }
    }
}

namespace {
    constexpr char name[] = "Acq";
    UDriverFn<Acq, name> iocsh_init;
}

extern "C" {
    static void registerAcq()
    {
        iocshRegister(&iocsh_init.init_func_def, iocsh_init.init_call_func);
    }

    epicsExportRegistrar(registerAcq);
}

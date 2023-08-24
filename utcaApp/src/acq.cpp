#include <chrono>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <tuple>
#include <unordered_set>

#include <alarm.h>
#include <epicsMessageQueue.h>
#include <epicsString.h>
#include <epicsThread.h>

#include <modules/acq.h>

#include "pcie-single.h"
#include "udriver.h"

using namespace std::chrono_literals;

namespace {
    const unsigned number_of_channels = 24;

    const auto acq_task_sleep = 100ms;

    struct enum_info {
        const char *const name;
        const int severity = NO_ALARM;
    };

    enum class data_type {raw, lamp, sysid, last};
    const enum_info data_type_info[(int)data_type::last] = {{"raw"}, {"lamp"}, {"sysid"}};

    enum class polarity {positive, negative, last};
    const enum_info polarity_info[(int)polarity::last] = {{"positive"}, {"negative"}};

    enum class channel_organizations {lamp, dcc, sysid, sysid_applied, last};
    const enum_info invalid_channel = {"invalid", MINOR_ALARM};
    const enum_info channel_info[][(int)channel_organizations::last] = {
        {{"lamp"}, invalid_channel, invalid_channel, invalid_channel},
        {invalid_channel, {"dcc"}, invalid_channel, invalid_channel},
        {invalid_channel, invalid_channel, {"sysid"}, {"sysid_applied"}},
    };

    enum class trigger {now, external, data, software, last};
    const enum_info trigger_info[(int)trigger::last] = {{"now"}, {"external"}, {"data"}, {"software"}};

    enum class acq_command {start, stop, last};
    const enum_info trigger_event_info[(int)acq_command::last] = {{"start"}, {"stop"}};

    enum class repetitive_trigger {normal, repetitive, last};
    const enum_info repetitive_trigger_info[(int)repetitive_trigger::last] = {{"normal"}, {"repetitive"}};

    enum class acq_status {
      idle, waiting,
      /* TODO: these aren't used yet */ external_trigger, data_trigger, sw_trigger,
      acquiring,
      error, bad_post_samples, too_many_samples, no_samples,
      /* TODO: these aren't used yet */ no_memory, overflow,
      last};
    const enum_info acq_status_info[(int)acq_status::last] = {
        {"idle"}, {"waiting"},
        {"external_trigger"}, {"data_trigger"}, {"sw_trigger"},
        {"acquiring"},
        {"error", MINOR_ALARM}, {"bad_post_samples", MINOR_ALARM}, {"too_many_samples", MINOR_ALARM}, {"no_samples", MINOR_ALARM},
        {"no_memory", MAJOR_ALARM}, {"overflow", MAJOR_ALARM}
    };

    /* message format for the queue */
    struct AcqMessage {
        acq_command command;
        data_type type;
    };
    constexpr unsigned queue_len = 10;
}

class AcqWorker: public epicsThreadRunable {
    class Acq &acq;

    /* scratch vectors to avoid repeated allocations */
    std::vector<int8_t> i8scratch;
    std::vector<int16_t> i16scratch;
    std::vector<int32_t> i32scratch;

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
    int p_event, p_repetitive, p_update_time, p_data_type, p_status, p_count;
    /* parameters for data storage */
    int p_raw_data, p_lamp_current_data, p_lamp_voltage_data,
        p_pos_x, p_pos_y, p_setpoint, p_timeframe, p_prbs;

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
        auto v = find_device(port_number);
        dec.set_devinfo(v);
        ctl.set_devinfo(v);

        /* isn't read back from hardware */
        createParam("TRIGGER", asynParamInt32, &p_trigger_type);

        createParam("EVENT", asynParamInt32, &p_event);
        createParam("REPETITIVE", asynParamInt32, &p_repetitive);
        createParam("UPDATE_TIME", asynParamFloat64, &p_update_time);
        createParam("TYPE", asynParamInt32, &p_data_type);
        createParam("STATUS", asynParamInt32, &p_status);
        createParam("COUNT", asynParamInt32, &p_count);
        /* have to be initialized for the worker thread to work properly */
        setIntegerParam(p_repetitive, (int)repetitive_trigger::normal);
        setDoubleParam(p_update_time, 0);
        setIntegerParam(p_data_type, (int)data_type::raw);
        setIntegerParam(p_status, (int)acq_status::idle);
        setIntegerParam(p_count, 0);

        createParam("RAW", asynParamInt32Array, &p_raw_data);
        createParam("LAMP_I", asynParamInt16Array, &p_lamp_current_data);
        createParam("LAMP_V", asynParamInt16Array, &p_lamp_voltage_data);
        createParam("POS_X", asynParamInt32Array, &p_pos_x);
        createParam("POS_Y", asynParamInt32Array, &p_pos_y);
        createParam("SETPOINT", asynParamInt16Array, &p_setpoint);
        /* it's an unsigned 16-bit type, so we need signed 32-bit to store/display it */
        createParam("TIMEFRAME", asynParamInt32Array, &p_timeframe);
        createParam("PRBS", asynParamInt8Array, &p_prbs);

        thread.start();

        parameters_to_store = {
            p_trigger_type, /* so it's copied to the readback variable */
            /* used for custom logic */
            p_repetitive,
            p_update_time,
            p_data_type,
        };

        read_parameters();
    }

    asynStatus readEnum(asynUser *pasynUser, char *strings[], int values[],
        int severities[], [[maybe_unused]] size_t nElements, size_t *nIn)
    {
        const int function = pasynUser->reason;

        auto set_enum = [&strings, &values, &severities, nIn](auto &infos) {
            for (auto &&[i, info]: enumerate(infos)) {
                strings[i] = epicsStrDup(info.name);
                values[i] = i;
                severities[i] = info.severity;

                *nIn = i+1;
            }
        };

        if (function == p_data_type) {
            set_enum(data_type_info);
        } else if (function == p_polarity) {
            set_enum(polarity_info);
        } else if (function == p_data_trigger_channel) {
            set_enum(channel_info[(int)channel_organizations::lamp]);
        } else if (function == p_channel) {
            /* TODO: make this more generic; perhaps take into account data_type,
             * using a callback from writeInt32Impl */
            if (port_number == 0) set_enum(channel_info[(int)channel_organizations::lamp]);
            else if (port_number < 3) set_enum(channel_info[(int)channel_organizations::dcc]);
            else set_enum(channel_info[(int)channel_organizations::sysid]);
        } else if (function == p_trigger_type) {
            set_enum(trigger_info);
        } else if (function == p_event) {
            set_enum(trigger_event_info);
        } else if (function == p_repetitive) {
            set_enum(repetitive_trigger_info);
        } else if (function == p_status) {
            set_enum(acq_status_info);
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

            if (function == p_trigger_type) ctl.trigger_type = trigger_info[value].name;

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
    std::chrono::time_point<std::chrono::steady_clock> start_time;

    auto set_status = [this](acq_status v) {
        std::lock_guard g(acq);
        acq.setIntegerParam(acq.p_status, (int)v);
        acq.callParamCallbacks(0);
    };

    while (1) {
        AcqMessage msg;
        /* block in queue if there is no acquisition happening */
        if ((ongoing && queue.tryReceive(&msg, sizeof msg) != -1) ||
            (!ongoing && queue.receive(&msg, sizeof msg) != -1)) {
            if (msg.command == acq_command::start && !ongoing) {
                acq::acq_error error;
                {
                    /* lock only inside this block so we don't deadlock when locking acq afterwards */
                    std::lock_guard g(acq.ctl_lock);
                    error = acq.ctl.start_acquisition();
                }
                switch (error) {
                    case acq::acq_error::success:
                        ongoing = true;
                        start_time = std::chrono::steady_clock::now();
                        set_status(acq_status::acquiring);
                        break;
                    case acq::acq_error::error:
                        set_status(acq_status::error);
                        break;
                    case acq::acq_error::bad_post_samples:
                        set_status(acq_status::bad_post_samples);
                        break;
                    case acq::acq_error::too_many_samples:
                        set_status(acq_status::too_many_samples);
                        break;
                    case acq::acq_error::no_samples:
                        set_status(acq_status::no_samples);
                        break;
                }
            }
            if (msg.command == acq_command::stop) {
                acq.ctl.stop_acquisition();
                ongoing = false;
                set_status(acq_status::idle);
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
            else if constexpr (std::is_same_v<value_type, int8_t>)
                return acq.doCallbacksInt8Array(vec.data(), vec.size(), parameter, addr);
            else
                static_assert(!sizeof(value_type)); /* has to depend on template parameter */
        };

        if (ongoing && acq.ctl.get_acq_status() == acq::acq_status::success) {
            ongoing = false;

            if (msg.type == data_type::raw) {
                auto rv = acq.ctl.get_result<int32_t>();

                do_callbacks(rv, acq.p_raw_data, 0);
            } else if (msg.type == data_type::lamp) {
                auto rv = acq.ctl.get_result<int16_t>();

                auto len = rv.size() / 32;
                i16scratch.resize(len);
                for (int addr = 0; addr < 12; addr++) {
                    for (size_t i = 0; i < len; i++) {
                        i16scratch[i] = rv[addr + i * 32];
                    }
                    do_callbacks(i16scratch, acq.p_lamp_current_data, addr);

                    for (size_t i = 0; i < len; i++) {
                        i16scratch[i] = rv[addr + 12 + i * 32];
                    }
                    do_callbacks(i16scratch, acq.p_lamp_voltage_data, addr);
                }
            } else if (msg.type == data_type::sysid) {
                auto rv = acq.ctl.get_result<int32_t>();

                const size_t atoms = 32, bpm_channels = 8, lamp_channels = 12;

                auto len = rv.size() / atoms;
                i8scratch.resize(len);
                i16scratch.resize(len);
                i32scratch.resize(len);

                for (size_t addr = 0; addr < bpm_channels; addr++) {
                    for (size_t i = 0; i < len; i++)
                        i32scratch[i] = rv[addr + i * atoms];
                    do_callbacks(i32scratch, acq.p_pos_x, addr);

                    for (size_t i = 0; i < len; i++)
                        i32scratch[i] = rv[addr + bpm_channels + i * atoms];
                    do_callbacks(i32scratch, acq.p_pos_y, addr);
                }

                /* the setpoint samples are grouped two-by-two in 32-bit atoms */
                for (size_t addr = 0; addr < lamp_channels; addr++) {
                    for (size_t i = 0; i < len; i++) {
                        auto v = (uint32_t)rv[addr/2 + 16 + i * atoms];
                        i16scratch[i] = (uint16_t)(addr & 0x1 ? v & 0xffff : v >> 16);
                    }
                    do_callbacks(i16scratch, acq.p_setpoint, addr);
                }

                for (size_t i = 0; i < len; i++) {
                    i32scratch[i] = rv[22 + i * atoms] & 0xffff;
                    i8scratch[i] = rv[23 + i * atoms] & 0x1;
                }
                do_callbacks(i32scratch, acq.p_timeframe, 0);
                do_callbacks(i8scratch, acq.p_prbs, 0);
            }

            epicsInt32 repetitive, counter;
            epicsFloat64 update_time;
            {
                std::lock_guard g(acq);
                acq.getIntegerParam(acq.p_repetitive, &repetitive);
                acq.getDoubleParam(acq.p_update_time, &update_time);

                /* since we are already holding the lock, update COUNT as well */
                acq.getIntegerParam(acq.p_count, &counter);
                acq.setIntegerParam(acq.p_count, counter+1);
                acq.callParamCallbacks(0);
            }
            /* only start a new acquisition if there is no incoming command */
            if (repetitive == (int)repetitive_trigger::repetitive && queue.pending() == 0) {
                set_status(acq_status::waiting);
                std::this_thread::sleep_until(start_time + update_time * 1s);
                queue.send(&msg, sizeof msg);
            } else {
                set_status(acq_status::idle);
            }
        } else if (ongoing) {
            std::this_thread::sleep_for(acq_task_sleep);
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

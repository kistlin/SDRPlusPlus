#include <imgui.h>
#include <module.h>
#include <gui/gui.h>
#include <signal_path/signal_path.h>
#include <signal_path/sink.h>
#include <dsp/audio.h>
#include <dsp/processing.h>
#include <spdlog/spdlog.h>
#include <config.h>
#include <options.h>

#include <spa-0.2/spa/param/audio/format-utils.h>
#include <spa-0.2/spa/param/param.h>
#include <spa-0.2/spa/param/audio/raw.h>
#include <pipewire-0.3/pipewire/pipewire.h>
#include <pipewire-0.3/pipewire/core.h>
#include <pipewire-0.3/pipewire/keys.h>

#include <utility>
#include <string>

#define CONCAT(a, b) ((std::string(a) + b).c_str())

SDRPP_MOD_INFO{
    /* Name:            */ "pipewire_sink",
    /* Description:     */ "PipeWire sink module for SDR++",
    /* Author:          */ "",
    /* Version:         */ 0, 1, 0,
    /* Max instances    */ 1
};

ConfigManager config;

class PipeWireSink : SinkManager::Sink {
public:
    PipeWireSink(SinkManager::Stream* stream, std::string streamName) {
        spdlog::debug("PipeWireSink::PipeWireSink stream = {0}, streamName = {1}", (void*)stream, streamName);
        _stream = stream;
        _streamName = std::move(streamName);
        stereoPacker.init(_stream->sinkOut, BUFFER_SIZE);
        s2m.init(&stereoPacker.out);
        monoPacker.init(&s2m.out, BUFFER_SIZE);
        _stream->setSampleRate(DEFAULT_RATE);

        playStateHandler.handler = playStateChangeHandler;
        playStateHandler.ctx = this;
        gui::mainWindow.onPlayStateChange.bindHandler(&playStateHandler);
    }

    ~PipeWireSink() {
        spdlog::debug("PipeWireSink::~PipeWireSink");
        cleanupPipeWire(this);
        gui::mainWindow.onPlayStateChange.unbindHandler(&playStateHandler);
    }

    void start() {
        spdlog::debug("PipeWireSink::start");

        if (running) {
            spdlog::debug("PipeWireSink::start: already running");
            return;
        }

        if (initPipeWire(this)) {
            stereoPacker.start();
            s2m.start();
            monoPacker.start();
            running = true;
        }
    }

    void stop() {
        spdlog::debug("PipeWireSink::stop");
        if (!running) {
            spdlog::debug("PipeWireSink::stop: not running, but stop called");
            return;
        }

        stereoPacker.stop();
        s2m.stop();
        monoPacker.stop();
        cleanupPipeWire(this);

        running = false;
    }

    void menuHandler() {
        // spdlog::debug("PipeWireSink::menuHandler");
    }

private:
    static bool initPipeWire(void* ctx) {
        spdlog::debug("PipeWireSink::initPipeWire");
        PipeWireSink* _this = (PipeWireSink*)ctx;
        _this->spaPodBuilder = SPA_POD_BUILDER_INIT(_this->buffer, sizeof(_this->buffer));

        pw_init(nullptr, nullptr);

        _this->pwData.pipeWireSinkInstance = ctx;
        _this->pwData.loop = pw_thread_loop_new("sdrpp", NULL);

        _this->pwData.stream = pw_stream_new_simple(
            pw_thread_loop_get_loop(_this->pwData.loop),
            "sdrpp",
            pw_properties_new(
                PW_KEY_MEDIA_TYPE, "Audio",
                PW_KEY_MEDIA_CATEGORY, "Playback",
                PW_KEY_MEDIA_ROLE, "Music",
                NULL),
            &pwStreamEvents,
            &_this->pwData);

        if (!_this->pwData.stream) {
            spdlog::error("pw_stream_new_simple failed");
            cleanupPipeWire(ctx);
            return false;
        }
        spdlog::debug("pw_stream_new_simple successful");

        _this->spaPodParams[0] = spa_format_audio_raw_build(&_this->spaPodBuilder, SPA_PARAM_EnumFormat,
                                                            &_this->spaAudioInfo);

        int result = pw_stream_connect(_this->pwData.stream,
                                       PW_DIRECTION_OUTPUT,
                                       PW_ID_ANY,
                                       static_cast<pw_stream_flags>(PW_STREAM_FLAG_AUTOCONNECT |
                                                                    PW_STREAM_FLAG_MAP_BUFFERS |
                                                                    PW_STREAM_FLAG_RT_PROCESS),
                                       _this->spaPodParams, 1);
        if (result != 0) {
            spdlog::error("pw_stream_connect failed: {0}", result);
            cleanupPipeWire(ctx);
            return false;
        }

        spdlog::debug("PipeWireSink::initPipeWire successful");
        return true;
    }

    static void playStateChangeHandler(bool newState, void* ctx) {
        PipeWireSink* _this = (PipeWireSink*)ctx;
        spdlog::debug("PipeWireSink::playStateChangeHandler: newState = {0}, ctx = {1}", newState, ctx);
        if (newState) {
            if (!_this->pwData.loop) {
                initPipeWire(ctx);
            }
            if (!_this->playing) {
                spdlog::debug("calling pw_thread_loop_start");
                if (pw_thread_loop_start(_this->pwData.loop) != 0) {
                    spdlog::error("pw_thread_loop_start failed");
                    cleanupPipeWire(ctx);
                    return;
                }
                _this->playing = true;
            }
            spdlog::debug("playStateChangeHandler: playing");
        }
        else {
            cleanupPipeWire(ctx);
            _this->playing = false;
            spdlog::debug("playStateChangeHandler: not playing");
        }
    };

    static void cleanupPipeWire(void* ctx) {
        spdlog::debug("PipeWireSink::cleanupPipeWire");
        PipeWireSink* _this = (PipeWireSink*)ctx;
        if (_this->pwData.loop) {
            pw_thread_loop_stop(_this->pwData.loop);
            pw_stream_destroy(_this->pwData.stream);
            pw_thread_loop_destroy(_this->pwData.loop);
            pw_deinit();
            _this->pwData.pipeWireSinkInstance = nullptr;
            _this->pwData.loop = nullptr;
            _this->pwData.stream = nullptr;
        }
        else {
            spdlog::debug("PipeWireSink::cleanupPipeWire: tried to stop non-existing loop");
        }
    };

    static void onProcess(void* userdata) {
        struct PipeWireData_t* data = static_cast<struct PipeWireData_t*>(userdata);
        PipeWireSink* _this = (PipeWireSink*)data->pipeWireSinkInstance;
        struct pw_buffer* pwBuffer;
        struct spa_buffer* spaBuffer;
        void* dst;

        if ((pwBuffer = pw_stream_dequeue_buffer(data->stream)) == nullptr) {
            spdlog::warn("out of buffers: %m");
            return;
        }

        spaBuffer = pwBuffer->buffer;
        if ((dst = spaBuffer->datas[0].data) == nullptr)
            return;

        int stride = sizeof(float) * DEFAULT_CHANNELS;

        const int readCount = _this->monoPacker.out.read(); // blocks if nothing available
        if (readCount < 0) { return; }

        const int maxPwBufferSize = spaBuffer->datas[0].maxsize;
        int dataSizeInBytes = std::min<int>(readCount * stride, maxPwBufferSize);

        std::memcpy(dst, _this->monoPacker.out.readBuf, dataSizeInBytes);
        _this->monoPacker.out.flush();

        spaBuffer->datas[0].chunk->offset = 0;
        spaBuffer->datas[0].chunk->stride = stride;
        spaBuffer->datas[0].chunk->size = dataSizeInBytes;

        pw_stream_queue_buffer(data->stream, pwBuffer);
    }

    constexpr static const struct pw_stream_events pwStreamEvents = {
        .version = PW_VERSION_STREAM_EVENTS,
        .process = onProcess,
    };

    enum {
        BUFFER_SIZE = 1024,
        DEFAULT_RATE = 48000,
        DEFAULT_CHANNELS = 1
    };

    struct PipeWireData_t {
        void* pipeWireSinkInstance;
        struct pw_thread_loop* loop;
        struct pw_stream* stream;
    };

    bool playing = false;
    bool running = false;
    uint8_t buffer[BUFFER_SIZE] = { 0 };
    struct PipeWireData_t pwData = { nullptr, nullptr, nullptr };
    struct spa_audio_info_raw spaAudioInfo = { .format = SPA_AUDIO_FORMAT_F32,
                                               .rate = DEFAULT_RATE,
                                               .channels = DEFAULT_CHANNELS };
    const struct spa_pod* spaPodParams[1];
    struct spa_pod_builder spaPodBuilder = {};
    SinkManager::Stream* _stream = nullptr;
    EventHandler<bool> playStateHandler;
    dsp::StereoToMono s2m;
    dsp::Packer<float> monoPacker;
    dsp::Packer<dsp::stereo_t> stereoPacker;
    std::string _streamName;
};

class PipeWireSinkModule : public ModuleManager::Instance {
public:
    PipeWireSinkModule(std::string name) {
        spdlog::debug("PipeWireSinkModule::PipeWireSinkModule: name = {0}", name.c_str());
        this->name = std::move(name);
        provider.create = create_sink;
        provider.ctx = this;

        sigpath::sinkManager.registerSinkProvider(sinkProviderName, provider);
    }

    ~PipeWireSinkModule() {
        // Unregister sink, this will automatically stop and delete all instances of the audio sink
        sigpath::sinkManager.unregisterSinkProvider(sinkProviderName);
    }

    void postInit() {
        spdlog::debug("PipeWireSinkModule::postInit");
    }

    void enable() {
        spdlog::debug("PipeWireSinkModule::enable");
        enabled = true;
    }

    void disable() {
        spdlog::debug("PipeWireSinkModule::disable");
        enabled = false;
    }

    bool isEnabled() {
        spdlog::debug("PipeWireSinkModule::isEnabled");
        return enabled;
    }

private:
    static SinkManager::Sink* create_sink(SinkManager::Stream* stream, std::string streamName, void* ctx) {
        PipeWireSink* newSink = new PipeWireSink(stream, std::move(streamName));
        spdlog::debug("PipeWireSinkModule::create_sink: {0}", (void*)newSink);
        return (SinkManager::Sink*)(newSink);
    }

    bool enabled = false;
    std::string name;
    SinkManager::SinkProvider provider;
    const std::string sinkProviderName = "PipeWire";
};

MOD_EXPORT void _INIT_() {
    json def = json({});
    config.setPath(options::opts.root + "/pipewire_sink_config.json");
    config.load(def);
    config.enableAutoSave();
}

MOD_EXPORT void* _CREATE_INSTANCE_(std::string name) {
    PipeWireSinkModule* instance = new PipeWireSinkModule(std::move(name));
    return instance;
}

MOD_EXPORT void _DELETE_INSTANCE_(void* instance) {
    delete (PipeWireSinkModule*)instance;
}

MOD_EXPORT void _END_() {
    config.disableAutoSave();
    config.save();
}
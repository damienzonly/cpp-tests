#include <pulse/pulseaudio.h>
#include <pulse/stream.h>
#include <memory>
#include <map>

typedef struct StreamData {
    pa_buffer_attr buffer_attr;
    pa_sample_spec sample_spec;
};

typedef struct IOStreamData {
    StreamData* input;
    StreamData* output;
};

class Application
{
private:
public:
    pa_threaded_mainloop* mMainLoop;
    pa_context* mContext;
    std::unique_ptr<std::map<int, const pa_sink_input_info*>> sinkInputs;
    std::unique_ptr<std::map<int, const pa_client_info*>> clients;
    std::unique_ptr<std::map<int, IOStreamData*>> streamsData;

    Application() {
        this->sinkInputs = std::make_unique<std::map<int, const pa_sink_input_info*>>();
        this->clients = std::make_unique<std::map<int, const pa_client_info*>>();
        this->streamsData = std::make_unique<std::map<int, IOStreamData*>>();
    }
    ~Application() {
        // Free resources
        pa_context_disconnect(this->mContext);
        pa_context_unref(this->mContext);
        pa_threaded_mainloop_free(this->mMainLoop);
    }

    void init();
    
    // callback methods
    static void eventCallback(pa_context *c, pa_subscription_event_type_t t, uint32_t idx, void *userdata);
    static void sinkListCallback(pa_context *c, const pa_sink_input_info *i, int eol, void *userdata);
    static void clientListCallback(pa_context *c, const pa_client_info*i, int eol, void *userdata);
    static void contextReadyCallback(pa_context *c, void *userdata);
    static Application* convertToApplication(void* userdata) {
        return static_cast<Application*>(userdata);
    }

    // add/remove methods
    void addSinkInput(const pa_sink_input_info* i, void* userdata);
    void removeSinkInput(const uint32_t index);
    void addClientInfo(const pa_client_info* i);
    void removeClientInfo(const uint32_t index);
    void createIOStreams(const pa_sink_input_info* info, void* userdata);
};
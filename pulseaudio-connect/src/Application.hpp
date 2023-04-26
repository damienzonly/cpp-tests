#include <pulse/pulseaudio.h>
#include <memory>
#include <map>

class Application
{
private:
public:
    pa_threaded_mainloop* mMainLoop;
    pa_context* mContext;
    std::unique_ptr<std::map<int, const pa_sink_input_info*>> sinkInputs;
    std::unique_ptr<std::map<int, const pa_client_info*>> clients;

    Application() {
        this->sinkInputs = std::make_unique<std::map<int, const pa_sink_input_info*>>();
        this->clients = std::make_unique<std::map<int, const pa_client_info*>>();
    }
    ~Application() {
        // Free resources
        pa_context_disconnect(this->mContext);
        pa_context_unref(this->mContext);
        pa_threaded_mainloop_free(this->mMainLoop);
    }

    void init();
    static void eventCallback(pa_context *c, pa_subscription_event_type_t t, uint32_t idx, void *userdata);
    static void sinkListCallback(pa_context *c, const pa_sink_input_info *i, int eol, void *userdata);
    static void clientListCallback(pa_context *c, const pa_client_info*i, int eol, void *userdata);
    static void contextReadyCallback(pa_context *c, void *userdata);
    static Application* convertToApplication(void* userdata) {
        return static_cast<Application*>(userdata);
    }

    void addSinkInput(const pa_sink_input_info* i);
    void removeSinkInput(const uint32_t index);
    void addClientInfo(const pa_client_info* i);
    void removeClientInfo(const uint32_t index);
};
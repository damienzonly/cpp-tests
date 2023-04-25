#include <pulse/pulseaudio.h>
#include <memory>
#include <vector>

typedef std::vector<const pa_sink_input_info*> sink_inputs_vector;

class Application
{
private:
public:
    pa_threaded_mainloop* mMainLoop;
    pa_context* mContext;
    std::unique_ptr<sink_inputs_vector> sinkInputs;

    Application() {
        this->sinkInputs = std::make_unique<sink_inputs_vector>();
    }
    ~Application() {
        // Free resources
        pa_context_disconnect(this->mContext);
        pa_context_unref(this->mContext);
        pa_threaded_mainloop_free(this->mMainLoop);
        this->sinkInputs.reset();
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
};
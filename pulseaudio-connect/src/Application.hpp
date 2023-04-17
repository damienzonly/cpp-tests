#include <pulse/pulseaudio.h>
#include <memory>
#include <vector>

typedef std::vector<const pa_sink_input_info*> sink_inputs_vector;

class Application
{
private:
    pa_threaded_mainloop* mMainLoop;
    pa_context* mContext;
    std::unique_ptr<sink_inputs_vector> sink_inputs;
    void fill_sink_inputs();
public:
    Application() {
        this->sink_inputs = std::make_unique<sink_inputs_vector>();
    }
    ~Application() {
        // Free resources
        pa_context_disconnect(this->mContext);
        pa_context_unref(this->mContext);
        pa_threaded_mainloop_free(this->mMainLoop);
        this->sink_inputs.reset();
    }
    void init();
    void start();
    static void state_callback(pa_context *c, void *userdata);
    static Application* convert_to_application(void* userdata) {
        return static_cast<Application*>(userdata);
    }
};
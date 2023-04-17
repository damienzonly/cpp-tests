#include "Application.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <stdio.h>

void foo(pa_context* c, const pa_sink_info* info, int eol, void* userdata) {
    if (info != NULL) {
        std::cout << "name is " << info->name << std::endl;
    } else if (eol) {
        std::cout << "end of input list" << std::endl;
    } else {
        std::cerr << "error fetching sink inputs list" << std::endl;
    }
}

void bar(pa_context* c, const pa_sink_input_info* info, int eol, void* userdata) {
    if (info != NULL) {
        std::cout << "name is " << info->name << std::endl;
    } else if (eol) {
        std::cout << "end of input list" << std::endl;
    } else {
        std::cerr << "error fetching sink inputs list" << std::endl;
    }
}

void Application::state_callback(pa_context *c, void *userdata) {
    auto application = Application::convert_to_application(userdata);
    auto state = pa_context_get_state(c);
    switch(state) {
        case PA_CONTEXT_CONNECTING:
            std::cout << "connecting to audio server..." << std::endl;
            break;
        case PA_CONTEXT_READY: {
            std::cout << "connected to audio server" << std::endl;
            application->start();
            break;
        }
        case PA_CONTEXT_FAILED:
            std::cout << "failed to connect to audio server" << std::endl;
            break;
        case PA_CONTEXT_TERMINATED:
            std::cout << "connection to audio server terminated" << std::endl;
            break;
    };
}

void Application::init() {
    this->mMainLoop = pa_threaded_mainloop_new();
    this->mContext = pa_context_new(pa_threaded_mainloop_get_api(this->mMainLoop), "myname");
    pa_context_set_state_callback(mContext, &Application::state_callback, this);
    pa_context_connect(mContext, nullptr, PA_CONTEXT_NOFLAGS, nullptr);
    pa_threaded_mainloop_start(mMainLoop);
    int i = 1;
    while (i) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        std::cout << "cycle number " << i++ << std::endl;
    }
}

Application* Application::convert_to_application(void *userdata) {
    return static_cast<Application*>(userdata);
}

void Application::fill_sink_inputs() {
    pa_operation* op = pa_context_get_sink_input_info_list(this->mContext, [](pa_context* c, const pa_sink_input_info* i, int eol, void* userdata) {
        // this->sink_inputs.
    }, this);
    // pa_operation* op = pa_context_get_sink_info_list(application->mContext, foo, nullptr);
    pa_operation_unref(op);
}

void Application::start() {
    
}

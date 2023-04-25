#include "Application.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <stdio.h>

void sleep() {
    std::this_thread::sleep_for(std::chrono::seconds(1));
}

void quit(const std::string& message) {
    std::cerr << message << std::endl;
    exit(1);
}

void Application::sinkListCallback(pa_context *c, const pa_sink_input_info* info, int eol, void *userdata) {
    if (!info && eol) return;
    auto application = Application::convertToApplication(userdata);
    application->addSinkInput(info);
}

void Application::clientListCallback(pa_context *c, const pa_client_info* info, int eol, void *userdata) {
    if (!info && eol) return;
    if (info != nullptr) {
        std::cout << "client " << info->name << " index " << info->index << std::endl;
    }
}


void Application::contextReadyCallback(pa_context *c, void *userdata) {
    auto application = Application::convertToApplication(userdata);
    auto state = pa_context_get_state(c);
    switch(state) {
        case PA_CONTEXT_CONNECTING:
            std::cout << "connecting to audio server..." << std::endl;
            break;
        case PA_CONTEXT_READY: {
            std::cout << "connected to audio server" << std::endl;
            pa_operation* op;
            if (!(op = pa_context_get_client_info_list(application->mContext, &Application::clientListCallback, userdata))) {
                quit("pa_context_get_client_info_list error");
            }
            pa_operation_unref(op);
            if (!(op = pa_context_get_sink_input_info_list(application->mContext, &Application::sinkListCallback, userdata))) {
                quit("pa_context_get_sink_input_info_list error");
            }
            pa_operation_unref(op);
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
    pa_context_set_state_callback(mContext, &Application::contextReadyCallback, this);
    pa_context_connect(mContext, nullptr, PA_CONTEXT_NOFLAGS, nullptr);
    pa_threaded_mainloop_start(mMainLoop);
    while (true) {
        sleep();
    }
}

void Application::addSinkInput(const pa_sink_input_info* i) {
    std::cout << "index of " << i->name << " is " << i->index << " and client is " << i->client << std::endl;
    // this->sinkInputs->push_back(i);
}

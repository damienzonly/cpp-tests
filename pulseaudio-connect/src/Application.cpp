#include "Application.hpp"
#include <iostream>
#include <chrono>
#include <thread>
#include <stdio.h>
#include <condition_variable>
#include <mutex>

struct AsyncData {
    std::mutex mtx;
    std::condition_variable cv;
    bool isFinished = false;
    Application* application;
};

void sleep() {
    std::this_thread::sleep_for(std::chrono::seconds(1));
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
    std::cout << "main thread id is " << std::this_thread::get_id() << std::endl;
    this->mMainLoop = pa_threaded_mainloop_new();
    this->mContext = pa_context_new(pa_threaded_mainloop_get_api(this->mMainLoop), "myname");
    pa_context_set_state_callback(mContext, &Application::state_callback, this);
    pa_context_connect(mContext, nullptr, PA_CONTEXT_NOFLAGS, nullptr);
    pa_threaded_mainloop_start(mMainLoop);
    while (true) {
        sleep();
    }
}

typedef void (*list_callback_t)(pa_context*, const pa_sink_input_info*, int, void*);

void Application::fill_sink_inputs() {
    
    AsyncData d;
    d.application = this;
    d.isFinished = false;
    
    auto list_callback = [](pa_context* c, const pa_sink_input_info* i, int eol, void* userdata) {
        std::cout << "list_callback thread id is " << std::this_thread::get_id() << std::endl;
        auto asyncData = static_cast<AsyncData*>(userdata);
        std::cout << "locking inside list_callback" << std::endl;
        if (i != nullptr) {
            std::cout << "appending \"" << i->name << "\" to list" << std::endl;
            asyncData->application->sink_inputs->push_back(i);
        } else if (eol) {
            std::unique_lock<std::mutex> lock(asyncData->mtx);
            asyncData->isFinished = true;
            asyncData->cv.notify_all();
            return;
        };
    };
    pa_operation* op = pa_context_get_sink_input_info_list(this->mContext, list_callback, &d);
    std::cout << "pointer to mutex is " << &d.mtx << std::endl;
    std::unique_lock<std::mutex> lock(d.mtx);
    d.cv.wait(lock, [&d] {
        std::cout << "finished is " << d.isFinished << std::endl;
        return d.isFinished;
    });
    pa_operation_unref(op);
}

void Application::start() {
    this->fill_sink_inputs();
    std::cout << "listing sinks" << std::endl;
    for (auto sink: *this->sink_inputs) {
        std::cout << "sink name: " << sink->name << std::endl;
    }
    // how do i guarantee that the asyncronous operation is completed here?
}

void Application::createStreams() {
    // pa_stream *stream = pa_stream_new(this->mContext, "MyMixer Stream", &sampleSpec, nullptr);
}
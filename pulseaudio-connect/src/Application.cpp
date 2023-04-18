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

AsyncData d;

void printThreadId(std::string prefix) {
    std::cout << prefix << " thread id is ";
    printf("%x", std::this_thread::get_id());
    std::cout << std::endl;
}

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
            // printThreadId("state callback");
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
    // printThreadId("init function");
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

void list_callback(pa_context* c, const pa_sink_input_info* i, int eol, void* userdata) {
    // printThreadId("list_callback callback");
    auto asyncData = static_cast<AsyncData*>(userdata);
    if (i != nullptr) {
        std::cout << "appending \"" << i->name << "\" to list" << std::endl;
        asyncData->application->addSinkInput(i);
    } else if (eol) {
        std::lock_guard<std::mutex> lock(asyncData->mtx);
        asyncData->isFinished = true;
        asyncData->cv.notify_all();
        return;
    };
}

void Application::addSinkInput(const pa_sink_input_info* i) {
    this->sink_inputs->push_back(i);
}

void Application::fill_sink_inputs() {
    d.application = this;
    d.isFinished = false;
    pa_operation* op = pa_context_get_sink_input_info_list(this->mContext, list_callback, &d);
    std::unique_lock<std::mutex> lock(d.mtx);
    // d.cv.wait(lock, [] { return d.isFinished; });
    pa_operation_unref(op);
}

void Application::start() {
    this->fill_sink_inputs();
    std::cout << "fill_sink should be finished" << std::endl;
    for (auto sink: *this->sink_inputs) {
        std::cout << "sink name: " << sink->name << std::endl;
    }
}

void Application::createStreams() {
    // pa_stream *stream = pa_stream_new(this->mContext, "MyMixer Stream", &sampleSpec, nullptr);
}
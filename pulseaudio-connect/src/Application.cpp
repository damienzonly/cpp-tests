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
    application->addSinkInput(info, userdata);
}

void Application::clientListCallback(pa_context *c, const pa_client_info* info, int eol, void *userdata) {
    if (!info && eol) return;
    auto application = Application::convertToApplication(userdata);
    application->addClientInfo(info);
}

void Application::eventCallback(pa_context *c, pa_subscription_event_type_t t, uint32_t index, void *userdata) {
    auto isRemoveEvent = [t]() {
        return ((t & PA_SUBSCRIPTION_EVENT_TYPE_MASK) == PA_SUBSCRIPTION_EVENT_REMOVE);
    };
    auto application = Application::convertToApplication(userdata);
    pa_operation* op;
    switch (t & PA_SUBSCRIPTION_EVENT_FACILITY_MASK) {
        case PA_SUBSCRIPTION_EVENT_SINK_INPUT:
            if (isRemoveEvent()) {
                // remove sink
                application->removeSinkInput(index);
            } else {
                // add sink
                if (!(op = pa_context_get_sink_input_info(application->mContext, index, Application::sinkListCallback, userdata))) {
                    quit("error while fetching sink input info");
                }
                pa_operation_unref(op);
            }
            break;
        case PA_SUBSCRIPTION_EVENT_CLIENT:
            if (isRemoveEvent()) {
                application->removeClientInfo(index);
            } else {
                if (!(op = pa_context_get_client_info(application->mContext, index, Application::clientListCallback, userdata))) {
                    quit("error fetching client info");
                }
                pa_operation_unref(op);
            }
            break;
    }

}


void Application::contextReadyCallback(pa_context *c, void *userdata) {
    auto application = Application::convertToApplication(userdata);
    auto state = pa_context_get_state(c);
    pa_operation* op;
    switch(state) {
        case PA_CONTEXT_CONNECTING:
            std::cout << "connecting to audio server..." << std::endl;
            break;
        case PA_CONTEXT_READY: {
            std::cout << "context is ready" << std::endl;
            pa_context_set_subscribe_callback(c, Application::eventCallback, userdata);
            if (!(op = pa_context_subscribe(c, (pa_subscription_mask_t)
                                           (PA_SUBSCRIPTION_MASK_SINK|
                                            PA_SUBSCRIPTION_MASK_SOURCE|
                                            PA_SUBSCRIPTION_MASK_SINK_INPUT|
                                            PA_SUBSCRIPTION_MASK_SOURCE_OUTPUT|
                                            PA_SUBSCRIPTION_MASK_CLIENT|
                                            PA_SUBSCRIPTION_MASK_SERVER|
                                            PA_SUBSCRIPTION_MASK_CARD), NULL, NULL))) {
                quit("pa_context_subscribe() failed");
            }
            pa_operation_unref(op);
            
            if (!(op = pa_context_get_client_info_list(application->mContext, &Application::clientListCallback, userdata))) {
                quit("pa_context_get_client_info_list init error");
            }
            pa_operation_unref(op);
            if (!(op = pa_context_get_sink_input_info_list(application->mContext, &Application::sinkListCallback, userdata))) {
                quit("pa_context_get_sink_input_info_list init error");
            }
            pa_operation_unref(op);
            
            break;
        }
        case PA_CONTEXT_FAILED:
            std::cerr << "failed to connect to audio server" << std::endl;
            break;
        case PA_CONTEXT_TERMINATED:
            std::cerr << "connection to audio server terminated" << std::endl;
            break;
    };
}



void Application::init() {
    this->mMainLoop = pa_threaded_mainloop_new();
    this->mContext = pa_context_new(pa_threaded_mainloop_get_api(this->mMainLoop), "Smix");
    pa_context_set_state_callback(mContext, &Application::contextReadyCallback, this);
    pa_context_connect(mContext, nullptr, PA_CONTEXT_NOFLAGS, nullptr);
    pa_threaded_mainloop_start(mMainLoop);
    while (true) {
        sleep();
    }
}

void Application::addSinkInput(const pa_sink_input_info* i, void* userdata) {
    if (this->sinkInputs->count(i->index)) {
        std::cout << "existing stream:" << i->name << " id " << i->index << std::endl;
    } else {
        std::cout << "adding stream:" << i->name << " id " << i->index << std::endl;
        (*this->sinkInputs)[i->index] = i;
        this->createIOStreams(i, userdata);
    }
}

void Application::removeSinkInput(const uint32_t index) {
    if (this->sinkInputs->count(index)) {
        std::cout << "removing stream: " << index << std::endl;
        this->sinkInputs->erase(index);
    }
}

void Application::addClientInfo(const pa_client_info* i) {
    // std::cout << "stream: " << i->name << " is " << i->index << " and client is " << i->client << std::endl;
    // auto already = this->sinkInputs[i->index];
    if (this->clients->count(i->index)) {
        std::cout << "existing client:" << i->name << std::endl;
    } else {
        std::cout << "adding client:" << i->name << std::endl;
        (*this->clients)[i->index] = i;
    }
}

void Application::removeClientInfo(const uint32_t index) {
    if (this->clients->count(index)) {
        std::cout << "removing client: " << index << std::endl;
        this->clients->erase(index);
    }
}

float randomFloat(float a, float b) {
    float random = ((float) rand()) / (float) RAND_MAX;
    float diff = b - a;
    float r = random * diff;
    return a + r;
}

void streamStateCallback(pa_stream *stream, void *userdata) {
    switch (pa_stream_get_state(stream)) {
        case PA_STREAM_READY: {
            std::cout << "read stream is ready" << std::endl;
            break;
        }
        case PA_STREAM_FAILED: {
            quit("failed state of read stream");
            break;
        }
    }
}

void streamReadCallback(pa_stream *stream, size_t length, void *userdata) {
    pa_stream* writeStream = (pa_stream*) userdata;
    const void* data;
    pa_stream_peek(stream, &data, &length);
    float* samples = (float*) data;
    auto size = length/sizeof(float);
    for (auto i = 0; i < size; ++i) {
        samples[i] = randomFloat(-0.5f, 0.5f);
    }
    pa_stream_write(writeStream, data, length, nullptr, 0, PA_SEEK_RELATIVE);
    pa_stream_drop(stream);
    pa_stream_drop(writeStream);
}

void Application::createIOStreams(const pa_sink_input_info* sinkInput, void* userdata) {
    std::string s(sinkInput->name);
    std::string readStreamName = s + " read";
    std::string writeStreamName = s + " write";
    
    StreamData* inputStreamData = new StreamData;
    StreamData* outputStreamData = new StreamData;
    IOStreamData* ioStreamData = new IOStreamData;
    ioStreamData->input = inputStreamData;
    ioStreamData->output = outputStreamData;
    (*this->streamsData)[sinkInput->index] = ioStreamData;

    pa_stream* readStream = pa_stream_new(this->mContext, readStreamName.c_str(), &sinkInput->sample_spec, nullptr);
    pa_stream* writeStream = pa_stream_new(this->mContext, writeStreamName.c_str(), &sinkInput->sample_spec, nullptr);

    if (!readStream) {
        quit("error creating read stream for " + readStreamName);
    }
    if (!writeStream) {
        quit("error creating write stream for " + writeStreamName);
    }
    
    pa_stream_set_state_callback(readStream, streamStateCallback, userdata);
    pa_stream_set_read_callback(readStream, streamReadCallback, writeStream);
    pa_stream_connect_record(readStream, NULL, &inputStreamData->buffer_attr, PA_STREAM_NOFLAGS);
}
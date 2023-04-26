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
                std::cout << "client id " << index << " is being added" << std::endl;
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
    this->mContext = pa_context_new(pa_threaded_mainloop_get_api(this->mMainLoop), "myname");
    pa_context_set_state_callback(mContext, &Application::contextReadyCallback, this);
    pa_context_connect(mContext, nullptr, PA_CONTEXT_NOFLAGS, nullptr);
    pa_threaded_mainloop_start(mMainLoop);
    while (true) {
        sleep();
    }
}

void Application::addSinkInput(const pa_sink_input_info* i) {
    // std::cout << "stream: " << i->name << " is " << i->index << " and client is " << i->client << std::endl;
    // auto already = this->sinkInputs[i->index];
    if (this->sinkInputs->count(i->index)) {
        std::cout << "existing stream:" << i->name << " id " << i->index << std::endl;
    } else {
        std::cout << "adding stream:" << i->name << " id " << i->index << std::endl;
        (*this->sinkInputs)[i->index] = i;
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
        std::cout << "existing client:" << i->name << " id " << i->index << std::endl;
    } else {
        std::cout << "adding client:" << i->name << " id " << i->index << std::endl;
        (*this->clients)[i->index] = i;
    }
}

void Application::removeClientInfo(const uint32_t index) {
    if (this->clients->count(index)) {
        std::cout << "removing client: " << index << std::endl;
        this->clients->erase(index);
    }
}
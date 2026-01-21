
#pragma once

#include "juce_core/juce_core.h"

template<typename func_t_>
class lambda_wrapper_change_listener : public juce::ChangeListener {
    func_t_ func;

    template<typename func_t>
    lambda_wrapper_change_listener(func_t&& func) : func(std::forward<func_t>(func)) {}

    void changeListenerCallback(juce::ChangeBroadcaster* source) {
        func(source);
    }
};

class lambda_wrapper_change_listener_lifetime_manager {
public:
    template<typename func_t>
    juce::ChangeListener* create_listener(func_t&& func) {
        listeners_.emplace_back(std::forward<func_t>(func));
    }

private:
    std::vector<std::unique_ptr<juce::ChangeListener>> listeners_;
};
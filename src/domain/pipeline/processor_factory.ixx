module;

#include <string>
#include <functional>
#include <map>
#include <memory>
#include <iostream>

export module processor_factory;

import domain.pipeline;

namespace domain::pipeline {

using ProcessorCreator = std::function<std::shared_ptr<IFrameProcessor>(const void* context_ptr)>;

export class ProcessorFactory {
public:
    static ProcessorFactory& instance() {
        static ProcessorFactory factory;
        return factory;
    }

    void register_processor(const std::string& type, ProcessorCreator creator) {
        creators_[type] = creator;
        std::cout << "ProcessorFactory: Registered processor type '" << type << "'" << std::endl;
    }

    std::shared_ptr<IFrameProcessor> create(const std::string& type, const void* context_ptr) {
        auto it = creators_.find(type);
        if (it != creators_.end()) { return it->second(context_ptr); }
        std::cerr << "ProcessorFactory: Unknown processor type '" << type << "'" << std::endl;
        return nullptr;
    }

private:
    ProcessorFactory() = default;
    std::map<std::string, ProcessorCreator> creators_;
};

export struct ProcessorRegistrar {
    ProcessorRegistrar(const std::string& type, ProcessorCreator creator) {
        ProcessorFactory::instance().register_processor(type, creator);
    }
};
} // namespace domain::pipeline

/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */

#ifndef CXX_CELIX_COMPONENT_MANAGER_H
#define CXX_CELIX_COMPONENT_MANAGER_H

#include <memory>
#include <unordered_map>

#include "celix/BundleContext.h"

namespace celix {

    enum class ComponentState {
        Disabled,
        Uninitialized,
        Initialized,
        Started
    };

    template<typename T, typename I>
    class ServiceDependency; //forward declaration
    class GenericServiceDependency; //forward declaration

    class GenericComponentManager {
    public:
        virtual ~GenericComponentManager() = default;

        ComponentState getState() const;
        bool isEnabled() const;
        bool isResolved() const;

        //TODO make friend of SvcDep
        void updateState();
    protected:
        GenericComponentManager(std::shared_ptr<BundleContext> _ctx, std::shared_ptr<void> cmpInstance) : ctx{std::move(_ctx)}, instance{std::move(cmpInstance)} {};

        void removeServiceDependency(long serviceDependencyId);
        void setEnabled(bool enable);


        /**** Fields ****/
        mutable std::mutex mutex{}; //protects below
        ComponentState state = ComponentState::Disabled;
        bool enabled = false;
        std::function<void()> init{};
        std::function<void()> start{};
        std::function<void()> stop{};
        std::function<void()> deinit{};

        std::shared_ptr<BundleContext> ctx;
        std::shared_ptr<void> instance;

        long nextServiceDependencyId = 1L;
        std::unordered_map<long,GenericServiceDependency> serviceDependencies{};
    };

    template<typename T>
    class ComponentManager : public GenericComponentManager {
    public:
        using ComponentType = T;

        ComponentManager(std::shared_ptr<BundleContext> ctx, std::shared_ptr<T> cmpInstance);
        ~ComponentManager() override = default;

        ComponentManager<T>& enable();
        ComponentManager<T>& disable();

        ComponentManager<T>& setCallbacks(
                void (T::*init)(),
                void (T::*start)(),
                void (T::*stop)(),
                void (T::*deinit)());

//        template<typename I>
//        ServiceDependency<T,I>& addServiceDependency();
//
//        template<typename F>
//        ServiceDependency<T,F>& addFunctionServiceDependency(const std::string &functionName);

    };

    enum class Cardinality {
        One,
        Many
    };

    class GenericServiceDependency {
    public:
        virtual ~GenericServiceDependency() = default;

        bool isResolved() const;
        Cardinality getCardinality() const;
        bool getRequired() const;
        const std::string& getFilter() const;

    protected:
        GenericServiceDependency(std::shared_ptr<BundleContext> ctx);

        //Fields
        mutable std::mutex mutex{}; //protects below
        bool required = false;
        std::string filter{};
        Cardinality cardinality = Cardinality::One;
        std::vector<ServiceTracker> tracker{}; //max 1

        std::shared_ptr<BundleContext> ctx;
    };

    template<typename T, typename I>
    class ServiceDependency : public GenericServiceDependency {
    public:
        using ComponentType = T;
        using ServiceType = I;

        ServiceDependency(std::shared_ptr<ComponentManager<T>> mng);
        ~ServiceDependency() override = default;

        ServiceDependency<T,I>& setFilter(const std::string &filter);
        ServiceDependency<T,I>& setRequired(bool required);
        ServiceDependency<T,I>& setCardinality(celix::Cardinality cardinality);
        ServiceDependency<T,I>& setCallbacks(void (T::*set)(std::shared_ptr<I>));
        ServiceDependency<T,I>& setCallbacks(void (T::*add)(std::shared_ptr<I>), void (T::*remove)(std::shared_ptr<I>));
        //TODO update callback

        void enable();
        void disable();
        bool isResolved();
    private:
        std::function<void(std::shared_ptr<I>)> set{};
        std::function<void(std::shared_ptr<I>)> add{};
        std::function<void(std::shared_ptr<I>)> rem{};
        std::shared_ptr<ComponentManager<T>> mng;
    };
}



/**************************** IMPLEMENTATION *************************************************************************/

template<typename T>
celix::ComponentManager<T>::ComponentManager(std::shared_ptr<BundleContext> ctx, std::shared_ptr<T> cmpInstance) :
    GenericComponentManager(ctx, std::static_pointer_cast<void>(cmpInstance)) {}

template<typename T>
celix::ComponentManager<T>& celix::ComponentManager<T>::enable() {
    this->setEnabled(true);
    return *this;
}

template<typename T>
celix::ComponentManager<T>& celix::ComponentManager<T>::disable() {
    this->setEnabled(false);
    return *this;
}

template<typename T>
celix::ComponentManager<T>& celix::ComponentManager<T>::setCallbacks(
        void (T::*init)(),
        void (T::*start)(),
        void (T::*stop)(),
        void (T::*deinit)()) {
    std::lock_guard<std::mutex> lck{mutex};
    auto initFunc = [this,init]() {
        if (init) {
            (this->instance->*init)();
        }
    };
    auto startFunc = [this,start]() {
        if (start) {
            (this->instance->*start)();
        }
    };
    auto stopFunc = [this,stop]() {
        if (stop) {
            (this->instance->*stop)();
        }
    };
    auto deinitFunc = [this,deinit]() {
        if (deinit) {
            (this->instance->*deinit)();
        }
    };

}

template<typename T, typename I>
celix::ServiceDependency<T,I>::ServiceDependency(std::shared_ptr<ComponentManager<T>> _mng) : mng{std::move(_mng)} {
    add = [this](std::shared_ptr<I>) {
        mng->updateState();
    };
    rem = [this](std::shared_ptr<I>) {
        mng->updateState();
    };
}

template<typename T, typename I>
void celix::ServiceDependency<T,I>::enable() {
    std::vector<celix::ServiceTracker> localTrackers{};
    {
        std::lock_guard<std::mutex> lck{mutex};
        std::swap(localTrackers, tracker);
        assert(tracker.size() == 0);

        ServiceTrackerOptions<I> opts{};
        opts.filter = this->filter;
        opts.set = set;
        opts.add = add;
        opts.remove = rem;

        this->tracker.push_back(this->ctx->trackServices(opts));
    }

}

template<typename T, typename I>
void celix::ServiceDependency<T,I>::disable() {
    std::vector<celix::ServiceTracker> localTrackers{};
    {
        std::lock_guard<std::mutex> lck{mutex};
        std::swap(localTrackers, tracker);
        assert(tracker.size() == 0);
    }
}


template<typename T, typename I>
celix::ServiceDependency<T,I>& celix::ServiceDependency<T,I>::setCallbacks(void (T::*setfp)(std::shared_ptr<I>)) {
    std::lock_guard<std::mutex> lck{mutex};
    set = [this, setfp](std::shared_ptr<I> svc) {
        (mng->instance->*setfp)(svc);
    };
    return *this;
}

template<typename T, typename I>
celix::ServiceDependency<T,I>& celix::ServiceDependency<T,I>::setCallbacks(void (T::*addfp)(std::shared_ptr<I>), void (T::*remfp)(std::shared_ptr<I>)) {
    std::lock_guard<std::mutex> lck{mutex};
    add = [this, addfp](std::shared_ptr<I> svc) {
        (mng->instance->*addfp)(svc);
        mng->updateState();
    };
    rem = [this, remfp](std::shared_ptr<I> svc) {
        (mng->instance->*remfp)(svc);
        mng->updateState();
    };
    return *this;
}

template<typename T, typename I>
celix::ServiceDependency<T,I>& celix::ServiceDependency<T,I>::setFilter(const std::string &f) {
    std::lock_guard<std::mutex> lck{mutex};
    filter = f;
    return *this;
}

template<typename T, typename I>
celix::ServiceDependency<T,I>& celix::ServiceDependency<T,I>::setRequired(bool r) {
    std::lock_guard<std::mutex> lck{mutex};
    required = r;
    return *this;
}

template<typename T, typename I>
celix::ServiceDependency<T,I>& celix::ServiceDependency<T,I>::setCardinality(celix::Cardinality c) {
    std::lock_guard<std::mutex> lck{mutex};
    cardinality = c;
    return *this;
}


#endif //CXX_CELIX_PROPERTIES_H

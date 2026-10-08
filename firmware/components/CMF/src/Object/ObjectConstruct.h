#ifndef CMF_OBJECTCONSTRUCT_H
#define CMF_OBJECTCONSTRUCT_H

#include <tuple>

template<typename T, typename ...Args>
struct ObjectConstruct {
public:
    constexpr explicit ObjectConstruct(void* arguments) {
        if(arguments == nullptr) {
            return;
        }

        args = static_cast<std::tuple<Args...>*>(arguments);
    }

    constexpr std::tuple<Args...>* getArguments() { return args; }

    constexpr T* create(void* addr) const {
        return std::apply([addr]<typename ...__Args>(__Args&&... args) { return new(addr) T(std::move<__Args>(args)...); }, *args);
    }

private:
    std::tuple<Args...>* args = nullptr;
};

template<typename T>
struct ObjectConstruct<T, void> {
public:
    constexpr explicit ObjectConstruct(void* arguments) {};

    constexpr T* create(void* addr) const {
        return new(addr) T();
    }
};

#endif //CMF_OBJECTCONSTRUCT_H
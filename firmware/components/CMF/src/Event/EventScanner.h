#ifndef EVENTSCANNER_H
#define EVENTSCANNER_H

#include <mutex>
#include <set>
#include "Entity/AsyncEntity.h"
#include "Object/Class.h"

class EventScanner : public AsyncEntity {
    GENERATED_BODY(EventScanner, AsyncEntity, void)

public:
    explicit EventScanner(bool internalStack = true);
    virtual ~EventScanner() override;

    void unlock() const noexcept;
    void registerHandle(class EventHandleBase* handle) noexcept;
    void unregisterHandle(EventHandleBase* handle) noexcept;

protected:
    virtual inline void tick(float deltaTime) noexcept override;

private:
    SemaphoreHandle_t semaphore;
    std::mutex registerMutex;
    std::set<EventHandleBase*> eventHandles;
};

#endif //EVENTSCANNER_H
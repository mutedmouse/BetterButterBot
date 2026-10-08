#ifndef BUTTERBOT_FIRMWARE_UDPEMITTER_H
#define BUTTERBOT_FIRMWARE_UDPEMITTER_H

#include <lwip/sockets.h>
#include <Object/Object.h>
#include <Object/Class.h>

class UDPEmitter : public Object {
    GENERATED_BODY(UDPEmitter, Object, void)

public:
    UDPEmitter() noexcept;
    virtual ~UDPEmitter() noexcept override;

    bool write(const std::vector<uint8_t>& buffer) noexcept;
    bool write(const uint8_t* buffer, size_t count) noexcept;

private:
    int socket = -1;
    sockaddr_in dest{};
};

#endif //BUTTERBOT_FIRMWARE_UDPEMITTER_H
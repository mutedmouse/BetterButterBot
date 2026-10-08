#ifndef CMF_I2S_H
#define CMF_I2S_H

#include "Object/Class.h"
#include "Object/Object.h"
#include <driver/i2s_std.h>
#include <driver/i2s_pdm.h>

class I2S : public Object {
	GENERATED_BODY(I2S, Object, CONSTRUCTOR_PACK(i2s_port_t, i2s_std_config_t))

public:
	I2S(i2s_port_t port = I2S_NUM_AUTO, i2s_std_config_t config = {});

	//Note - no default parameters, since multiple default constructors are ambiguous inside CMF Object architecture
	I2S(i2s_port_t port, i2s_pdm_rx_config_t config);

	~I2S() override;

	/**
	 * Write data to I2S channel.
	 * @param data data pointer
	 * @param size number of bytes to be written
	 * @return number of bytes successfully written, 0 if error
	 */
	size_t write(uint8_t* data, size_t bytes);

	/**
	 * Read data from I2S channel.
	 * @param data data pointer
	 * @param bytes number of bytes to be read
	 * @return number of bytes successfully read, 0 if error
	 */
	size_t read(uint8_t* data, size_t bytes);

private:
	i2s_chan_handle_t tx_chan = nullptr;
	i2s_chan_handle_t rx_chan = nullptr;
};

#endif //CMF_I2S_H
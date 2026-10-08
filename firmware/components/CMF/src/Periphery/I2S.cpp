#include "I2S.h"
#include "Log/Log.h"

DEFINE_LOG(I2S)

I2S::I2S(i2s_port_t port, i2s_std_config_t config){
	i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(port, I2S_ROLE_MASTER);
	chan_cfg.auto_clear = true;
	i2s_new_channel(&chan_cfg, &tx_chan, nullptr);

	ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_chan, &config));

	i2s_channel_enable(tx_chan);
}

I2S::I2S(i2s_port_t port, i2s_pdm_rx_config_t config){
	i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(port, I2S_ROLE_MASTER);
	i2s_new_channel(&chan_cfg, nullptr, &rx_chan);

	ESP_ERROR_CHECK(i2s_channel_init_pdm_rx_mode(rx_chan, &config));

	i2s_channel_enable(rx_chan);
}

I2S::~I2S(){
	if(tx_chan){
		i2s_channel_disable(tx_chan);
		i2s_del_channel(tx_chan);
	}

	if(rx_chan){
		i2s_channel_disable(rx_chan);
		i2s_del_channel(rx_chan);
	}
}

size_t I2S::write(uint8_t* data, size_t bytes){
	if(!tx_chan){
		CMF_LOG(I2S, LogLevel::Error, "No TX channel configured!");
		return ESP_ERR_INVALID_STATE;
	}

	size_t bytesWritten = 0;
	auto err = i2s_channel_write(tx_chan, data, bytes, &bytesWritten, portMAX_DELAY);

	if(err != ESP_OK){
		CMF_LOG(I2S, LogLevel::Warning, "i2s_channel_write failed: %s", esp_err_to_name(err));
		return 0;
	}

	return bytesWritten;
}

size_t I2S::read(uint8_t* data, size_t bytes){
	if(!rx_chan){
		CMF_LOG(I2S, LogLevel::Error, "No RX channel configured!");
		return 0;
	}

	size_t bytesRead = 0;
	auto err = i2s_channel_read(rx_chan, data, bytes, &bytesRead, portMAX_DELAY);

	if(err != ESP_OK){
		CMF_LOG(I2S, LogLevel::Warning, "i2s_channel_read failed: %s", esp_err_to_name(err));
		return 0;
	}

	return (int32_t) bytesRead;
}

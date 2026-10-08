#ifndef CODEE_FIRMWARE_ADC_H
#define CODEE_FIRMWARE_ADC_H

#include "Object/Class.h"
#include "Object/Object.h"
#include <esp_adc/adc_oneshot.h>
#include <soc/gpio_num.h>
#include <mutex>

class ADCUnit : public Object {
	GENERATED_BODY(ADCUnit, Object, CONSTRUCTOR_PACK(adc_unit_t))

public:
	static ADCUnit* getADCUnit(adc_unit_t unit);

	static ADCUnit* getADCUnit(gpio_num_t gpio);

	explicit ADCUnit(adc_unit_t unit = ADC_UNIT_1);

	virtual ~ADCUnit() override;

	adc_unit_t getUnitID() const;

	void config(adc_channel_t chan, const adc_oneshot_chan_cfg_t& cfg);

	esp_err_t read(adc_channel_t chan, int& valueOut, adc_cali_handle_t cali = nullptr) const;

	/**
	 * Sometimes ADC will start having an offset during sleep and after wakeup.
	 * This method enables easy ADC unit reinitialization when needed.
	 *
	 * Necessary on certain HW revisions because of this ADC reading glitch after/during light sleep
	 * (https://github.com/espressif/esp-idf/issues/12612)
	 *
	 * TODO - refactor reinitialization since ADCReaders (channels) need to be reconfigured after this call (CMF-50).
	 */
	void reinit();

private:
	adc_oneshot_unit_handle_t hndl;
	const adc_unit_t unit;

	mutable std::mutex mutex;

	static StrongObjectPtr<ADCUnit> units[SOC_ADC_PERIPH_NUM];
};

#endif //CODEE_FIRMWARE_ADC_H

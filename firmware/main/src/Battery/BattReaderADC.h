#ifndef BUTTERBOT_FIRMWARE_BATTREADERADC_H
#define BUTTERBOT_FIRMWARE_BATTREADERADC_H

#include <Services/ADCReader.h>

/**
 * First multiplies the result by given factor, then adds the offset.
 * Useful for converting from voltage divider output to actual source voltage.
 */
class FactorOffset_ADCFilter : public ADCFilter {
	GENERATED_BODY(FactorOffset_ADCFilter, ADCFilter, CONSTRUCTOR_PACK(float, float))
public:
	FactorOffset_ADCFilter(float factor, float offset) : factor(factor), offset(offset){
	}

	float apply(float sample) override{
		return sample * factor + offset;
	}

	float getOffset() const{
		return offset;
	}

	void setOffset(float offset){
		FactorOffset_ADCFilter::offset = offset;
	}

	float getFactor() const{
		return factor;
	}

	void setFactor(float factor){
		FactorOffset_ADCFilter::factor = factor;
	}

private:
	float factor = 1.0f;
	float offset = 0.f;
};

/**
 * Converts value from the specified range to [0.0 - 100.0]
 */
class Remap_ADCFilter : public ADCFilter {
	GENERATED_BODY(Remap_ADCFilter, ADCFilter, CONSTRUCTOR_PACK(float, float));

public:
	Remap_ADCFilter(float min, float max) : min(min), max(max){
	}

	float apply(float sample) override{
		if(min == max) return Super::apply(sample);

		sample = std::clamp(sample, min, max);

		return (sample - min) / (max - min) * 100.0f;
	}

private:
	float min = 0, max = 0;
};

class Composite_ADCFilter : public ADCFilter {
	GENERATED_BODY(Composite_ADCFilter, ADCFilter, CONSTRUCTOR_PACK(std::vector<StrongObjectPtr<ADCFilter>>))
public:
	Composite_ADCFilter(std::vector<StrongObjectPtr<ADCFilter>> filters) : filters(std::move(filters)){
	}

	float apply(float sample) override{
		for(auto& filter : filters){
			sample = filter->apply(sample);
		}

		return sample;
	}

private:
	std::vector<StrongObjectPtr<ADCFilter>> filters;
};


#endif //BUTTERBOT_FIRMWARE_BATTREADERADC_H

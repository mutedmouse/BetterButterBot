#ifndef BUTTERBOT_FIRMWARE_HYSTERESIS_H
#define BUTTERBOT_FIRMWARE_HYSTERESIS_H

#include <array>

template<size_t T>
class Hysteresis {
public:
	/**
	 * @param thresholds Ordered low to high. Should include min and max values
	 * count(levels) = count(thresholds)-1
	 */
	Hysteresis(std::array<int, T> thresholds, int margin) : Thresholds(thresholds), LevelCount(thresholds.size() - 1), Margin(margin), currentLevel(0){
	}

	int get() const{
		if(currentLevel < 0){
			return 0;
		} else if(currentLevel >= LevelCount){
			return LevelCount - 1;
		}

		return currentLevel;
	}

	int update(int val){
		int lb = Thresholds[currentLevel];

		if(currentLevel > 0){
			lb -= Margin; // subtract margin
		}

		int ub = Thresholds[currentLevel + 1];

		if(currentLevel < LevelCount){
			ub += Margin; // add margin
		}

		// now test if input is between the outer margins for current output value
		if(val < lb || val > ub){
			// determine new output level by scanning endPointInput array
			currentLevel = findLevel(val);
		}

		return get();
	}

	int reset(int val = 0){
		currentLevel = findLevel(val);
		return get();
	}

private:
	// define input to output conversion table/formula by specifying endpoints of the levels.
	// the number of end points is equal to the number of output levels plus one.
	// in the example below, output level 0 results from an input of between 0 and 112.
	// 1 results from an input of between 113 and 212 etc.
	const std::array<int, T> Thresholds;

	// set the number of output levels. These are numbered starting from 0.
	const int LevelCount; // 0..9

	// margin sets the 'stickyness' of the hysteresis or the relucatance to leave the current state.
	// It is measured in units of the the input level. As a guide it is a few percent of the
	// difference between two end points. Don't make the margin too wide or ranges may overlap.
	const int Margin;

	int currentLevel;

	int findLevel(int val){
		int i;
		for(i = 0; i < LevelCount; i++){
			if(val >= Thresholds[i] && val <= Thresholds[i + 1]){
				break;
			}
		}
		return i;
	}
};


#endif //BUTTERBOT_FIRMWARE_HYSTERESIS_H

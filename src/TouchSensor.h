#ifndef MULTITOUCH_BAND_TOUCHSENSOR_H
#define MULTITOUCH_BAND_TOUCHSENSOR_H

#include "Trill.h"
#include "Touch.h"


typedef std::array<unsigned int, MAX_CHANNELS> DataArray;

class MultitouchSensor {
protected:
    Trill sensor1;
    DataArray data{};

private:
    int num_touches = 0;

public:

    void setup(int prescaler) {
        int ret1 = sensor1.setup(Trill::TRILL_CRAFT);

        //TODO: Error handling
        if (ret1 != 0) {}

        delay(100);
        sensor1.setPrescaler(prescaler);
        delay(50);
        sensor1.updateBaseline();
        delay(100);
    }

    void read_data() {
        unsigned int loc = 0;
        unsigned int iterator = 0;
        auto readSensorData = [this, &loc, &iterator](Trill &sensor) {
            sensor.requestRawData();
            while (sensor.rawDataAvailable() && loc < MAX_CHANNELS) {
                if(iterator >= FIRST_CHANNEL){
                    this->data[loc++] = sensor.rawDataRead();
                }else{
                    sensor.rawDataRead();
                }
                iterator++;
            }
        };

        readSensorData(sensor1);
    }

    int find_next_zero_channel_in_data(int start_channel, int &channel_processed) {
        int &channel = start_channel;
        while (data[channel] != 0) {
            if (++channel == MAX_CHANNELS) channel = 0;
            if (++channel_processed == MAX_CHANNELS) break;
        }
        return channel;
    }

    int find_next_non_zero_channel_in_data(int start_channel, int &channel_processed) {
        int &channel = start_channel;
        while (data[channel] == 0) {
            if (++channel == MAX_CHANNELS) channel = 0;
            if (++channel_processed == MAX_CHANNELS) break;
        }
        return channel;
    }

    Touch get_next_touch_in_data(int &channel_processed, int &current_channel) {
        current_channel = find_next_non_zero_channel_in_data(current_channel, channel_processed);
        int dont_process;
        int channel_touch_ended = find_next_zero_channel_in_data(current_channel, dont_process);

        TouchBuilder touch_builder;
        while (current_channel != channel_touch_ended) {
            int last_channel = (current_channel - 1 + MAX_CHANNELS) % MAX_CHANNELS;
            int current_channel_data = data[current_channel];
            int last_channel_data = data[last_channel];

            if (current_channel_data >= last_channel_data) {
                if (touch_builder.biggest_pressure_channel != last_channel &&
                    touch_builder.biggest_pressure_channel != -1) {
                    touch_builder.update(current_channel_data, current_channel, false);
                    return touch_builder.create_touch(current_channel + 1);
                }
                touch_builder.update(current_channel_data, current_channel, true);
            } else {
                touch_builder.update(current_channel_data, current_channel, false);
            }


            if (++current_channel == MAX_CHANNELS) current_channel = 0;
            if (++channel_processed == MAX_CHANNELS) break;
        }

        return touch_builder.create_touch(current_channel);

    }


    TouchArray get_touches_from_data() {
        TouchArray touches{};
        int touch_num = 0;

        int channel_processed = 0;
        int current_channel = find_next_zero_channel_in_data(0, channel_processed);
        channel_processed = 0;
        num_touches = 0;

        while (channel_processed < MAX_CHANNELS && touch_num < MAX_TOUCHES) {
            Touch touch = get_next_touch_in_data(channel_processed, current_channel);
            if (!touch.is_empty()) {
                touches[touch_num++] = touch;
                num_touches++;
            }
        }

        return touches;
    }

    int get_num_touches() const { return num_touches; }
};

#endif //MULTITOUCH_BAND_TOUCHSENSOR_H

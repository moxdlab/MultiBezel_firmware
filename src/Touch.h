//
// Created by Joel Neumann on 22.12.23.
//

#ifndef MULTITOUCH_BAND_TOUCH_H
#define MULTITOUCH_BAND_TOUCH_H

#define MAX_CHANNELS 8
#define FIRST_CHANNEL 7

class Touch {
private:
    float position;
    int channels;
    int pressure;

public:
    Touch() : position(-1), channels(0), pressure(0) {}

    Touch(float position, int channels, int pressure) : position(position), channels(channels), pressure(pressure) {}

    float get_position() const {
        return position;
    }

    int get_channels() const {
        return channels;
    }

    int get_pressure() const {
        return pressure;
    }

    void set_position(float pos) {
        position = pos;
    }

    void increase_channels(int amount = 1) {
        channels += amount;
    }

    void set_pressure(int pres) {
        pressure = pres;
    }

    bool is_empty() const {
        return position == -1 && channels == 0 && pressure == 0;
    }

    float get_position_in_degrees() const {
        if (position == -1) return -1;
        return 360 - (position * 360 / MAX_CHANNELS);
    }
};


typedef std::array<Touch, MAX_TOUCHES> TouchArray;

class TouchBuilder {
public:
    int channels = 0;
    int pressure = 0;
    int weighted_index_pressure = 0;
    int biggest_pressure_channel = -1;

    void update(int &channel_data, int &current_channel, bool update_biggest_pressure_channel) {
        weighted_index_pressure += channels * channel_data;
        channels++;
        pressure += channel_data;
        if (update_biggest_pressure_channel) biggest_pressure_channel = current_channel;
    }

    Touch create_touch(int last_channel_of_touch) const {
        if (channels == 0) return {};

        float position = (float) weighted_index_pressure / (float) pressure;
        if(last_channel_of_touch == 0) last_channel_of_touch = MAX_CHANNELS;
        position += (float) last_channel_of_touch - (float) channels;
        if (position >= MAX_CHANNELS) position -= MAX_CHANNELS;
        return {position, channels, pressure};
    }

};


#endif //MULTITOUCH_BAND_TOUCH_H

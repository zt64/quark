#include <cstdint>

namespace ac97 {
    constexpr uint8_t RESET = 0x00;
    constexpr uint8_t MASTER_VOLUME = 0x02;
    constexpr uint8_t AUX_OUT_VOLUME = 0x04;
    constexpr uint8_t SET_MIC_VOL = 0x0E;
    constexpr uint8_t SET_PCM_OUT_VOL = 0x18;
    constexpr uint8_t SELECT_INPUT_DEVICE = 0x1A;
    constexpr uint8_t SET_INPUT_GAIN = 0x1C;
    constexpr uint8_t SET_GAIN_OF_MICROPHONE = 0x1E;
    constexpr uint8_t EXTENDED_CAPABILITIES = 0x28;
    constexpr uint8_t CONTROL_OF_EXTENDED_CAPABILITIES = 0x2A;
    constexpr uint8_t SAMPLE_RATE_OF_PCM_FRONT_DAC = 0x2C;
    constexpr uint8_t SAMPLE_RATE_OF_PCM_SURREND_DAC = 0x2E;
    constexpr uint8_t SAMPLE_RATE_OF_PCM_LFE_DAC = 0x30;
    constexpr uint8_t SAMPLE_RATE_OF_PCM_LR_ADC = 0x32;

    void init() {

    }
}

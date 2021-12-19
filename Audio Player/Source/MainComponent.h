#pragma once

#include <JuceHeader.h>

using namespace juce;
//==============================================================================

class MainComponent : public juce::AudioAppComponent, public juce::TextButton::Listener, public juce::Slider::Listener,
    public juce::ChangeListener, public juce::Timer
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent() override;

    //==============================================================================
    void prepareToPlay(int samplesPerBlockExpected, double sampleRate) override;
    void getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill) ;
    void releaseResources() override;

    void paint(juce::Graphics& g) override;
    void resized() override;

    enum
    {
        fftOrder = 10,
        fftSize = 1 << fftOrder
    };

    Reverb reverbInstance;
    Reverb::Parameters reverbParameters;

private:
    //==============================================================================
    //JUCE自带的FFT函数
    dsp::FFT forwardFFT;
    // 用来展示频谱的图片，需要注意这个是图片而不是图片控件，需要使用JUCE Graph在屏幕绘制此图片
    Image spectrogramImage;

    //FFT参数
    float fifo[fftSize]; //FFT输入数据，每次为2^fftOrder个
    float fftData[2 * fftSize]; //FFT运算结果，实虚对称
    int fifoIndex = 0; //FFT输入数据索引
    bool nextFFTBlockReady = false; //是否进行下一次FFT的标志位，防止FFT运算过程中新的数据刷新

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(MainComponent) 

    double sampleRate = 0.0;
    bool ReverbOpen = false;

    juce::TextButton openButton;
    juce::TextButton playButton;
    juce::TextButton pauseButton;
    juce::TextButton stopButton;
    juce::Slider volumeSlider;
    juce::Label volumeLabel;
    juce::TextButton nameButton;
    juce::Label  nowTimeLabel;
    juce::Label  totalTimeLabel;
    juce::TextButton backwardButton;
    juce::TextButton forwardButton;
    juce::Label  Image1;
    juce::Label  Image2;
    juce::TextButton LadderButton;
    juce::Slider RoomSize;
    juce::Label  RomeSizeLabel;
    juce::ToggleButton ReverbButton;

    double nowTime;
    double totalTime;

    enum TransportState
    {
        Stopped,
        Starting,
        Playing,
        Pausing,
        Paused,
        Stopping
    };


    juce::AudioFormatManager formatManager;
    std::unique_ptr<juce::AudioFormatReaderSource> readerSource;
    juce::AudioTransportSource transportSource;
    TransportState state;



    juce::AudioSampleBuffer fileBuffer;

    int fileBufferPosition;

    juce::AudioThumbnailCache thumbnailCache;
    juce::AudioThumbnail thumbnail;


    void openButtonClicked();

    void changeState(TransportState newState);

    void playButtonClicked();

    void stopButtonClicked();

    void backwardButtonClicked();

    void forwardButtonClicked();

    void ReverbButtonClicked();

    void updateTime();

 
    virtual void timerCallback() override;

    virtual void sliderValueChanged(juce::Slider* slider) override;

    virtual void changeListenerCallback(ChangeBroadcaster* source) override;

    virtual void buttonClicked(Button*) override;

    void pushNextSampleIntoFifo(float sample);  //将音频数据写入FFT数据缓冲区，但是并不进行计算

    void drawNextLineOfSpectrogram();  // 根据FFT缓冲区的数据，计算FFT，并绘制频谱

    juce::String _numberFormat(int number, int minWidth)
    {
        juce::String result = juce::String(number);
        while (result.length() < minWidth)
        {
            result = "0" + result;
        }
        return result;
    }

    juce::String _timeFormat(double seconds)
    {

        return juce::String(std::floor(seconds / 60))
            + ":" + _numberFormat(int(std::floor(seconds)) % 60, 2)
            + "." + _numberFormat(int(std::floor(seconds * 1000)) % 1000, 3);
    }

 
};


#include "MainComponent.h"

using namespace juce;

//==============================================================================
MainComponent::MainComponent()
    :forwardFFT(fftOrder),
    spectrogramImage(Image::RGB, 512, 512, true),
    state(Stopped),
    thumbnailCache(5),
    thumbnail(
        128,
        formatManager,
        thumbnailCache
    )
{
    setSize(640, 780);
    formatManager.registerBasicFormats();
    setAudioChannels(0, 2);
    transportSource.addChangeListener(this);
    startTimerHz(60);
    setOpaque(true);

    addAndMakeVisible(&openButton);
    openButton.setButtonText("Open...");
    openButton.onClick = [this] { openButtonClicked(); };

    addAndMakeVisible(&playButton);
    playButton.setButtonText("Play");
    playButton.onClick = [this] { playButtonClicked(); };
    playButton.setColour(juce::TextButton::buttonColourId, juce::Colours::green);
    playButton.setEnabled(false);

    addAndMakeVisible(&stopButton);
    stopButton.setButtonText("Stop");
    stopButton.onClick = [this] { stopButtonClicked(); };
    stopButton.setColour(juce::TextButton::buttonColourId, juce::Colours::red);
    stopButton.setEnabled(false);

    addAndMakeVisible(&nowTimeLabel);
    nowTimeLabel.setText("--:--.---", dontSendNotification);
    nowTimeLabel.setJustificationType(Justification::left);

    addAndMakeVisible(&totalTimeLabel);
    totalTimeLabel.setText("--:--.---", dontSendNotification);
    totalTimeLabel.setJustificationType(Justification::right);

    addAndMakeVisible(&backwardButton);
    backwardButton.setButtonText("<<");
    backwardButton.onClick = [this] { backwardButtonClicked(); };

   
    addAndMakeVisible(&forwardButton);
    forwardButton.setButtonText(">>");
    forwardButton.onClick = [this] { forwardButtonClicked(); };

    addAndMakeVisible(&volumeLabel);
    volumeLabel.setText("Volume", juce::dontSendNotification);
    volumeLabel.attachToComponent(&volumeSlider, true);

    addAndMakeVisible(&volumeSlider);
    volumeSlider.setRange(-50, 30, 1);
    volumeSlider.setTextValueSuffix(" dB");
    volumeSlider.setValue(0);
    volumeSlider.addListener(this);

    addAndMakeVisible(&Image2);
    Image2.setText("FFT", juce::dontSendNotification);

    addAndMakeVisible(&Image1);
    Image1.setText("Audio", juce::dontSendNotification);


    addAndMakeVisible(&nameButton);
    nameButton.setButtonText("No File Loaded");
    nameButton.setEnabled(false);

    addAndMakeVisible(&ReverbButton);
   ReverbButton.setButtonText("Pass");
   ReverbButton.onClick = [this] { ReverbButtonClicked(); };


    addAndMakeVisible(&RoomSize);
    RomeSizeLabel.setText("REVERB", juce::dontSendNotification);
    RomeSizeLabel.attachToComponent(&RoomSize, true);

    RoomSize.setRange(0, 1, 0.01);
    RoomSize.setSliderStyle(Slider::SliderStyle::LinearHorizontal);
    RoomSize.setTextBoxStyle(Slider::TextBoxLeft, true, 100, 50);
    RoomSize.setValue(0);
    RoomSize.onValueChange = [this]
    {
        reverbParameters.roomSize = RoomSize.getValue();
        reverbInstance.setParameters(reverbParameters);
    };

    addAndMakeVisible(RoomSize);

    nowTime = 0.0f;
}

MainComponent::~MainComponent()
{
    shutdownAudio();
}

//==============================================================================
void MainComponent::prepareToPlay(int samplesPerBlockExpected, double sampleRate)
{
    reverbInstance.setSampleRate(sampleRate);
    transportSource.prepareToPlay(samplesPerBlockExpected, sampleRate);
}

void MainComponent::getNextAudioBlock(const juce::AudioSourceChannelInfo& bufferToFill)
{

    if (readerSource.get() == nullptr)
    {
        bufferToFill.clearActiveBufferRegion();
        return;
    }

    transportSource.getNextAudioBlock(bufferToFill);
        ScopedNoDenormals noDenormals;

    for (auto channel = 0; channel < bufferToFill.buffer->getNumChannels(); ++channel)
    {
        auto* inBuffer = bufferToFill.buffer->getReadPointer(channel, bufferToFill.startSample);
        auto* outBuffer = bufferToFill.buffer->getWritePointer(channel, bufferToFill.startSample);
        const auto* channelData = bufferToFill.buffer->getReadPointer(0, bufferToFill.startSample);

        for (auto sample = 0; sample < bufferToFill.numSamples; ++sample)
        {
            pushNextSampleIntoFifo(channelData[sample]);
            outBuffer[sample] = inBuffer[sample] * pow(10, ((float)volumeSlider.getValue() / 20.0));
           
         }
        if(ReverbOpen)
        { reverbInstance.processStereo(bufferToFill.buffer->getWritePointer(0),bufferToFill.buffer->getWritePointer(1), bufferToFill.buffer->getNumSamples());
    }
}
         

  


}

void MainComponent::releaseResources()
{
    transportSource.releaseResources();
}

//==============================================================================
void MainComponent::paint(juce::Graphics& g)
{
    g.setColour(juce::Colours::black);
    g.fillRect(0, 0, getWidth(), getHeight());
    if (totalTime == 0.0f) {
        totalTime = 1.0f;
    }
    juce::Rectangle<int> area(10, 70, getWidth() - 20, 10);
    g.setColour(juce::Colours::black);
    g.fillRect(area);
    g.setColour(juce::Colours::white);
    g.fillRect(10, 160, int((getWidth() - 20) * (nowTime / totalTime)), 10);

    g.setOpacity(1.0f); //不透明度
    juce::Rectangle<int> FFT(10, 520, getWidth() - 20, 100);
    g.drawImage(spectrogramImage, FFT.toFloat());

    juce::Rectangle<int> thumbnailBounds(10, 380, getWidth() - 20, 100);
    g.setColour(juce::Colours::white);
    g.fillRect(thumbnailBounds);
    g.setColour(juce::Colours::blue);
    thumbnail.drawChannels(g, thumbnailBounds, 0, thumbnail.getTotalLength(), 5);


}

void MainComponent::resized()
{
    nameButton.setBounds(10, 40, getWidth() - 20, 20);
    openButton.setBounds(10, 80, getWidth() - 20, 20);
    nowTimeLabel.setBounds(10, 130, getWidth() / 2 - 15, 20);
    totalTimeLabel.setBounds(getWidth() / 2 + 5, 130, getWidth() / 2 - 15, 20);
    playButton.setBounds(10, 180, getWidth() / 2 - 15, 20);
    stopButton.setBounds(getWidth() / 2 + 5, 180, getWidth() / 2 - 15, 20);
    backwardButton.setBounds(10, 210, getWidth() / 2 - 15 , 20);
    forwardButton.setBounds(getWidth() / 2 + 5 , 210, getWidth() / 2 - 15, 20);
    auto Left = 70;
    volumeSlider.setBounds(Left, 300, getWidth() - Left - 10, 20);
    Image1.setBounds(10, 360, 40, 20);
    Image2.setBounds(10, 500, 40, 20);
    RoomSize.setBounds(Left, 700 , getWidth() - Left - 10, 20);
    ReverbButton.setBounds(12, 730, 100, 20);
}



void MainComponent::openButtonClicked()
{
    juce::FileChooser chooser("Select a Wave file to play...", {}, "*.wav,*.aac,*.mp3");

    if (chooser.browseForFileToOpen())
    {
        auto file = chooser.getResult();
        auto* reader = formatManager.createReaderFor(file);

        if (reader != nullptr)
        {
            std::unique_ptr<juce::AudioFormatReaderSource> newSource(new juce::AudioFormatReaderSource(reader, true));
            transportSource.setSource(newSource.get(), 0, nullptr, reader->sampleRate);
            playButton.setEnabled(true);
            readerSource.reset(newSource.release());
        }
        nowTime = transportSource.getCurrentPosition();
        totalTime = transportSource.getLengthInSeconds();
        nameButton.setButtonText(file.getFileName());
        nowTimeLabel.setText(_timeFormat(nowTime), dontSendNotification);
        totalTimeLabel.setText(_timeFormat(totalTime), dontSendNotification);
        thumbnail.setSource(new juce::FileInputSource(file));
    }
}

void MainComponent::changeState(TransportState newState)
{
    if (state != newState)
    {
        state = newState;

        switch (state)
        {
        case Stopped:
            playButton.setButtonText("Play");
            stopButton.setButtonText("Stop");
            stopButton.setEnabled(false);
            transportSource.setPosition(0.0);
            break;

        case Starting:
            transportSource.start();
            break;

        case Playing:
            playButton.setButtonText("Pause");
            stopButton.setButtonText("Stop");
            stopButton.setEnabled(true);
            break;

        case Pausing:
            transportSource.stop();
            break;

        case Paused:
            playButton.setButtonText("Resume");
            stopButton.setButtonText("Return to Zero");
            break;

        case Stopping:
            transportSource.stop();
            break;
        }
    }
}

void MainComponent::sliderValueChanged(juce::Slider* slider){}


void MainComponent::changeListenerCallback(ChangeBroadcaster* source)
{
    if (source == &transportSource)
    {
        if (transportSource.isPlaying())
            changeState(Playing);
        else if ((state == Stopping) || (state == Playing))
            changeState(Stopped);
        else if (Pausing == state)
            changeState(Paused);
    }
}

void MainComponent::buttonClicked(Button*)
{
}

void MainComponent::playButtonClicked()
{
    if ((state == Stopped) || (state == Paused))
        changeState(Starting);
    else if (state == Playing)
        changeState(Pausing);
}

void  MainComponent::stopButtonClicked()
{
    if (state == Paused)
        changeState(Stopped);
    else
        changeState(Stopping);
}

void  MainComponent::backwardButtonClicked()
{
    double _nowTime = transportSource.getCurrentPosition();
    if (_nowTime <= 1.0f)
        transportSource.setPosition(0.0f);
    else
        transportSource.setPosition(_nowTime - 1.0f);
}

void  MainComponent::forwardButtonClicked()
{
    double _nowTime = transportSource.getCurrentPosition();
    if (_nowTime + 1.0f > totalTime)
        transportSource.setPosition(totalTime);
    else
        transportSource.setPosition(_nowTime + 1.0f);
}

void MainComponent::ReverbButtonClicked()
{
    if (ReverbOpen == false)
    {
        ReverbOpen = true;
    }
    else if (ReverbOpen == true)
    {
        ReverbOpen = false;
    }
}


void MainComponent::updateTime()
{
    nowTime = transportSource.getCurrentPosition();
    nowTimeLabel.setText(_timeFormat(nowTime), dontSendNotification);
    if (nextFFTBlockReady)
    {
        drawNextLineOfSpectrogram();
        nextFFTBlockReady = false;

    }
    repaint();
}


void  MainComponent::timerCallback()
{
    updateTime();
}

//-------------------------------------------------------------------------------

void MainComponent::pushNextSampleIntoFifo(float sample) 
{
    if (fifoIndex == fftSize)
    {
        if (!nextFFTBlockReady) // 防止运算过程中，错误的写入
        {
            zeromem(fftData, sizeof(fftData)); // 清空缓冲区
            memcpy(fftData, fifo, sizeof(fifo));// 写入缓冲区，此时只有实部（内存拷贝函数）
            nextFFTBlockReady = true;
        }

        fifoIndex = 0;
    }

    fifo[fifoIndex++] = sample;
}

// 根据FFT缓冲区的数据，计算FFT，并绘制频谱
void MainComponent::drawNextLineOfSpectrogram()
{
    auto rightHandEdge = spectrogramImage.getWidth() - 1;
    auto imageHeight = spectrogramImage.getHeight();

    //向左移动当前图片1个像素，每次计算的FFT结果使用1列像素表示
    spectrogramImage.moveImageSection(0, 0, 1, 0, rightHandEdge, imageHeight);

    //计算FFT，计算后的数据缓冲区中同时具有实部和虚部
    forwardFFT.performFrequencyOnlyForwardTransform(fftData);

    //获取最大振幅，绘制振幅时使用这个比例尺进行缩放
    auto maxLevel = FloatVectorOperations::findMinAndMax(fftData, fftSize / 2);

    for (auto y = 1; y < imageHeight; ++y)
    {
        // 计算当前频点在当前像素列中的像素行位置
        auto skewedProportionY = 1.0f - std::exp(std::log((float)y / (float)imageHeight) * 0.2f);
        // 获取当前频点的FFT结果
        auto fftDataIndex = jlimit(0, fftSize / 2, (int)(skewedProportionY * (int)fftSize / 2));
        // 频点振幅缩放
        auto level = jmap(fftData[fftDataIndex], 0.0f, jmax(maxLevel.getEnd(), 1e-5f), 0.0f, 1.0f);

        //绘制当前频点，使用颜色的冷暖表示振幅的强弱
        spectrogramImage.setPixelAt(rightHandEdge, y, Colour::fromHSV(level, 1.0f, level, 1.0
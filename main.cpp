#include <juce_audio_devices/juce_audio_devices.h>
#include <atomic>
#include <csignal>
#include <cstdio>
#include <iostream>
#include <memory>

static std::atomic<bool> gRunning { true };

static void signalHandler (int) { gRunning = false; }

//==============================================================================
static void listDevices (juce::AudioDeviceManager& manager)
{
    int devIndex = 0;

    for (auto* type : manager.getAvailableDeviceTypes())
    {
        type->scanForDevices();

        auto outputNames = type->getDeviceNames (false);
        auto inputNames  = type->getDeviceNames (true);

        // Merge into a single unique list: outputs first, then any input-only names
        juce::StringArray allNames = outputNames;
        for (auto& n : inputNames)
            if (! allNames.contains (n))
                allNames.add (n);

        if (allNames.isEmpty())
            continue;

        std::cout << "[" << type->getTypeName() << "]\n";

        for (auto& name : allNames)
        {
            std::unique_ptr<juce::AudioIODevice> dev (type->createDevice (name, name));

            int nIn  = dev ? dev->getInputChannelNames().size()  : 0;
            int nOut = dev ? dev->getOutputChannelNames().size() : 0;

            auto chanSpec = juce::String (nIn) + "i|" + juce::String (nOut) + "o";
            std::printf ("dev: %2d  %7s  %s\n",
                         devIndex++,
                         chanSpec.toRawUTF8(),
                         name.toRawUTF8());
        }

        std::cout << "\n";
    }
}

//==============================================================================
struct Loopback : public juce::AudioIODeviceCallback
{
    void audioDeviceAboutToStart (juce::AudioIODevice*) override {}
    void audioDeviceStopped()                           override {}

    void audioDeviceIOCallbackWithContext (
        const float* const* inputChannelData,  int numInputChannels,
        float* const*       outputChannelData, int numOutputChannels,
        int numSamples,
        const juce::AudioIODeviceCallbackContext&) override
    {
        int numChannels = std::min (numInputChannels, numOutputChannels);

        for (int ch = 0; ch < numChannels; ++ch)
            if (inputChannelData[ch] && outputChannelData[ch])
                std::memcpy (outputChannelData[ch], inputChannelData[ch],
                             (size_t) numSamples * sizeof (float));

        // Zero any output channels that have no corresponding input
        for (int ch = numChannels; ch < numOutputChannels; ++ch)
            if (outputChannelData[ch])
                std::memset (outputChannelData[ch], 0,
                             (size_t) numSamples * sizeof (float));
    }
};

//==============================================================================
// Opens the named device on the Dante backend directly, via the raw AudioIODeviceType /
// AudioIODevice interfaces. Deliberately bypasses juce::AudioDeviceManager here: its
// getAvailableDeviceTypes()/initialise() scan *every* registered backend (ALSA included),
// which probes ALSA PCMs such as the depasound-backed ones. Since a named Dante endpoint
// was requested, only the Dante backend should ever be touched.
static std::unique_ptr<juce::AudioIODevice> startLoopback (const juce::String& deviceName,
                                                           juce::AudioIODeviceCallback& callback)
{
    std::unique_ptr<juce::AudioIODeviceType> danteType (juce::AudioIODeviceType::createAudioIODeviceType_Dante());
    if (danteType == nullptr)
    {
        std::cerr << "Dante backend not available in this build\n";
        return nullptr;
    }

    danteType->scanForDevices();

    if (! danteType->getDeviceNames (false).contains (deviceName))
    {
        std::cerr << "Device not found: " << deviceName << "\n";
        return nullptr;
    }

    std::unique_ptr<juce::AudioIODevice> device (danteType->createDevice (deviceName, deviceName));
    if (device == nullptr)
    {
        std::cerr << "Error creating device: " << deviceName << "\n";
        return nullptr;
    }

    juce::BigInteger channels;
    channels.setRange (0, 256, true);

    auto error = device->open (channels, channels, 0.0, 0);
    if (error.isNotEmpty())
    {
        std::cerr << "Error opening device: " << error << "\n";
        return nullptr;
    }

    device->start (&callback);
    return device;
}

//==============================================================================
int main (int argc, char* argv[])
{
    juce::MessageManager::getInstance();

    // Parse arguments
    juce::String loopbackDevice;
    unsigned txLatencyUs = 1000;   // default matches kDefaultTxLatencyUs in juce_Dante.cpp

    for (int i = 1; i < argc; ++i)
    {
        auto arg = juce::String (argv[i]);
        if (arg == "-l" && i + 1 < argc)
            loopbackDevice = argv[++i];
        else if (arg == "-t" && i + 1 < argc)
            txLatencyUs = (unsigned) std::atoi (argv[++i]);
        else
        {
            juce::String bin (argv[0]);
            auto slash = bin.lastIndexOfChar ('/');
            if (slash >= 0) bin = bin.substring (slash + 1);
            std::cerr << "Usage:\n"
                      << "  " << bin << "              List available audio devices\n"
                      << "  " << bin << " -l <name>    Run loopback on named device\n"
                      << "\n"
                      << "Options:\n"
                      << "  -l <name>   Device name to use for loopback\n"
                      << "  -t <us>     Dante TX latency in microseconds (default: 1000)\n";
            juce::MessageManager::deleteInstance();
            return 1;
        }
    }

    if (loopbackDevice.isEmpty())
    {
        // No named device requested: fine to enumerate every backend (ALSA included).
        juce::AudioDeviceManager manager;
        listDevices (manager);
    }
    else
    {
        // A specific Dante endpoint was requested: only ever touch the Dante backend.
        juce::setDanteTxLatencyUs (txLatencyUs);

        Loopback loopback;
        auto device = startLoopback (loopbackDevice, loopback);
        if (device == nullptr)
        {
            juce::MessageManager::deleteInstance();
            return 1;
        }

        std::signal (SIGINT,  signalHandler);
        std::signal (SIGTERM, signalHandler);

        std::cout << "Loopback running on: " << device->getName() << "\n"
                  << "  Sample rate : " << device->getCurrentSampleRate() << " Hz\n"
                  << "  Buffer size : " << device->getCurrentBufferSizeSamples() << " samples\n"
                  << "  Inputs      : " << device->getActiveInputChannels().countNumberOfSetBits() << "\n"
                  << "  Outputs     : " << device->getActiveOutputChannels().countNumberOfSetBits() << "\n"
                  << "  TX latency  : " << txLatencyUs << " us\n";

        while (gRunning)
            juce::Thread::sleep (100);

        device->close();
    }

    juce::MessageManager::deleteInstance();
    return 0;
}

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
static bool startLoopback (juce::AudioDeviceManager& manager, const juce::String& deviceName)
{
    // Find which backend owns this device name
    for (auto* type : manager.getAvailableDeviceTypes())
    {
        type->scanForDevices();

        for (auto& name : type->getDeviceNames (false))
        {
            if (name == deviceName)
            {
                manager.setCurrentAudioDeviceType (type->getTypeName(), true);

                juce::AudioDeviceManager::AudioDeviceSetup setup;
                manager.getAudioDeviceSetup (setup);
                setup.outputDeviceName = deviceName;
                setup.inputDeviceName  = deviceName;
                setup.useDefaultInputChannels  = false;
                setup.useDefaultOutputChannels = false;
                setup.inputChannels .setRange (0, 256, true);
                setup.outputChannels.setRange (0, 256, true);
                setup.sampleRate = 0;   // let the device choose
                setup.bufferSize = 0;

                auto error = manager.setAudioDeviceSetup (setup, true);
                if (error.isNotEmpty())
                {
                    std::cerr << "Error opening device: " << error << "\n";
                    return false;
                }

                return true;
            }
        }
    }

    std::cerr << "Device not found: " << deviceName << "\n";
    return false;
}

//==============================================================================
int main (int argc, char* argv[])
{
    juce::MessageManager::getInstance();

    juce::AudioDeviceManager manager;
    manager.initialise (2, 2, nullptr, false);

    // Parse arguments
    juce::String loopbackDevice;
    int txLatencyUs = -1;   // -1 = use backend default

    for (int i = 1; i < argc; ++i)
    {
        auto arg = juce::String (argv[i]);
        if (arg == "-l" && i + 1 < argc)
            loopbackDevice = argv[++i];
        else if (arg == "-t" && i + 1 < argc)
            txLatencyUs = std::atoi (argv[++i]);
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
        listDevices (manager);
    }
    else
    {
        if (txLatencyUs >= 0)
            juce::setDanteTxLatencyUs ((unsigned) txLatencyUs);

        Loopback loopback;
        manager.addAudioCallback (&loopback);

        if (!startLoopback (manager, loopbackDevice))
        {
            juce::MessageManager::deleteInstance();
            return 1;
        }

        auto* device = manager.getCurrentAudioDevice();
        std::signal (SIGINT,  signalHandler);
        std::signal (SIGTERM, signalHandler);

        std::cout << "Loopback running on: " << device->getName() << "\n"
                  << "  Sample rate : " << device->getCurrentSampleRate() << " Hz\n"
                  << "  Buffer size : " << device->getCurrentBufferSizeSamples() << " samples\n"
                  << "  Inputs      : " << device->getActiveInputChannels().countNumberOfSetBits() << "\n"
                  << "  Outputs     : " << device->getActiveOutputChannels().countNumberOfSetBits() << "\n";

        while (gRunning)
            juce::Thread::sleep (100);

        manager.removeAudioCallback (&loopback);
        manager.closeAudioDevice();
    }

    juce::MessageManager::deleteInstance();
    return 0;
}

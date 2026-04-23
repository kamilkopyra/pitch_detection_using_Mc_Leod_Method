Skip to content
kamilkopyra
pitch_detection_using_Mc_Leod_Method
Repository navigation
Code
Issues
Pull requests
Agents
Actions
Projects
Wiki
Security and quality
Insights
Settings
Comparing changes
Choose two branches to see what’s changed or to start a new pull request. If you need to, you can also  or learn more about diff comparisons.
...
There isn’t anything to compare.
main and master are entirely different commit histories.

 Showing  with 0 additions and 364 deletions.
 364 changes: 0 additions & 364 deletions364  
caly/Tuner_Main.cpp
Original file line number	Diff line number	Diff line change
@@ -1,364 +0,0 @@
#include <stdio.h>
#include <string> 
#include <vector>
#include "portaudio.h"
#include <iostream>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <stdlib.h>


#define pi 3.14159265358979323846
using namespace std;

const int windowSize = 4096;
const int sampleRate = 44100;
float previousFreq = 0;
float secondFreq = 0;
float df = (float)sampleRate / (float)windowSize;


struct GuitarString {
    string name;
    double frequency;
};

struct Peak {
    int index =0;
    float value=0;
};


float getClosestTarget(float maxFreq);
void McLeod_Method(vector <float> fftBuffer, float sampleRate);
void printTuner(float freq, float target, float cents, const string& stringName);
string getClosestName(float maxFreq);

inline float median3(float a, float b, float c)
{
    return max(min(a, b), min(max(a, b), c));
}



static vector<float> fftBuffer;

static int audioCallback(const void* inputBuffer, void* outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void* userData)
{
    const float* in = static_cast<const float*>(inputBuffer);

    // Zbieranie próbek do bufora
    for (unsigned i = 0; i < framesPerBuffer; ++i) {
        fftBuffer.push_back(*in++);
    }

    if (fftBuffer.size() >= windowSize) {

        McLeod_Method(fftBuffer, sampleRate);

        fftBuffer.erase(fftBuffer.begin(), fftBuffer.end() - windowSize * 0.8);
    }

    return paContinue;  
}



// Funkcja do znajdowania najbliższej zdefiniwanej częstotliwości
float getClosestTarget(float maxFreq)
{
    vector<GuitarString> guitarStrings = {
        {"E4", 329.63},
        {"B3", 246.94},
        {"G3", 196.00},
        {"D3", 146.83},
        {"A2", 110.00},
        {"E2", 82.41}
    };

    //normalize to avaliable freq value

    float closestFreq = guitarStrings[0].frequency;
    string closestString = guitarStrings[0].name;
    float minDiff = abs(maxFreq - closestFreq);

    for (const auto& gs : guitarStrings) {
        float diff = abs(maxFreq - gs.frequency);
        if (diff < minDiff) {
            minDiff = diff;
            closestFreq = gs.frequency;
            closestString = gs.name;
        }
    }

    //printf("Closest string: %s with frequency %f Hz\n", closestString.c_str(), closestFreq);
    //printf("Difference: %f\n\n", minDiff);
    return closestFreq;
}





struct Candidate {
    int   tau;      // opóźnienie w próbkach (indeks w nsdf)
    float freq;     // częstotliwość w Hz
    float value;    // nsdf[tau]
};

void McLeod_Method(vector <float> fftBuffer, float sampleRate)
{
    int N = fftBuffer.size();
    vector<float> nsdf(N, 0.0f);
    float meanFreq;
    float detectedString;
    float centsDifference;

    // 0. Sprawdzenie czy sygnał jest wystarczająco silny
    double energy = 0.0;
    for (int i = 0; i < N; i++) {
        energy += fftBuffer[i] * fftBuffer[i];
    }
    energy = sqrt(energy / N);  // RMS

    if (energy < 0.00005) {        // próg zależny od mikrofonu
        meanFreq = 0.0;
        return;
        //printf("McLeod Method : %.1f\n", meanFreq);

    }

    else {

        // 1. Obliczenie NSDF
        for (int tau = 0; tau < N; tau++) {
            double numerator = 0.0;
            double denominator = 0.0;
            for (int i = 0; i < N - tau; i++) {
                numerator += fftBuffer[i] * fftBuffer[i + tau];
                denominator += fftBuffer[i] * fftBuffer[i] + fftBuffer[i + tau] * fftBuffer[i + tau];
            }
            nsdf[tau] = (denominator > 0) ? (2.0 * numerator / denominator) : 0;
        }

        // 2. Znalezienie głównego maksimum (po tau > 0)
        int peakIndex = 1;
        float peakValue = -1.0f;
        float maxScore = 0.0f;

        int minTau = sampleRate / 1200;
        int maxTau = sampleRate/25;
        vector<Candidate> candidates;

        for (int tau = 2; tau < maxTau; tau++) {
            if (nsdf[tau] > 0.8 &&        // musi być wyraźnie tonalny
                nsdf[tau] > nsdf[tau - 1] &&
                nsdf[tau] > nsdf[tau + 1]) 
            {

                float freq = sampleRate / tau;
                candidates.push_back({ tau, freq, nsdf[tau] });
                if (nsdf[tau] > peakValue) {
                    peakIndex = tau;
                    peakValue = nsdf[tau];
                    break;
                }
            }
        }
        int size = candidates.size();

        float LN = nsdf[peakIndex - 1];
        float RN = nsdf[peakIndex + 1];
        float offset = 0.5f * (LN - RN) / (LN - 2 * peakValue + RN);
        float trueIndex = peakIndex + offset;

        // 4. Wyliczenie częstotliwości
        float interpolatedFreq = sampleRate / trueIndex;

        float rawFreq = interpolatedFreq;

        meanFreq = median3(rawFreq, previousFreq, secondFreq);

        secondFreq = previousFreq;
        previousFreq = rawFreq;
        if (meanFreq > 1000) 
        {
            meanFreq = 0;
        }
        else {

            detectedString = getClosestTarget(meanFreq);
            centsDifference = 0.0f;
            if (meanFreq > 0.0f) {
                centsDifference = 1200.0f * log(meanFreq / detectedString) / log(2.0f);
            }
        }
        float detectedString = getClosestTarget(meanFreq);
        string detectedName = getClosestName(meanFreq);
        float centsDifference = (meanFreq > 0.0f)
            ? 1200.0f * log2f(meanFreq / detectedString)
            : 0.0f;
        printTuner(meanFreq, detectedString, centsDifference, detectedName);

    }

}

void printTuner(float freq, float target, float cents, const string& stringName)
{
    if (freq < 10.0f) return;  // nie rysuj dla 0

    system("cls");

    const int barWidth = 41;
    const float maxCents = 50.0f;
    int pos = (int)((cents / maxCents) * (barWidth / 2)) + barWidth / 2;
    pos = max(0, min(barWidth - 1, pos));

    string bar(barWidth, '-');
    bar[barWidth / 2] = '|';
    bar[pos] = '*';

    printf("+------------------------------------------+\n");
    printf("|           GUITAR TUNER                   |\n");
    printf("+------------------------------------------+\n");
    printf("|  Struna: %-6s     Freq: %6.1f Hz       |\n", stringName.c_str(), freq);
    printf("|  Target: %-6s     Freq: %6.2f Hz       |\n", stringName.c_str(), target);
    printf("|                                          |\n");
    printf("|  [%s]  |\n", bar.c_str());
    printf("|   -50        -10    0    +10        +50  |\n");
    printf("|                                          |\n");

    if (abs(cents) < 5.0f)
        printf("|          *** NASTROJONO! ***             |\n");
    else if (cents > 0)
        printf("|     %+6.1f centow  ->  poluzuj strune      |\n", cents);
    else
        printf("|     %+6.1f centow  ->  naciagnij strune  |\n", cents);

    printf("+------------------------------------------+\n");
}

string getClosestName(float maxFreq)
{
    vector<GuitarString> guitarStrings = {
        {"E4", 329.63}, {"B3", 246.94}, {"G3", 196.00},
        {"D3", 146.83}, {"A2", 110.00}, {"E2", 82.41}
    };

    string closestName = guitarStrings[0].name;
    float minDiff = abs(maxFreq - (float)guitarStrings[0].frequency);

    for (const auto& gs : guitarStrings) {
        float diff = abs(maxFreq - (float)gs.frequency);
        if (diff < minDiff) {
            minDiff = diff;
            closestName = gs.name;
        }
    }
    return closestName;
}

















int main() {

    PaError err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "Init error: " << Pa_GetErrorText(err) << std::endl;
        return 1;
    }

    int numDev = Pa_GetDeviceCount();
    std::cout << "=== PortAudio devices ===\n";
    for (int i = 0; i < numDev; ++i)
        std::cout << i << ": " << Pa_GetDeviceInfo(i)->name << "\n";
    std::cout << "=========================\n";

    PaStreamParameters inParams{}, outParams{};
    int inDev = Pa_GetDefaultInputDevice();
    int outDev = Pa_GetDefaultOutputDevice();
    for (int i = 0; i < numDev; ++i) {
        std::string n(Pa_GetDeviceInfo(i)->name);
        if (n.find("USB Audio") != std::string::npos)  inDev = i;
        if (n.find("Headphones") != std::string::npos) outDev = i;
    }
    std::cout << "Wejście = " << inDev << ", Wyjście = " << outDev << "\n";

    inParams.device = inDev;
    inParams.channelCount = 1;
    inParams.sampleFormat = paFloat32;
    inParams.suggestedLatency = Pa_GetDeviceInfo(inDev)->defaultLowInputLatency;
    inParams.hostApiSpecificStreamInfo = nullptr;

    outParams.device = outDev;
    outParams.channelCount = 2;
    outParams.sampleFormat = paFloat32;
    outParams.suggestedLatency = Pa_GetDeviceInfo(outDev)->defaultLowOutputLatency;
    outParams.hostApiSpecificStreamInfo = nullptr;

    PaStream* stream;
    err = Pa_OpenStream(&stream, &inParams, nullptr,
        sampleRate, 512,
        paClipOff, audioCallback, nullptr);
    if (err != paNoError) {
        std::cerr << "OpenStream error: " << Pa_GetErrorText(err) << std::endl;
        return 1;
    }

    err = Pa_StartStream(stream);
    if (err != paNoError) {
        std::cerr << "StartStream error: " << Pa_GetErrorText(err) << std::endl;
        return 1;
    }

    std::cout << "Strumień uruchomiony, Ctrl+C aby zakończyć\n";


    while (Pa_IsStreamActive(stream) == 1) {
        Pa_Sleep(100);
    }

    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();
    return 0;

}




















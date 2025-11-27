#include <stdio.h>
#include <string> 
#include <vector>
#include "portaudio.h"
#include <iostream>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <stdlib.h>
//#include "matplotlibcpp.h" 

#define pi 3.14159265358979323846
using namespace std;

const int windowSize = 4096;
const int sampleRate = 44100;
float meanFreq = 0;
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

        meanFreq = (previousFreq + 2 * interpolatedFreq + secondFreq) / 4;
        secondFreq = previousFreq;
        previousFreq = interpolatedFreq;
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
        printf("McLeod Method : %.1f\n", meanFreq);
        

    }

}

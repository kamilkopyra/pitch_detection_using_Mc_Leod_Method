# pitch_detection_using_Mc_Leod_Method
In this repository I will share code for detecting pitch using a method developed by Mr. Phil McLeod from the University of Otago.  
The purpose of this project was to build a funcional guitar tuner.
The tuner was found to work best when connected via wire to the guitar.
The code can be easily modified into tuning different music instruments (ex. bass, violin)


The McLeod Pitch Method (MPM) is a time-domain pitch detection algorithm developed by Philip McLeod at the University of Otago. It is based on the Normalized Squared Difference Function (NSDF), which allows the algorithm to efficiently determine the fundamental frequency of a periodic audio signal, such as a note played on a guitar string. Instead of analyzing audio in the frequency domain using Fourier Transform, the McLeod method operates directly on the waveform, making it significantly faster and better suited for real-time applications.

What makes this method particularly effective is its ability to accurately detect the true fundamental frequency, even when strong harmonic overtones are present. This is especially important for musical instruments like guitar or violin, where higher harmonics can easily mislead traditional FFT-based pitch detectors. By normalizing the difference function and performing peak interpolation, MPM improves both the precision and stability of pitch estimation


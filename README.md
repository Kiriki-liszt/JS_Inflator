# JS_Inflator_to_VST2_VST3

JS Inflator, the copy of Sonox Inflator, in vstsdk

<img src="https://github.com/Kiriki-liszt/JS_Inflator_to_VST2_VST3/blob/478b505b6dacf0eacc5e71adc3a60bd1212e7428/screenshot_orig.png"  width="200"/>
<img src="https://github.com/Kiriki-liszt/JS_Inflator_to_VST2_VST3/blob/478b505b6dacf0eacc5e71adc3a60bd1212e7428/screenshot_twarch.png"  width="400"/>

Now comes in two GUIs.
The new GUI is made by Twarch.

Windows and Mac(Intel only). Linux not compatible.  
This is my first attempt with VSTSDK & VSTGUI.  
Some codes might be inefficent...  

## Licensing  

> Q: I would like to share the source code of my VST 3 plug-in/host on GitHub or other such platform.  
> * You can choose the GPLv3 license and feel free to share your plug-ins/host's source code including or referencing the VST 3 SDK's sources on GitHub.  
> * **You are allowed to provide a binary form of your plug-ins/host too, provided that you provide its source code as GPLv3 too.** 
> * Note that you have to follow the Steinberg VST usage guidelines.  
> 
> <https://steinbergmedia.github.io/vst3_dev_portal/pages/FAQ/Licensing.html>  
 
<img src="https://github.com/Kiriki-liszt/JS_Inflator_to_VST2_VST3/raw/main/VST_Compatible_Logo_Steinberg_with_TM.png"  width="200"/>

VSTSDK 3.7.7 used  
VSTGUI 4.12 used  

Built as VST3, but compatible to VST2, too.  

## Version logs

v1.0.0: intial try.  
v1.1.0: VuPPM meter change(mono -> stereo, continuous to discrete), but not complete!  
v1.2.0: VuPPM meter corrected!  
v1.2.1: Channel configuration corrected. probably a bug fix for crashing sometimes.  
v1.3.0: Curve knob fixed!!! and 32FP dither by airwindows.  
v1.4.0: Oversampling up to x8 now works! DPC works.  
v1.5.0: Band Split added.  
v1.5.1: macOS build added. Intel x86 tested, Apple silicon not tested(I dont have one).  
v1.6.0rc: FX meter(Effect Meter) is added. Original GUI is now on high definition.

Now up to date with latest RC Inflator form ReaTeam/JSFX repository.  

## What I've learned

* Volumefader  

For someone like me, wondering how to use volume fader;  
Use a RangeParameter!  
param as normalized parameter[0.0, 1.0],  
dB as Plain value,  
gain as multiplier of each samples.  
For normParam to gain, check ~process.cpp  

ex)  
| param 	| dB  	| gain 	|
|-------	|-----	|------	|
| 0.0   	| -12 	| 0.25 	|
| 0.5   	| 0   	| 1    	|
| 1.0   	| +12 	| 4    	|  

| param 	| dB  	| gain 	|
|-------	|-----	|------	|
| 0.0   	| -12 	| 0.25 	|
| 0.5   	| -6  	| 0.5  	|
| 1.0   	| 0   	| 1    	|  

* Resampling  

Generally, the process goes as:  

1. doubling samples  
2. LP Filtering  
3. ProcessAudio  
4. LP Filtering  
5. reducing samples  

Filter choice is most critical, I think.  
HIIR code used Polyphase Filter, which is minimun phase filter.  
Has some letency, has some phase issue, but both very low.  

<img src="https://github.com/Kiriki-liszt/JS_Inflator_to_VST2_VST3/raw/main/resource/8x_freq.png"  width="400"/>
<img src="https://github.com/Kiriki-liszt/JS_Inflator_to_VST2_VST3/raw/main/resource/8x_phase_RC.png"  width="400"/>
<img src="https://github.com/Kiriki-liszt/JS_Inflator_to_VST2_VST3/raw/main/resource/8x_phase_JS.png"  width="400"/>

Compared to 31-tap filter, polyphase has more flat frequency and phase response.  

<img src="https://github.com/Kiriki-liszt/JS_Inflator_to_VST2_VST3/raw/main/resource/8x_H_RC.png"  width="400"/>
<img src="https://github.com/Kiriki-liszt/JS_Inflator_to_VST2_VST3/raw/main/resource/8x_H_JS.png"  width="400"/>  

Meaning, HIIR is better at anti-alising.  

* Latency Reporting  

Naive implementaiton could use 'sendTextMessage' and 'receiveText' pair.  

## references

1. RC Inflator  
<https://forum.cockos.com/showthread.php?t=256286>  
<https://github.com/ReaTeam/JSFX/tree/master/Distortion>  

2. HIIR resampling codes by 'Laurent De Soras'.  
<http://ldesoras.free.fr/index.html>  

## Todo

- [ ] Preset save & import menu bar.  
- [ ] malloc/realloc/free to new/delete change.  
- [ ] Oversampling options : Linear phase, Balanced.  
- [ ] GUI : resizing, and HDDPI.  
- [ ] GUI : double click to enter value.  

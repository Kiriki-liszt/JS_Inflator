# JS_Inflator_to_VST2_VST3

JS Inflator is a copy of Sonox Inflator.  
Runs in double precision 64-bit internal processing.  
Also double precision input / output if supported.  

<img src="https://github.com/Kiriki-liszt/JS_Inflator_to_VST2_VST3/raw/main/screenshot_orig.png"  width="200"/>  
<img src="https://github.com/Kiriki-liszt/JS_Inflator_to_VST2_VST3/raw/main/screenshot_twarch.png"  width="400"/>

Comes in two GUIs.
The alternative GUI is made by Twarch.

## Builds and Requirements  

Windows - x64, SSE2  
Mac - macos 10.13 ~ 14.2, SSE2 or NEON  

## Project Compability  

Windows, Mac and Linux.  

## How to use  

1. Windows

Unzip Win.zip from latest release and copy to "C:\Program Files\Common Files\VST3".  

2. MacOS(Intel tested, Apple Silicon not tested).  

Unzip MacOS.zip from latest release and copy vst3 to "/Library/Audio/Plug-Ins/VST3" and component to "/Library/Audio/Plug-Ins/Components".  

> If it doesn't go well, configure security options in console as  
>  
> ``` console  
> sudo xattr -r -d com.apple.quarantine /Library/Audio/Plug-Ins/VST3/InflatorPackage.vst3  
> sudo xattr -r -d com.apple.quarantine /Library/Audio/Plug-Ins/Components/InflatorPackage.component
>
> sudo codesign --force --sign - /Library/Audio/Plug-Ins/VST3/InflatorPackage.vst3  
> sudo codesign --force --sign - /Library/Audio/Plug-Ins/Components/InflatorPackage.component  
> ```  
>  
> tested by @jonasborneland [here](https://github.com/Kiriki-liszt/JS_Inflator_to_VST2_VST3/issues/12#issuecomment-1616671177)

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

VSTSDK 3.7.9 used  
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
v1.5.1: macOS build added. Intel x86 & Apple silicon tested.  
v1.6.0rc: FX meter(Effect Meter) is added. Original GUI is now on high definition.  
v1.6.0rc1: Linear phase Oversampling is now added.  
v1.6.0: Linear knob mode is now specified, and GUI size flinching fixed.  
v1.7.0.beta + beta 2

1. AUv2 build added. VSTSDK update to 3.7.9. Xcode 15.2, OSX 14.2 build.  
2. Changed oversampling method from whole buff resample to each sample resample, for less crashes.  
3. hiir min-phase resampler is no longer used, and implememted my own FIR resampler with SSE2 optimization.
4. Fir filter is now hardwired.
5. Bypass latency compensated.

v1.7.0: Fir using Kaiser-Bessel window, label change from 'Lin' to 'Max'.  

Now up to date with latest RC Inflator from ReaTeam/JSFX repository.  

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

* Linear Phase  
HIIR resampling is Min-phase resampler, meaning phase distortion at high freqs.  
For more natural high frequency hearing I Implemented r8brain-free-src designed by Aleksey Vaneev of Voxengo.  
Had some time with it trying understand how FIR works. I still don't know yet but it works.  
About weird choices for x4 and x8 - these resamplers have asynchronous latencies so downsampling starts little before upsampling starts.  
To fix it, I just changed Transition band for x4 and 24-bit for x8.  
Now it is free of Phase issuses. 

* Comparisons  
1x  
<img src="https://github.com/Kiriki-liszt/JS_Inflator_to_VST2_VST3/blob/380f6b668782e78d0526607e5c9d3586f8af04d0/screenshots/OS_1x.png"  width="400"/>  

2x Min-phase  
<img src="https://github.com/Kiriki-liszt/JS_Inflator_to_VST2_VST3/blob/380f6b668782e78d0526607e5c9d3586f8af04d0/screenshots/OS_2x_Min.png"  width="400"/>  

2x Lin-phase  
<img src="https://github.com/Kiriki-liszt/JS_Inflator_to_VST2_VST3/blob/380f6b668782e78d0526607e5c9d3586f8af04d0/screenshots/OS_2x_Lin.png"  width="400"/>  

4x Min-phase  
<img src="https://github.com/Kiriki-liszt/JS_Inflator_to_VST2_VST3/blob/380f6b668782e78d0526607e5c9d3586f8af04d0/screenshots/OS_4x_Min.png"  width="400"/>  

4x Lin-phase  
<img src="https://github.com/Kiriki-liszt/JS_Inflator_to_VST2_VST3/blob/380f6b668782e78d0526607e5c9d3586f8af04d0/screenshots/OS_4x_Lin.png"  width="400"/>  

8x Min-phase  
<img src="https://github.com/Kiriki-liszt/JS_Inflator_to_VST2_VST3/blob/380f6b668782e78d0526607e5c9d3586f8af04d0/screenshots/OS_8x_Min.png"  width="400"/>  

8x Lin-phase  
<img src="https://github.com/Kiriki-liszt/JS_Inflator_to_VST2_VST3/blob/380f6b668782e78d0526607e5c9d3586f8af04d0/screenshots/OS_8x_Lin.png"  width="400"/>  
  
* Latency Reporting  

Naive implementaiton could use 'sendTextMessage' and 'receiveText' pair.  

* Knob Modes

Specifying knob modes at 'createView' in 'controller.cpp' makes knobs work in selected mode.  
``` c++
setKnobMode(Steinberg::Vst::KnobModes::kLinearMode);
```
[https://github.com/Kiriki-liszt/JS_Inflator_to_VST2_VST3/blob/v1.6.0/source/InflatorPackagecontroller.cpp#L339](https://github.com/Kiriki-liszt/JS_Inflator_to_VST2_VST3/blob/v1.6.0/source/InflatorPackagecontroller.cpp#L339)  


## references

1. RC Inflator  
<https://forum.cockos.com/showthread.php?t=256286>  
<https://github.com/ReaTeam/JSFX/tree/master/Distortion>  

2. HIIR resampling codes by 'Laurent De Soras'.  
<http://ldesoras.free.fr/index.html>  

3. r8brain-free-src - Sample rate converter designed by Aleksey Vaneev of Voxengo  
<https://github.com/avaneev/r8brain-free-src>  
Modified for my need: Fractional resampling, Interpolation parts are deleted.  

## Todo

- [ ] Preset save & import menu bar.  
- [x] malloc/realloc/free to new/delete change. -> stick with it. 
- [x] Oversampling options : Linear phase, Balanced.  
- [x] GUI : resizing, and HDDPI.  
- [ ] GUI : double click to enter value.
- [x] soft bypass latency.  

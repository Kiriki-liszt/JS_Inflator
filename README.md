# JS Inflator

JS Inflator is a copy of Sonox Inflator.  
Runs in double precision 64-bit internal processing.  
Also double precision input / output if supported.  

[![GitHub Release](https://img.shields.io/github/v/release/kiriki-liszt/JS_Inflator?style=flat-square&label=Get%20latest%20Release)](https://github.com/Kiriki-liszt/JS_Inflator/releases/latest)
[![GitHub Downloads (all assets, all releases)](https://img.shields.io/github/downloads/kiriki-liszt/JS_Inflator/total?style=flat-square&label=total%20downloads&color=blue)](https://github.com/Kiriki-liszt/JS_Inflator/releases/latest)  

[![Static Badge](https://img.shields.io/badge/coffee%20maybe%3F%20%3D%5D%20-gray?style=for-the-badge&logo=buy-me-a-coffee)](https://buymeacoffee.com/kirikiaris)

<img src="https://github.com/Kiriki-liszt/JS_Inflator_to_VST2_VST3/raw/main/screenshots/screenshot_both.png"  width="600"/>  

Comes in two GUIs.
The alternative GUI is made by Twarch.  

## Builds and Requirements  

Windows - x64  
Mac - macos 10.13 ~ 14.5, Intel and Apple Silicon Native  

## Project Compability  

Windows, Mac and Linux(Change VSTSDK location in cmake and re-build any IDE project you prefer).  

## How to use  

* Windows  

Unzip Windows version from latest Release and copy to "C:\Program Files\Common Files\VST3".  

* MacOS  

Unzip macOS version from latest Release and copy vst3 to "/Library/Audio/Plug-Ins/VST3" and component to "/Library/Audio/Plug-Ins/Components".  

> If it doesn't go well, configure security options in console as  
>
> ``` console  
> sudo xattr -r -d com.apple.quarantine /Library/Audio/Plug-Ins/VST3/JS_Inflator.vst3  
> sudo xattr -r -d com.apple.quarantine /Library/Audio/Plug-Ins/Components/JS_Inflator.component
>
> sudo codesign --force --sign - /Library/Audio/Plug-Ins/VST3/JS_Inflator.vst3  
> sudo codesign --force --sign - /Library/Audio/Plug-Ins/Components/JS_Inflator.component  
> ```  
>
> tested by @jonasborneland [here](https://github.com/Kiriki-liszt/JS_Inflator_to_VST2_VST3/issues/12#issuecomment-1616671177)

* From v1.x.x to v2.x.x  

Just delete old one and use new one!  
Any DAW with steinberg standard will automatically replace old plugins while opening project.  
Settings are also transfered automatically.  

Also, after plugin version change, do not open projects directly.  
Please open a blank project first, and let DAW scan plugins and change from v1 to v2,
and then we can safely open previous projects.  

Tested as working are;

1. Cubase 12(Windows), plugin v1.7.0 -> v2.0.0  
2. Cubase 12(macOS), plugin v1.7.0 -> v2.0.0  
3. Cubase 12(Windows), plugin v1.7.0 -> Cubase 12(macOS), plugin v2.0.0  
4. Logic 10, plugin v1.7.0 -> v2.0.0
5. Ableton 12(Windows), plugin v1.7.0 -> v2.0.0
6. Ableton 12(macOS), plugin v1.7.0 -> v2.0.0
7. Ableton 12(Windows), plugin v1.7.0 -> Ableton 12(macOS), plugin v2.0.0  

## Licensing  

> Q: I would like to share the source code of my VST 3 plug-in/host on GitHub or other such platform.  
>
> * You can choose the GPLv3 license and feel free to share your plug-ins/host's source code including or referencing the VST 3 SDK's sources on GitHub.  
> * **You are allowed to provide a binary form of your plug-ins/host too, provided that you provide its source code as GPLv3 too.**
> * Note that you have to follow the Steinberg VST usage guidelines.  
>
> <https://steinbergmedia.github.io/vst3_dev_portal/pages/FAQ/Licensing.html>  

<img src="https://github.com/Kiriki-liszt/JS_Inflator_to_VST2_VST3/raw/main/VST_Compatible_Logo_Steinberg_with_TM.png"  width="200"/>

VSTSDK 3.7.9 used  
VSTGUI 4.12 used  

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

v2.0.0  

* Rename 'InflatorPackage' into 'JS Inflator'.  
* AUv2: Controller state was overwriting Processor state. Fixed.  
* Ctrl-Z: VU meter was using parameter to send data to Controller, and it caued 'undo history' to be filled with meter changes. Fixed.  
* Meters are now following envelope detector with time contants.  

v2.0.1: Fix for Crash for Ableton, and fix for VU meter in Twarch GUI.  

v2.0.2: Two GUIs are now integrated to one plugin.  

v2.0.2.1: GUI recall state corrected(Bitwig).  

v2.0.2.2: Re-structure how GUI switching works to more safe way.  

v2.0.3: Error in Apple Silicon Native build fixed.  

## What I've learned

* Volumefader  

For someone like me, wondering how to use volume fader;  
Use a RangeParameter!  
param as normalized parameter[0.0, 1.0],  
dB as Plain value,  
gain as multiplier of each samples.  
For normParam to gain, check ~process.cpp  

ex)  
| param  | dB   | gain  |
|------- |----- |------ |
| 0.0    | -12  | 0.25  |
| 0.5    | 0    | 1     |
| 1.0    | +12  | 4     |  

| param  | dB   | gain  |
|------- |----- |------ |
| 0.0    | -12  | 0.25  |
| 0.5    | -6   | 0.5   |
| 1.0    | 0    | 1     |  

* Resampling  

Generally, the process goes as:  

1. doubling samples  
2. LP Filtering  
3. ProcessAudio  
4. LP Filtering  
5. reducing samples  

* Linear Phase  

HIIR resampling is Min-phase resampler, meaning phase disorder at high freqs.  
For more natural high frequency hearing, Linear resampling such as r8brain-free-src designed by Aleksey Vaneev of Voxengo is recomanded.  
About weird choices for x4 and x8 - these resamplers have asynchronous latencies so downsampling starts little before upsampling starts.  
To fix it, I just changed Transition band for x4 and 24-bit for x8.  
Now it is free of Phase issuses.

* Comparisons  
1x  
<img src="https://github.com/Kiriki-liszt/JS_Inflator_to_VST2_VST3/raw/main/screenshots/OS_1x_Min.png"  width="400"/>  

2x Min-phase  
<img src="https://github.com/Kiriki-liszt/JS_Inflator_to_VST2_VST3/raw/main/screenshots/OS_2x_Min.png"  width="400"/>  

2x Lin-phase  
<img src="https://github.com/Kiriki-liszt/JS_Inflator_to_VST2_VST3/raw/main/screenshots/OS_2x_Lin.png"  width="400"/>  

4x Min-phase  
<img src="https://github.com/Kiriki-liszt/JS_Inflator_to_VST2_VST3/raw/main/screenshots/OS_4x_Min.png"  width="400"/>  

4x Lin-phase  
<img src="https://github.com/Kiriki-liszt/JS_Inflator_to_VST2_VST3/raw/main/screenshots/OS_4x_Lin.png"  width="400"/>  

8x Min-phase  
<img src="https://github.com/Kiriki-liszt/JS_Inflator_to_VST2_VST3/raw/main/screenshots/OS_8x_Min.png"  width="400"/>  

8x Lin-phase  
<img src="https://github.com/Kiriki-liszt/JS_Inflator_to_VST2_VST3/raw/main/screenshots/OS_8x_Lin.png"  width="400"/>  

* SIMD optimization

To my surprise, manual SSE2 SIMD optimization was slower then compiler's O3 maximum optimization.  
The compiler's vectorization was more efficient than mine... 
Anyone trying to do so, check and compare!  

* Latency change reporting  

Restarting plugin should be from Contorller side.  
One example would be using 'sendTextMessage' and 'receiveText' pair, so when Processor detects parameter change related to latency, it sends textMessage and Controller receives it and restarts.  

However, In AUv2, the restartComponent should call setupProcess, but it does not...  
Due to this, we should move any initializing into new custom function, and call it in setupProcess and process both, with checking if that new function is called.  

[https://forums.steinberg.net/t/reporting-latency-change/201601](https://forums.steinberg.net/t/reporting-latency-change/201601)  
[https://forums.steinberg.net/t/how-to-use-restartcomponent-and-which-flags-are-the-right-one-when-changing-all-characteristics-parameters-except-size/202031](https://forums.steinberg.net/t/how-to-use-restartcomponent-and-which-flags-are-the-right-one-when-changing-all-characteristics-parameters-except-size/202031)  
[https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical+Documentation/Workflow+Diagrams/Audio+Processor+Call+Sequence.html](https://steinbergmedia.github.io/vst3_dev_portal/pages/Technical+Documentation/Workflow+Diagrams/Audio+Processor+Call+Sequence.html)  

* Knob Modes

Specifying knob modes at 'createView' in 'controller.cpp' makes knobs work in selected mode.  

``` c++
setKnobMode(Steinberg::Vst::KnobModes::kLinearMode);
```

[https://github.com/Kiriki-liszt/JS_Inflator_to_VST2_VST3/blob/v1.6.0/source/InflatorPackagecontroller.cpp#L339](https://github.com/Kiriki-liszt/JS_Inflator_to_VST2_VST3/blob/v1.6.0/source/InflatorPackagecontroller.cpp#L339)  

* How VSTSDK identifies each plugins  

It uses "Steinberg::FUID kProcessorUID" in cid header to identify plugin.  
So, if ProcessorUID is kept same, host will see it as same plugin.  
In example, project using v1.7.0 'InflatorPackage' will automatically replace it with v2.0.0 'JS Inflator', with same settings.  

However, ParamIDs should be same as before while replacing old plugin with new one.  
If else, one should use Vst::IRemapParamID introduced in VSTSDK v3.7.11.  

* About AUv2  

While using AUv2 wrapper of VSTSDK, one should save Controller state also.  
IDK why, but in AUv2 wrapper overwrites values set from state to default UI values.  

The version number in plist is converted from hex to decimal.  
For example, v1.7.2 -> 0x010702 -> 67330.  

* VSTSDK with apple ARM Release setting  

By default, VSTSDK sets Release setting with -O3 and -ffast-math.  
It caused std::sqrt(1.0 - (i\*i)/(n\*n)) to return NaN instead of 0.0, when i == n.  

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

* [ ] Preset save & import menu bar.  
* [x] malloc/realloc/free to new/delete change. -> stick with it.
* [x] Oversampling options : Linear phase, Balanced.  
* [x] GUI : resizing, and HDDPI.  
* [ ] GUI : double click to enter value.
* [x] soft bypass latency.  

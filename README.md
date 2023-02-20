# JS_Inflator_to_VST2_VST3
JS Inflator, the copy of Sonox Inflator, in vstsdk

![VST](VST_Compatible_Logo_Steinberg_with_TM.png)

VSTSDK 3.7.7 used  
VSTGUI 4.12 used  

Built in VST3, but compatible to VST2, too.  


This is my first attempt with VSTSDK & VSTGUI.  
Some codes might be inefficent...  


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




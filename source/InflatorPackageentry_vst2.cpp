#include "public.sdk/source/vst/vst2wrapper/vst2wrapper.h"
#include "InflatorPackagecids.h"	// for class ids

//------------------------------------------------------------------------
::AudioEffect* createEffectInstance(audioMasterCallback audioMaster)
{
	return Steinberg::Vst::Vst2Wrapper::create(GetPluginFactory(), yg331::kInflatorPackageProcessorUID, 'ygIN', audioMaster);
}

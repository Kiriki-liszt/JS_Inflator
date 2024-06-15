//------------------------------------------------------------------------
// Copyright(c) 2023 Steinberg Media Technologies.
//------------------------------------------------------------------------

#pragma once

#include "public.sdk/source/vst/utility/dataexchange.h"
#include <cstdint>

//------------------------------------------------------------------------
namespace yg331 {

	//------------------------------------------------------------------------
	struct DataBlock
	{
		uint32_t sampleRate;
		uint32_t numSamples;
		float inL;
		float inR;
		float outL;
		float outR;
		float gR;
	};

	//------------------------------------------------------------------------
	inline DataBlock* toDataBlock(const Steinberg::Vst::DataExchangeBlock& block)
	{
		if (block.blockID != Steinberg::Vst::InvalidDataExchangeBlockID)
			return reinterpret_cast<DataBlock*> (block.data);
		return nullptr;
	} 

	//------------------------------------------------------------------------
} // Steinberg::Tutorial
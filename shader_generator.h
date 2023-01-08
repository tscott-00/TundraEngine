#pragma once

#include <string>

#include "general_common.h"

namespace GenerateShaderCode {
	std::string QuadVertex();
	std::string FinalStageFragment();
	std::string BrdfLUTFragment();

	std::string CubemapProcessingVertex();
	std::string IrradianceMapComputationFragment();
	std::string PrefilterdEnvironmentMapComputationFragment();

	std::string SkyboxVertex();
	std::string SkyboxFragment();

	std::string ObjectVertex();
	std::string TerrainVertex();
	std::string ObjectFragment(bool usesTextureArrays);
}

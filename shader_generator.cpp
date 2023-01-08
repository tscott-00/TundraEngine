#include "shader_generator.h"

// TODO: Replace with shader macro / compiler option system!

/*
std::string shader = "";

shader = shader +
"#version 430 core\n"

""

"void main()"
"{"
"	"
"}"
"";

return shader;
*/

std::string GenerateShaderCode::QuadVertex()
{
	std::string shader = "";

	shader = shader +
		"#version 430 core\n"

		"layout(location = 0) in vec2 in_position;"
		"layout(location = 1) in vec2 in_tex_coords;"

		"out vec2 pass_tex_coords;"

		"void main()"
		"{"
		"	gl_Position = vec4(in_position, 0.0, 1.0);"
		"	pass_tex_coords = in_tex_coords;"
		"}"
		"";

	return shader;
}

// TODO: Could use texelFetch
std::string GenerateShaderCode::FinalStageFragment()
{
	std::string shader = "";

	shader = shader +
		"#version 430 core\n"

		"in vec2 pass_tex_coords;"

		"out vec3 out_final_color;"

		"layout(location = 0) uniform float exposure;"
		"layout(binding = 0) uniform sampler2D raw_color_sampler;"

		"const float A = 2.51;"
		"const float B = 0.03;"
		"const float C = 2.43;"
		"const float D = 0.59;"
		"const float E = 0.14;"

		"const int indexMatrix[64] = int[]("
		"	0, 32, 8, 40, 2, 34, 10, 42,"
		"	48, 16, 56, 24, 50, 18, 58, 26,"
		"	12, 44, 4, 36, 14, 46, 6, 38,"
		"	60, 28, 52, 20, 62, 30, 54, 22,"
		"	3, 35, 11, 43, 1, 33, 9, 41,"
		"	51, 19, 59, 27, 49, 17, 57, 25,"
		"	15, 47, 7, 39, 13, 45, 5, 37,"
		"	63, 31, 55, 23, 61, 29, 53, 21);"

		"void main()"
		"{"
		"	vec3 color = texture(raw_color_sampler, pass_tex_coords).rgb * exposure;"

		// Aces approximation for tone mapping from https://64.github.io/tonemapping/
		"	color *= 0.6;"
		"	color = clamp((color * (A * color + B)) / (color * (C * color + D) + E), 0.0, 1.0);"

		// Applying dithering
		"	color += 0.5 / 255.0 * vec3((indexMatrix[int(mod(gl_FragCoord.x, 8)) + int(mod(gl_FragCoord.y, 8)) * 8]) / 64.0 - (1.0 / 128.0));"

		"	out_final_color = color;"
		"}"
		"";

	return shader;
}

std::string GenerateShaderCode::BrdfLUTFragment()
{
	std::string shader = "";

	shader = shader +
		"#version 430 core\n"

		"in vec2 pass_tex_coords;"

		"out vec2 out_color;"

		"const float PI = 3.14159265359;"
		"const uint SAMPLE_COUNT = 1024u;"

		"float RadicalInverse_VdC(uint bits)"
		"{"
		"	bits = (bits << 16u) | (bits >> 16u);"
		"	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);"
		"	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);"
		"	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);"
		"	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);"

		"	return float(bits) * 2.3283064365386963e-10;"
		"}"

		"vec2 Hammersley(uint i, uint N)"
		"{"
		"	return vec2(float(i) / float(N), RadicalInverse_VdC(i));"
		"}"

		"vec3 ImportanceSampleGGX(vec2 Xi, vec3 normal, float roughness)"
		"{"
		"	float a = roughness * roughness;"

		"	float phi = 2.0 * PI * Xi.x;"
		"	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));"
		"	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);"

		"	vec3 halfway = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);"

		"	vec3 up = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);"
		"	vec3 tangent = normalize(cross(up, normal));"
		"	vec3 bitangent = cross(normal, tangent);"

		"	return normalize(tangent * halfway.x + bitangent * halfway.y + normal * halfway.z);"
		"}"

		"float GeometrySchlickGGX(float NdotV, float roughness)"
		"{"
		"	float a = roughness;"
		"	float k = (a * a) / 2.0;"

		"	float nom = NdotV;"
		"	float denom = NdotV * (1.0 - k) + k;"

		"	return nom / denom;"
		"}"

		"float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)"
		"{"
		"	float NdotV = max(dot(N, V), 0.0);"
		"	float NdotL = max(dot(N, L), 0.0);"
		"	float ggx2 = GeometrySchlickGGX(NdotV, roughness);"
		"	float ggx1 = GeometrySchlickGGX(NdotL, roughness);"

		"	return ggx1 * ggx2;"
		"}"

		"void main()"
		"{"
		"	float NdotV = pass_tex_coords.x;"
		"	float roughness = pass_tex_coords.y;"

		"	vec3 V = vec3(sqrt(1.0 - NdotV * NdotV), 0.0, NdotV);"

		"	float A = 0.0;"
		"	float B = 0.0;"

		"	vec3 N = vec3(0.0, 0.0, 1.0);"

		"	for (uint i = 0u; i < SAMPLE_COUNT; ++i)"
		"	{"
		"		vec2 Xi = Hammersley(i, SAMPLE_COUNT);"
		"		vec3 H = ImportanceSampleGGX(Xi, N, roughness);"
		"		vec3 L = normalize(2.0 * dot(V, H) * H - V);"

		"		float NdotL = max(L.z, 0.0);"
		"		float NdotH = max(H.z, 0.0);"
		"		float VdotH = max(dot(V, H), 0.0);"

		"		if (NdotL > 0.0)"
		"		{"
		"			float G = GeometrySmith(N, V, L, roughness);"
		"			float G_Vis = (G * VdotH) / (NdotH * NdotV);"
		"			float Fc = pow(1.0 - VdotH, 5.0);"

		"			A += (1.0 - Fc) * G_Vis;"
		"			B += Fc * G_Vis;"
		"		}"
		"	}"

		"	out_color = vec2(A / float(SAMPLE_COUNT), B / float(SAMPLE_COUNT));"
		"}"
		"";

	return shader;
}

std::string GenerateShaderCode::CubemapProcessingVertex()
{
	std::string shader = "";

	shader = shader +
		"#version 430 core\n"

		"layout(location = 0) in vec3 in_position;"

		"out vec3 pass_position;"

		"layout(location = 0) uniform mat4 P_V;"

		"void main()"
		"{"
		"	gl_Position = P_V * vec4(in_position, 1.0);"

		"	pass_position = in_position;"
		"}"
		"";

	return shader;
}

std::string GenerateShaderCode::IrradianceMapComputationFragment()
{
	std::string shader = "";

	shader = shader +
		"#version 430 core\n"

		"in vec3 pass_position;"

		"out vec3 out_irradiance;"

		"layout(binding = 0) uniform samplerCube environment_map;"

		"const float PI = 3.14159265359;"

		"void main()"
		"{"
		"	vec3 normal = normalize(pass_position);"
		"	vec3 irradiance = vec3(0.0);"

		"	vec3 up = vec3(0.0, 1.0, 0.0);"
		"	vec3 right = cross(up, normal);"
		"	up = cross(normal, right);"

		"	float sampleDelta = 0.025;"
		"	int nrSamples = 0;"
		"	for (float phi = 0.0; phi < 2.0 * PI; phi += sampleDelta)"
		"	{"
		"		for (float theta = 0.0; theta < 0.5 * PI; theta += sampleDelta)"
		"		{"
		"			vec3 tangentSample = vec3(sin(theta) * cos(phi), sin(theta) * sin(phi), cos(theta));"
		"			vec3 sampleVec = tangentSample.x * right + tangentSample.y * up + tangentSample.z * normal;"

		"			irradiance += texture(environment_map, sampleVec).rgb * cos(theta) * sin(theta);"
		// i?
		"			nrSamples++;"
		"		}"
		"	}"

		"	out_irradiance = PI * irradiance * (1.0 / float(nrSamples));\n"
		"}"
		"";

	return shader;
}

std::string GenerateShaderCode::PrefilterdEnvironmentMapComputationFragment()
{
	std::string shader = "";

	shader = shader +
		"#version 430 core\n"

		"in vec3 pass_position;"

		"out vec3 out_color;"

		"layout(location = 1) uniform float roughness;"

		"layout(binding = 0) uniform samplerCube environment_map;"

		"const float PI = 3.14159265359;"
		"const uint SAMPLE_COUNT = 128u;"

		"float RadicalInverse_VdC(uint bits)"
		"{"
		"	bits = (bits << 16u) | (bits >> 16u);"
		"	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);"
		"	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);"
		"	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);"
		"	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);"

		"	return float(bits) * 2.3283064365386963e-10;"
		"}"

		"vec2 Hammersley(uint i, uint N)"
		"{"
		"	return vec2(float(i) / float(N), RadicalInverse_VdC(i));"
		"}"

		"vec3 ImportanceSampleGGX(vec2 Xi, vec3 normal, float roughness)"
		"{"
		"	float a = roughness * roughness;"

		"	float phi = 2.0 * PI * Xi.x;"
		"	float cosTheta = sqrt((1.0 - Xi.y) / (1.0 + (a * a - 1.0) * Xi.y));"
		"	float sinTheta = sqrt(1.0 - cosTheta * cosTheta);"

		"	vec3 halfway = vec3(cos(phi) * sinTheta, sin(phi) * sinTheta, cosTheta);"

		"	vec3 up = abs(normal.z) < 0.999 ? vec3(0.0, 0.0, 1.0) : vec3(1.0, 0.0, 0.0);"
		"	vec3 tangent = normalize(cross(up, normal));"
		"	vec3 bitangent = cross(normal, tangent);"

		"	return normalize(tangent * halfway.x + bitangent * halfway.y + normal * halfway.z);"
		"}"

		"void main()"
		"{"
		"	vec3 normal = normalize(pass_position);"

		"	float totalWeight = 0.0;"
		"	vec3 prefilteredColor = vec3(0.0);"
		"	for (uint i = 0u; i < SAMPLE_COUNT; ++i)"
		"	{"
		"		vec2 Xi = Hammersley(i, SAMPLE_COUNT);"
		"		vec3 halfway = ImportanceSampleGGX(Xi, normal, roughness);"
		"		vec3 sampleRay = normalize(2.0 * dot(normal, halfway) * halfway - normal);"

		"		float NdotR = max(dot(normal, sampleRay), 0.0);"
		"		if (NdotR > 0.0)"
		"		{"
		"			prefilteredColor += texture(environment_map, sampleRay).rgb;"
		"			totalWeight += NdotR;"
		"		}"
		"	}"

		"	out_color = prefilteredColor / totalWeight;"
		"}"
		"";

	return shader;
}

std::string GenerateShaderCode::SkyboxVertex()
{
	std::string shader = "";

	shader = shader +
		"#version 430 core\n"

		"layout(location = 0) in vec3 in_position;"

		"out vec3 pass_position;"

		"layout(location = 0) uniform mat4 projection;"
		"layout(location = 1) uniform mat4 rotView;"

		"void main()"
		"{"
		"	vec4 clipPos = (projection * rotView * vec4(in_position, 1.0));"
		"	gl_Position = clipPos.xyww;"

		"	pass_position = in_position;"
		"}"
		"";

	return shader;
}

std::string GenerateShaderCode::SkyboxFragment()
{
	std::string shader = "";

	shader = shader +
		"#version 430 core\n"

		"in vec3 pass_position;"

		"out vec3 out_color;"

		"layout(binding = 0) uniform samplerCube environmentMap;"

		"void main()"
		"{"
		"	out_color = texture(environmentMap, pass_position).rgb;"
		"}"
		"";

	return shader;
}

// TODO: Depricate!
std::string GenerateShaderCode::ObjectVertex()
{
	std::string shader = "";

	shader = shader +
		"#version 430 core\n"

		"layout(location = 0) in vec3 in_position;"
		"layout(location = 1) in vec2 in_tex_coords;"
		"layout(location = 2) in vec3 in_normal;"
		"layout(location = 3) in vec3 in_tangent;"

		"out vec2 pass_tex_coords;"
		"out vec3 pass_position;"
		"out vec3 pass_relative_camera_pos;"
		"out vec3 pass_directional_light_direction;"
		"out mat3 pass_TBN;"

		"layout(location = 0) uniform mat4 p_v_t;"
		"layout(location = 1) uniform mat3 rotation;"
		"layout(location = 2) uniform vec3 scale;"

		"layout(location = 3) uniform vec3 relative_camera_pos;"
		"layout(location = 4) uniform vec3 directional_light_direction;"

		"void main()"
		"{"
		"	gl_Position = p_v_t * vec4(in_position, 1.0);"

		"	vec3 N = in_normal;"
		"	vec3 T = in_tangent;"
		"	vec3 B = cross(in_tangent, in_normal);"
		"	pass_TBN = rotation * mat3(T, B, N);"

		"	pass_tex_coords = in_tex_coords;"
		"	pass_position = rotation * (in_position * scale);"
		// TODO: Interpolate out ray instead (and do it all shaders), or if that is impossible, just put this directly in the fragment (already done everywhere else)
		"	pass_relative_camera_pos = relative_camera_pos;"
		"	pass_directional_light_direction = directional_light_direction;"
		"}"
		"";

	return shader;
}

// TODO: Depricate!
std::string GenerateShaderCode::TerrainVertex()
{
	std::string shader = "";

	shader = shader +
		"#version 430 core\n"

		"layout(location = 0) in vec2 in_position;"
		"layout(location = 1) in vec2 in_tex_coords;"

		"out vec2 pass_tex_coords;"
		// Is this what was ruining the tangent space calculations?
		"out vec3 pass_position;"
		"out vec3 pass_relative_camera_pos;"
		"out vec3 pass_directional_light_direction;"
		"out mat3 pass_TBN;"
		"out float pass_tex_array_index;"

		"layout(location = 0) uniform mat4 p_v_t;"
		// TODO: don't have a location gap
		//"layout(location = 1) uniform mat3 rotation;"
		"layout(location = 2) uniform vec3 scale;"

		"layout(location = 3) uniform vec3 relative_camera_pos;"
		"layout(location = 4) uniform vec3 directional_light_direction;"

		"layout(location = 6) uniform ivec2 map_indices;"
		"layout(location = 7) uniform float texture_frequency;"

		"layout(binding = 6) uniform sampler2DArray elevation_sampler;"
		"layout(binding = 7) uniform sampler2DArray data_sampler;"

		"void main()"
		"{"
		"	vec3 position = vec3(in_position.x, texelFetch(elevation_sampler, ivec3(ivec2(in_tex_coords), map_indices.x), 0).x, in_position.y);"
		"	gl_Position = p_v_t * vec4(position, 1.0);"

		"	vec4 data = texelFetch(data_sampler, ivec3(ivec2(in_tex_coords), map_indices.y), 0);"
		"	vec3 N = normalize(vec3(data.yzw) * 2.0 - 1.0);"
		"	vec3 T = normalize(vec3(N.y, N.x, 0.0));" // Source: https://www.gamedev.net/forums/topic/651083-tangents-for-heightmap-from-limited-information/
		"	vec3 B = cross(T, N);"
		"	pass_TBN = mat3(T, B, N);"

		"	pass_tex_array_index = data.x * 255.0;"
		"	pass_tex_coords = in_tex_coords * texture_frequency;"
		"	pass_position = position * scale;"
		"	pass_relative_camera_pos = relative_camera_pos;"
		"	pass_directional_light_direction = directional_light_direction;"
		"}"
		"";

	return shader;
}

// TODO: Depricate!
std::string GenerateShaderCode::ObjectFragment(bool usesTextureArrays)
{
	std::string shader = "";
	// TODO: Is there a built-in fragment world pos?
	shader = shader +
		"#version 430 core\n"

		"in vec2 pass_tex_coords;"
		"in vec3 pass_position;"
		"in vec3 pass_relative_camera_pos;"
		"in vec3 pass_directional_light_direction;"
		"in mat3 pass_TBN;"
		+ (usesTextureArrays ? "in float pass_tex_array_index;" : "") +

		"out vec3 out_color;"

		"layout(location = 5) uniform vec3 directional_light_color;"

		"layout(binding = 0) uniform sampler2D brdfLUT;"
		"layout(binding = 1) uniform samplerCube prefiltered_environment_map;"
		"layout(binding = 2) uniform samplerCube irradiance_map;"

		+ (usesTextureArrays ?
			"layout(binding = 3) uniform sampler2DArray albedo_sampler;"
			"layout(binding = 4) uniform sampler2DArray normals_sampler;"
			"layout(binding = 5) uniform sampler2DArray materialData_sampler;"
			:
			"layout(binding = 3) uniform sampler2D albedo_sampler;"
			"layout(binding = 4) uniform sampler2D normals_sampler;"
			"layout(binding = 5) uniform sampler2D materialData_sampler;"
			) +

		"const float PI = 3.14159265359;"
		"const float ONE_OVER_PI = 1.0 / PI;"

		// Schlick
		"vec3 fresnel(float cosTheta, vec3 F0)"
		"{"
		"	return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);"
		"}"

		// Above but with roughness taken into account
		"vec3 fresnelRoughness(float cosTheta, vec3 F0, float roughness)"
		"{"
		"	return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(1.0 - cosTheta, 5.0);"
		"}"

		// GGX
		"float distribution(vec3 normal, vec3 halfway, float roughness)"
		"{"
		"	float a2 = roughness * roughness;"
		"	a2 *= a2;"
		"	float NdotH2 = max(dot(normal, halfway), 0.0);"
		"	NdotH2 *= NdotH2;"

		"	float denom = NdotH2 * (a2 - 1.0) + 1.0;"
		"	denom = PI * denom * denom;"

		"	return a2 / denom;"
		"}"

		// Schlick
		"float geometryGGX(float NdotR, float roughness)"
		"{"
		"	float k = roughness + 1.0;"
		"	k = (k*k) / 8.0;"

		"	return NdotR / (NdotR * (1.0 - k) + k);"
		"}"

		// Smith
		"float geometry(vec3 normal, vec3 inRay, vec3 outRay, float roughness)"
		"{"
		"	return geometryGGX(max(dot(normal, inRay), 0.0), roughness) * geometryGGX(max(dot(normal, outRay), 0.0), roughness);"
		"}"

		"void main()"
		"{"
		+ (usesTextureArrays ?
			"	vec3 array_tex_coords = vec3(pass_tex_coords, pass_tex_array_index);"
			"	vec3 albedo = texture(albedo_sampler, array_tex_coords).rgb;"
			"	vec3 normal = normalize(pass_TBN * normalize(texture(normals_sampler, array_tex_coords).xyz * 2.0 - 1.0));"
			"	vec3 materialData = texture(materialData_sampler, array_tex_coords).rgb;"
			:
			"	vec3 albedo = texture(albedo_sampler, pass_tex_coords).rgb;"
			"	vec3 normal = normalize(pass_TBN * normalize(texture(normals_sampler, pass_tex_coords).xyz * 2.0 - 1.0));"
			"	vec3 materialData = texture(materialData_sampler, pass_tex_coords).rgb;"
			) +
		"	float roughness = materialData.r;"
		"	float metallic = materialData.g;"
		"	float ao = materialData.b;"
		//"normal = temp;"

		"	vec3 F0 = mix(vec3(0.04), albedo, metallic);"
		"	vec3 outRay = normalize(pass_relative_camera_pos - pass_position);"

		// Initialize with ambient irradiance
		"	vec3 aFactor = fresnelRoughness(max(dot(normal, outRay), 0.0), F0, roughness);"

		"	vec3 aDiffuse = texture(irradiance_map, normal).rgb * albedo;"

		"	vec2 envBRDF = texture(brdfLUT, vec2(max(dot(normal, outRay), 0.0), roughness)).rg;"
		"	vec3 aSpecular = textureLod(prefiltered_environment_map, reflect(-outRay, normal), roughness * 4.0).rgb * (aFactor * envBRDF.x + envBRDF.y);"
		// Should 1.0 - metallic be here?
		"	vec3 irradiance = ((1.0 - aFactor) * (1.0 - metallic) * aDiffuse + aSpecular) * ao;"

		// Directional Light
		"	vec3 inRay = pass_directional_light_direction;"
		"	vec3 halfway = normalize(inRay + outRay);"
		"	vec3 radiance = directional_light_color;"
		"	vec3 F = fresnel(max(dot(halfway, outRay), 0.0), F0);"
		"	float D = distribution(normal, halfway, roughness);"
		"	float G = geometry(normal, inRay, outRay, roughness);"
		// TODO: Re-use dots from G calculation
		"	vec3 specular = (D * G * F) / max(4.0 * max(dot(normal, inRay), 0.0) * max(dot(normal, outRay), 0.0), 0.001);"
		"	vec3 refractionRatio = (vec3(1.0) - F) * (1.0 - metallic);"
		"	irradiance += (refractionRatio * albedo * ONE_OVER_PI + specular) * radiance * max(dot(normal, inRay), 0.0);"

		"	out_color = irradiance;"
		"}"
		"";

	return shader;
}
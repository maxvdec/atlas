#version 410 core
out vec4 FragColor;

in vec2 TexCoord;

uniform sampler2D gPosition;   // world-space position
uniform sampler2D gNormal;     // world-space normal
uniform sampler2D gMaterial;   // metallic, roughness, reflectivity
uniform mat4 projection;
uniform mat4 view;
uniform vec3 cameraPosition;

const float kReflectivityThreshold = 0.02;
const float kRayStartBias = 0.15;
const float kThicknessMin = 0.01;
const float kThicknessMax = 0.08;
const float kStrideFactor = 0.02;
const float kMinStride = 0.04;
const float kMaxStride = 1.0;
const float kMaxDistance = 160.0;
const int kMaxMarchSteps = 96;
const int kBinarySearchSteps = 6;

vec3 fetchWorldPosition(vec2 uv)
{
	return texture(gPosition, uv).xyz;
}

vec3 fetchNormal(vec2 uv)
{
	return texture(gNormal, uv).xyz;
}

vec3 fetchMaterial(vec2 uv)
{
	return texture(gMaterial, uv).xyz;
}

vec3 toViewSpace(vec3 worldPos)
{
	return (view * vec4(worldPos, 1.0)).xyz;
}

bool projectToScreen(vec3 viewPos, out vec2 uv)
{
	vec4 clip = projection * vec4(viewPos, 1.0);
	if (clip.w <= 0.0)
	{
		return false;
	}

	vec3 ndc = clip.xyz / clip.w;
	if (ndc.z < -1.0 || ndc.z > 1.0)
	{
		return false;
	}

	uv = ndc.xy * 0.5 + 0.5;
	return uv.x >= 0.0 && uv.x <= 1.0 && uv.y >= 0.0 && uv.y <= 1.0;
}

float computeStride(vec3 positionVS)
{
    return clamp(abs(positionVS.z) * kStrideFactor, kMinStride, kMaxStride);
}

float computeThickness(vec3 sampleVS)
{
	float depth = abs(sampleVS.z);
	return clamp(depth * 0.008, kThicknessMin, kThicknessMax);
}

bool isFrontFacing(vec3 normalWS, vec3 reflectionDirWS)
{
	return dot(normalWS, reflectionDirWS) < -0.05;
}

bool traceSSR(vec3 originVS, vec3 reflectionDirVS, vec3 reflectionDirWS, out vec2 hitUV)
{
	if (reflectionDirVS.z >= -0.001)
	{
		return false;
	}

	vec3 rayVS = originVS + reflectionDirVS * kRayStartBias;
	vec3 previousRayVS = originVS;

	float traveled = kRayStartBias;
	float previousDiff = 0.0;
			bool hasPrevious = false;

	for (int step = 0; step < kMaxMarchSteps; ++step)
	{
		if (traveled > kMaxDistance || rayVS.z >= 0.0)
		{
			break;
		}

		vec2 uv;
		if (!projectToScreen(rayVS, uv))
		{
			break;
		}

		vec3 sampleNormal = fetchNormal(uv);
		if (length(sampleNormal) < 0.1)
		{
			float strideSkip = computeStride(rayVS);
			rayVS += reflectionDirVS * strideSkip;
			traveled += strideSkip;
			hasPrevious = false;
			continue;
		}

		vec3 sampleWorld = fetchWorldPosition(uv);
		vec3 sampleVS = toViewSpace(sampleWorld);
		vec3 sampleNormalWS = normalize(sampleNormal);

		if (sampleVS.z >= 0.0)
		{
			float strideSkip = computeStride(rayVS);
			rayVS += reflectionDirVS * strideSkip;
			traveled += strideSkip;
			hasPrevious = false;
			continue;
		}

		if (!isFrontFacing(sampleNormalWS, reflectionDirWS))
		{
			float strideSkip = computeStride(rayVS);
			rayVS += reflectionDirVS * strideSkip;
			traveled += strideSkip;
			hasPrevious = false;
			continue;
		}

		float diff = sampleVS.z - rayVS.z;
		float thickness = computeThickness(sampleVS);

		if (abs(diff) <= thickness)
		{
			hitUV = uv;
			return true;
		}

		if (hasPrevious && diff > 0.0 && previousDiff < 0.0)
		{
			vec3 startVS = previousRayVS;
			vec3 endVS = rayVS;
			vec2 refinedUV = uv;
			bool refined = false;

			for (int i = 0; i < kBinarySearchSteps; ++i)
			{
				vec3 midVS = mix(startVS, endVS, 0.5);
				vec2 midUV;

				if (!projectToScreen(midVS, midUV))
				{
					refined = false;
					break;
				}

				vec3 midWorld = fetchWorldPosition(midUV);
				vec3 midNormal = fetchNormal(midUV);
				float midNormalLen = length(midNormal);
				if (midNormalLen < 0.1)
				{
					refined = false;
					break;
				}
				vec3 midNormalWS = midNormal / midNormalLen;
				if (!isFrontFacing(midNormalWS, reflectionDirWS))
				{
					refined = false;
					break;
				}
				vec3 midSampleVS = toViewSpace(midWorld);
				float midDiff = midSampleVS.z - midVS.z;
				float midThickness = computeThickness(midSampleVS);

				if (abs(midDiff) <= midThickness)
				{
					hitUV = midUV;
					refined = true;
					return true;
				}

				if (midDiff > 0.0)
				{
					endVS = midVS;
					refinedUV = midUV;
				}
				else
				{
					startVS = midVS;
				}
			}

			if (refined)
			{
				hitUV = refinedUV;
				return true;
			}

			hasPrevious = false;
			continue;
		}

		hasPrevious = true;
		previousDiff = diff;
		previousRayVS = rayVS;

		float stride = computeStride(rayVS);
		rayVS += reflectionDirVS * stride;
		traveled += stride;
	}

	return false;
}

void main()
{
	vec3 material = fetchMaterial(TexCoord);
	float reflectivity = clamp(material.z, 0.0, 1.0);

	if (reflectivity <= kReflectivityThreshold)
	{
		FragColor = vec4(0.0);
		return;
	}

	vec3 worldPos = fetchWorldPosition(TexCoord);
	vec3 normalWS = normalize(fetchNormal(TexCoord));

	if (length(normalWS) < 0.1)
	{
		FragColor = vec4(0.0);
		return;
	}

	vec3 originVS = toViewSpace(worldPos);
	vec3 viewDirVS = normalize(-originVS);
	vec3 viewDirWS = normalize(cameraPosition - worldPos);
	if (length(viewDirVS) < 1e-4 || length(viewDirWS) < 1e-4)
	{
		FragColor = vec4(0.0);
		return;
	}
	vec3 reflectionDirWS = normalize(reflect(viewDirWS, normalWS));
	vec3 reflectionDirVS = normalize(mat3(view) * reflectionDirWS);

	vec2 hitUV;
	bool hit = traceSSR(originVS, reflectionDirVS, reflectionDirWS, hitUV);

	if (!hit)
	{
		FragColor = vec4(0.0);
		return;
	}

	vec2 reversedUV = vec2(1.0) - hitUV;
	FragColor = vec4(reversedUV, 1.0, reflectivity);
}
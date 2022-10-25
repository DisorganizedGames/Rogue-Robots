#include "ShaderInterop_Base.h"

SamplerState g_aniso_samp : register(s0, space1); // wrap, MaxAnisotropy = 8
SamplerState g_point_samp : register(s1, space1); // wrap
SamplerState g_point_clamp_samp : register(s2, space1);
SamplerState g_bilinear_wrap_samp : register(s3, space1);
SamplerState g_bilinear_clamp_samp : register(s4, space1);
SamplerState g_point_samp_shadows : register(s5, space1);
SamplerComparisonState g_linear_PCF_sampler : register(s6, space1);
// https://64.github.io/tonemapping/

static const float3 aces_input_matrix[] =
{
    float3(0.59719f, 0.35458f, 0.04823f),
    float3(0.07600f, 0.90834f, 0.01566f),
    float3(0.02840f, 0.13383f, 0.83777f)
};

static const float3 aces_output_matrix[] =
{
    float3(1.60475f, -0.53108f, -0.07367f),
    float3(-0.10208f, 1.10813f, -0.00605f),
    float3(-0.00327f, -0.07276f, 1.07602f)
};

float3 mul(float3 v)
{
    float x = aces_input_matrix[0][0] * v[0] + aces_input_matrix[0][1] * v[1] + aces_input_matrix[0][2] * v[2];
    float y = aces_input_matrix[1][0] * v[1] + aces_input_matrix[1][1] * v[1] + aces_input_matrix[1][2] * v[2];
    float z = aces_input_matrix[2][0] * v[1] + aces_input_matrix[2][1] * v[1] + aces_input_matrix[2][2] * v[2];
    return float3(x, y, z);
}

float3 mul2(float3 v)
{
    float x = aces_output_matrix[0][0] * v[0] + aces_output_matrix[0][1] * v[1] + aces_output_matrix[0][2] * v[2];
    float y = aces_output_matrix[1][0] * v[1] + aces_output_matrix[1][1] * v[1] + aces_output_matrix[1][2] * v[2];
    float z = aces_output_matrix[2][0] * v[1] + aces_output_matrix[2][1] * v[1] + aces_output_matrix[2][2] * v[2];
    return float3(x, y, z);
}

float3 rtt_and_odt_fit(float3 v)
{
    float3 a = v * (v + 0.0245786f) - 0.000090537f;
    float3 b = v * (0.983729f * v + 0.4329510f) + 0.238081f;
    return a / b;
}

float3 aces_fitted(float3 v)
{
    v = mul(v);
    v = rtt_and_odt_fit(v);
    return mul2(v);
}
/*
    Draw 3 vertices with a pipeline using this VS

    We arbitrarily select AD.
    Any number works and the fullscreen triangle UVs will be set accordingly and interpolated correctly.

  A
    |\
    | \
    |  \
    |   \
    |    \
    |     \
    |      \
    |       \
    |        \
  D |_________\ E
    |         |\
    |         | \
    |         |  \
    |_________|___\
    B         F      C

    Given that the box DEBF is NDC:
        DE = EF = FB = FB = 2

    (AD / DE) = (EF / FC)
    (AD / 2) = (2 / FC)
    AD = 4 / FC
    FC = 4 / AD

    -----------------------------------

    To solve for the UVs, we simply grab the side ratios of our chosen triangle in order
    to use it for the UV coordinate system.

    FC / DE = Ratio of the horizontal side
    AD / EF = Ratio of the vertical side

    With this ratio, no matter which coordinate system we are in, we can derive FC and AD
    given DE and EF in the corresponding coordinate system (which in this case is the texture coordinate system)

*/

const static float ad = 2.f;
const static float fc = 4.f / ad;

// NDC is 2 in width and height
const static float fc_de = fc / 2.f;
const static float ad_ef = ad / 2.f;

const static float3 positions[] =
{
    float3(-1.f, 1.f + ad, 0.f),
    float3(1.f + fc, -1.f, 0.f),
    float3(-1.f, -1.f, 0.f)
};

const static float uv_space_len = 1.f;    // DE == EF == 1
const static float2 uvs[] =
{
    float2(0.f, 0.f - uv_space_len * ad_ef),
    float2(1.f + uv_space_len * fc_de, 1.f),
    float2(0.f, 1.f)
};

struct VS_OUT
{
    float4 pos : SV_POSITION;
    float2 uv : TEXCOORD;
};

VS_OUT main(uint id : SV_VertexID)
{
    VS_OUT output = (VS_OUT)0;
    output.pos = float4(positions[id], 1.f);
    output.uv = uvs[id];

    return output;
}
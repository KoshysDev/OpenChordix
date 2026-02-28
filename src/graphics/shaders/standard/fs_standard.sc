$input v_worldPos, v_normal, v_tangent, v_uv

#include <common.sh>

SAMPLER2D(s_baseColor, 0);
SAMPLER2D(s_metallicRoughness, 1);
SAMPLER2D(s_normal, 2);
SAMPLER2D(s_emissive, 3);
SAMPLER2D(s_occlusion, 4);

uniform vec4 u_baseColorFactor;
uniform vec4 u_metallicRoughness;
uniform vec4 u_emissiveFactor;
uniform vec4 u_miscParams;
uniform vec4 u_glowColor;
uniform vec4 u_glowParams;
uniform vec4 u_cameraPos;
uniform vec4 u_lightDir;
uniform vec4 u_lightColor;
uniform vec4 u_envTop;
uniform vec4 u_envBottom;
uniform vec4 u_envParams;

void main()
{
    vec4 baseColor = u_baseColorFactor;
    vec4 baseTex = texture2D(s_baseColor, v_uv);
    baseColor *= mix(vec4(1.0), baseTex, u_miscParams.y);

    float alphaCutoff = u_miscParams.x;
    if (alphaCutoff >= 0.0 && baseColor.a < alphaCutoff)
    {
        discard;
    }

    vec3 N = normalize(v_normal);
    vec3 T = normalize(v_tangent.xyz);
    vec3 B = normalize(cross(N, T)) * v_tangent.w;
    vec3 mapN = texture2D(s_normal, v_uv).xyz * 2.0 - 1.0;
    mapN.xy *= u_miscParams.w;
    N = normalize(mat3(T, B, N) * mapN);

    vec3 V = normalize(u_cameraPos.xyz - v_worldPos);
    vec3 L = normalize(-u_lightDir.xyz);
    vec3 H = normalize(V + L);

    float metallic = clamp(u_metallicRoughness.x, 0.0, 1.0);
    float roughness = clamp(u_metallicRoughness.y, 0.04, 1.0);

    vec4 mrSample = texture2D(s_metallicRoughness, v_uv);
    metallic *= mrSample.b;
    roughness *= mrSample.g;

    float NdotL = max(dot(N, L), 0.0);
    float NdotH = max(dot(N, H), 0.0);

    vec3 albedo = baseColor.rgb;
    vec3 specColor = mix(vec3(0.04), albedo, metallic);
    float specPower = mix(128.0, 8.0, roughness);
    float spec = pow(max(NdotH, 0.0), specPower) * NdotL;

    vec3 lightColor = u_lightColor.rgb * u_lightColor.a;
    vec3 diffuse = albedo;

    float ao = mix(1.0, texture2D(s_occlusion, v_uv).r, u_miscParams.z);
    float hemi = clamp(N.y * 0.5 + 0.5, 0.0, 1.0);
    vec3 envColor = mix(u_envBottom.rgb, u_envTop.rgb, hemi) * u_envParams.x;
    vec3 ambient = envColor * diffuse * ao;

    vec3 emissiveTex = texture2D(s_emissive, v_uv).rgb;
    vec3 emissive = u_emissiveFactor.rgb * u_emissiveFactor.a * emissiveTex;

    float rim = pow(1.0 - clamp(dot(N, V), 0.0, 1.0), max(u_glowParams.y, 1e-3));
    vec3 glow = u_glowColor.rgb * (u_glowParams.x * rim);

    vec3 color = diffuse * lightColor * NdotL + specColor * lightColor * spec + ambient + emissive + glow;
    gl_FragColor = vec4(color, baseColor.a);
}

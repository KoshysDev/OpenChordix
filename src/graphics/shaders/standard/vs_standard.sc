$input a_position, a_normal, a_tangent, a_texcoord0
$output v_worldPos, v_normal, v_tangent, v_uv

#include <common.sh>

void main()
{
    vec4 worldPos = mul(u_model[0], vec4(a_position, 1.0));
    v_worldPos = worldPos.xyz;
    v_normal = normalize(mul(u_model[0], vec4(a_normal, 0.0)).xyz);

    vec3 tangent = normalize(mul(u_model[0], vec4(a_tangent.xyz, 0.0)).xyz);
    v_tangent = vec4(tangent, a_tangent.w);
    v_uv = a_texcoord0;

    gl_Position = mul(u_modelViewProj, vec4(a_position, 1.0));
}

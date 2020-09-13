
precision mediump float;
in vec4 COL0;
uniform samplerCube rglSampler0;
uniform samplerCube rglSampler1;
in vec4 TEXCOORD0;
in vec3 SUNDIR;
in vec3 VERTPOS;
in vec3 EYE;

float linstep( float begin, float end, float v ) {
    return min( 1.0, max( 0.0 , ( v - begin ) / ( end - begin ) ) );
}

float SmoothWindow( float begin, float end, float v ) {
    float hw = 0.003;
    return linstep( begin - hw, begin + hw, v ) * linstep( end + hw, end - hw, v );
}

void main() {
    vec4 p = COL0;
    vec4 s;
    vec3 ng = TEXCOORD0.xyz; // normalize( TEXCOORD0.xyz );
    s = textureCube( rglSampler0, TEXCOORD0.xyz, 2.0 ) * 0.9 + 0.2;
    p = p * s;
    vec3 n = textureCube( rglSampler1, TEXCOORD0.xyz, 2.0 ).xyz * 2.0 - 1.0;
    n = mix( ng, n, 0.01 );
    vec3 sunDir = normalize( SUNDIR );
    vec3 e = normalize( EYE - VERTPOS );
    float d = dot( sunDir, n );
    float dg = dot( sunDir, ng );
    float d2 = d * linstep( -0.02, 0.02, dg );
    float ds = linstep( -0.01, 0.01, d );
    vec3 h = normalize( e + sunDir );
    vec3 sc = mix( vec3( 1, 0.9, 0.5 ), s.xyz, 0.5 );
    float sd = max( 0.0, dot( h, n ) ) * 2.0 - 1.0;
    vec3 spec = 0.5 * sc * sd * sd;
    gl_FragColor.xyz = s.xyz * d2 + vec3(spec) * ds;
    float band = 0.5 + 0.5 * SmoothWindow( -0.39, -0.15, dg );
    gl_FragColor.xyz = mix( s.xyz * band, gl_FragColor.xyz, max( 0.0, min( 1.0, dg * 20.0 + 0.5 ) ));
    gl_FragColor.w = 1.0;
}




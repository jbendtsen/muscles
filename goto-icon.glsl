void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fragCoord/iResolution.xy;

    uv.x *= iResolution.x / iResolution.y;
    uv.y = 1.0 - uv.y;

    if (uv.x >= 1.0) {
        fragColor = vec4(1, 1, 1, 1);
        return;
    }

    float x = uv.x - 0.5;
    float y = uv.y - 0.5;
	int col = 1;
    
    float cx = x * 1.1 + 0.1;
    float cy = y - 0.1;
    if (cy < 0.0 && cy-cx < 0.0) {
        float outer = sqrt(cx * cx + cy * cy);
        float ix = x + 0.15;
        float inner = sqrt(ix * ix + cy * cy);
        if (inner >= 0.3 && outer < 0.45)
            col = 2;
    }

    if (cy >= 0.0 && y + abs(x - 0.225) < 0.28)
        col = 2;

    vec3 rgb = vec3(1, 1, 1);
    if (col == 1)
        rgb = vec3(0, 0, 0);
    else if (col == 2)
        rgb = 0.5 + 0.5*cos(iTime+uv.xyx+vec3(0,2,4));

    // Output to screen
    fragColor = vec4(rgb,1.0);
}
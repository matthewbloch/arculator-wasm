#version 330
precision highp float;
out vec4 FragColor;
in vec3 ourColor;
in vec2 TexCoord;
uniform sampler2D arcvideo;
uniform sampler2D overlay;
uniform vec4 zoom;
uniform int frame;
void main()
{
    vec4  izoom = zoom;
    ivec2 isize = textureSize(arcvideo, 0);
    vec2  size = vec2(isize.x, isize.y);

    izoom.y += float(frame) * 1024.0;

    // we flipped the y co-ordinate of the texture, compensate for that
    izoom.y += izoom.w;

    // pan and zoom into the texture wherever we're told by the VIDC
    vec2 zoomed = izoom.xy / size + TexCoord * (izoom.zw / size);

    // We've copied the VIDC memory directly into the texture, 
    // which is BGRA with alpha at 0.
    //
    // So flip it to RGBA which is the portable format the texture is set up for,
    // and force the alpha to 1.0.
    vec4 overlayPixel = texture(overlay, TexCoord).bgra;
    if (overlayPixel.a == 0.0) {
        FragColor = texture(arcvideo, zoomed).bgra;
    } else {
        FragColor = overlayPixel;
    }
    //FragColor = mix(
    //    texture(arcvideo, zoomed).bgra, 
    //    texture(overlay, TexCoord).bgra,
    //    1.0
    //);
    //FragColor = texture(arcvideo, zoomed).bgra;
    FragColor.a = 1.0;
}

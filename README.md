# CS488 Introduction to Computer Graphics Final Project

Simple raytracer that handles:
- Refraction, Snell-style mixed with reflection via a Schlick-style Fresnel term. Handles total internal reflection (reflection in general) and mirror surfaces.
- Blinn–Phong lighting with distance light falloff.
- Shadows, attenuated through participating media and transparent objects when along shadow ray
- Texture mapping: diffuse textures modulate kd; mip-mapped sampling with bilinear filtering
- Normal / bump mapping
- Participating media: ray marching inside bounded volumes: absorption + single scattering toward lights, density from fBm noise
- Adaptive supersampling: coarse grid first, extra samples when luminance variance exceeds a threshold
- Parallel rendering: 16×16 tiles farmed to worker threads

## Final Project

Animation of a Newton's cradle with somewhat realistic physics and sound.

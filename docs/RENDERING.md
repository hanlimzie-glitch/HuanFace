# Rendering — HuanFace SDK (Draft)

Rendering pipeline:

```
Input Image
↓
Face Detection
↓
Landmark
↓
Face Mesh
↓
Makeup/Beauty Effects (shaders, textures, blend)
↓
Compositor
↓
Output Image
```

- Texture, Shader, Mesh, Compositor modules
- OpenGL/D3D11 backends
- Blend modes: Normal, Multiply, Screen, Overlay, etc.

TODO: Detailed design in Phase 4.

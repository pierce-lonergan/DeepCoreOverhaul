<!-- Research document. Authored by source reading (src/game3d/DeepCore3D.cpp,
     src/sandbox/SyntheticLevel.{hpp,cpp}, src/game3d/deepcore3d.vcxproj) and by public
     specification/reference material cited inline. NO BUILD WAS RUN for this document and no
     binary was executed: the lead engineer is compiling concurrently and the brief forbids it.
     Consequently every performance number below is either (a) exactly derivable by counting
     draw calls in the shipped source, and is marked COUNTED, or (b) an analytic estimate from
     the level generator's parameters, and is marked ESTIMATE with the arithmetic shown and the
     ~12-line instrumentation that would replace it with a measurement. Nothing is asserted as
     observed. -->

# From OpenGL 1.x immediate mode to a GL 3.3 core pipeline, with zero dependencies

Repo: `C:/Users/Pierce Lonergan/Documents/GitHub/DeepCoreOverhaul`
Subject: `src/game3d/DeepCore3D.cpp` (725 lines), `src/game3d/deepcore3d.vcxproj` (v142, Win32, /W4)
Benchmark named by the user: Manic Miners. We will not match it. This document is about which
fraction of the perceived gap is buyable per engineer-day.

---

## 0. Headline, so nobody has to read to the end

1. **There is a visual upgrade you can ship tonight, in the existing 1.x renderer, in about
   50 lines, and it is the single largest perceived gain in this entire document: per-vertex
   ambient occlusion.** Voxel AO is computed on the CPU and fed to `glColor3f`. It does not
   need shaders, VBOs, or a new context. See §4.3 and §9. Everything else in this document is
   downstream of a port; this one is not. Do it first, out of order, tonight.

2. **The port itself buys no pixels.** Context creation, the extension loader, a `mat4`
   library to replace GLU, a font atlas to replace GDI text, and a static VBO for terrain is
   roughly **900 lines that make the screen look identical**. It is the cost of entry, not a
   feature. Budget it honestly and do it in one sitting so it is never half-done.

3. **Hidden-face culling is worth ~4.3×; greedy meshing after that is worth ~4× more and you
   should not do it.** Post-cull the terrain is ~2,285 quads / ~4,570 triangles / **210 KB in
   one draw call** (§3.2). Greedy meshing removes ~1,700 triangles. That is not a frame. It
   also actively fights per-vertex AO (§3.3), which is the thing that actually shows. Cull;
   do not greed. This is the one place where the standard voxel-engine advice is wrong for us,
   because our world is 1,600 tiles and Minecraft's is not.

4. **Skip cascades, skip SSAO, skip PBR.** In a dark cave the three things that carry
   readability are, in order: **ambient occlusion, rim light, and point lights that move**
   (helmet lamps, crystal glow). Those are ~130 lines of GLSL between them and they are worth
   more than a correct BRDF.

5. **Take no third-party code, but do vendor two Khronos headers.** `GL/glext.h` and
   `GL/wglext.h` are `SPDX-License-Identifier: MIT`, are pure `#define`s and `typedef`s, add
   **zero bytes to the binary**, and remove an entire class of transcription bug (§2.1). This
   is not a dependency in any meaningful sense. GLEW and GLAD are still refused.

6. **The `/W4` + **x86** combination has exactly one trap that will destroy you and it is not
   a warning: the `APIENTRY` (`__stdcall`) calling convention on every GL function-pointer
   typedef.** Omit it and you corrupt the stack on x86 while the identical code works fine on
   x64. See §2.4. Read that section before writing a line.

### Ranking, all items, by (perceived gain) / (cost)

| # | Item | § | LOC | Risk | Gain | Ratio |
|---|---|---|---|---|---|---|
| 1 | **Per-vertex AO (in the 1.x renderer, today)** | 4.3 | 50 | very low | **very high** | **★★★★★** |
| 2 | sRGB-correct output (`GL_FRAMEBUFFER_SRGB`) | 4.2 | 4 | very low | high | ★★★★★ |
| 3 | Dither before 8-bit output | 7.5 | 6 | very low | med-high (kills cave banding) | ★★★★★ |
| 4 | Rim light | 4.5 | 8 | very low | **very high in caves** | ★★★★★ |
| 5 | Moving point lights (helmet lamps, crystals) | 4.7 | 45 | low | very high | ★★★★☆ |
| 6 | ACES tonemap + exposure + vignette | 7.4 | 25 | low | high | ★★★★☆ |
| 7 | Emissive crystals → bloom | 7.3 | 160 | low | high | ★★★★☆ |
| 8 | Procedural triplanar rock/floor texture | 6 | 230 | low | **very high** | ★★★★☆ |
| 9 | Hidden-face culling + one static VBO | 3.2 | 180 | low | none visually, enables all | ★★★★☆ |
| 10 | **Tier 0 enabler** (context, loader, mat4, font, picking) | 2 | ~710 | **medium** | zero | ★★★☆☆ (mandatory) |
| 11 | Per-pixel normals + Blinn specular | 4.1, 4.6 | 30 | low | medium | ★★★☆☆ |
| 12 | MSAA via multisample FBO + blit resolve | 7.1 | 40 | low | medium | ★★★☆☆ |
| 13 | Height fog + exp² distance fog | 4.1 | 12 | very low | medium | ★★★☆☆ |
| 14 | KHR_debug callback (our substitute GPU debugger) | 2.5 | 35 | very low | zero visual, saves days | ★★★★☆ |
| 15 | Directional shadow map + 3×3 PCF | 5 | 200 | **medium-high** | low-medium *unless the light is tilted* | ★★☆☆☆ |
| 16 | Greedy meshing | 3.3 | 150 | medium | **zero** | ★☆☆☆☆ **do not** |
| 17 | SSAO | — | 220 | high | **negative** (baked AO is better here) | ☆☆☆☆☆ **do not** |

The single highest-value thing to do first: **item 1, and it does not require item 10.**
The single highest-value thing to do first *that requires the port*: **item 4 (rim), which is
eight lines of GLSL once the port exists.**

---

## 1. What we are actually starting from — COUNTED

Read from the shipped source, not estimated.

`Cube()` at `DeepCore3D.cpp:345-369` emits, per call: one `glBegin(GL_QUADS)`/`glEnd()` pair,
**6 faces**, **6 `glColor3f`**, **24 `glVertex3fv`**. It always emits all six faces; there is
no culling of any kind beyond `GL_CULL_FACE` backface rejection, which happens *after* the
vertices have crossed the driver boundary.

`RenderWorld()` at `:437-507` draws exactly one `Cube()` per tile, plus a second `Cube()` for
crystal seams (`:453`), ore seams (`:458`), and the Tool Store (`:471`).

`NewLevel()` at `:160-196` sets `d.width = 40; d.height = 40; d.crystalSeams = 22;
d.oreSeams = 14`.

| Quantity | Value | Derivation |
|---|---:|---|
| Tiles | 1,600 | 40 × 40 |
| Terrain `Cube()` calls / frame | **1,637** | 1600 + 22 crystal toppers + 14 ore toppers + 1 Tool Store base |
| Terrain quads / frame | **9,822** | 1,637 × 6 |
| Terrain triangles / frame | **19,644** | 9,822 × 2 |
| `glBegin`/`glEnd` pairs / frame | **1,637** | one per `Cube()` |
| `glVertex3fv` calls / frame | **39,288** | 1,637 × 24 |
| `glColor3f` calls / frame | **9,822** | 1,637 × 6 |
| Figures (5 miners × 6 cubes, ~5 monsters × 7 cubes) | +65 cubes | `DrawMiner` `:389-397`, `DrawMonster` `:424-433` |
| **Total immediate-mode API calls / frame** | **~52,000** | |
| **…at 60 Hz** | **~3.1 M/s** | |

Three further facts that matter for the port:

- **Picking stalls the pipeline.** `PickTile()` (`:510-527`) calls `glReadPixels` on
  `GL_DEPTH_COMPONENT` **on the mouse-event thread, inside `WndProc`**, before `SwapBuffers`.
  A depth readback is a full CPU/GPU sync. It is invisible today because we are drawing
  nothing, but it must not survive the port. §2.7 replaces it with a CPU DDA that is exact,
  free, and also removes `glu32` entirely.
- **The HUD is GDI `TextOutA` onto the GL device context** (`:603-608`, `:697-714`), which is
  a compatibility-profile behaviour with undefined interaction with double buffering. It will
  not work in a core context. §2.8 replaces it.
- **`Cube()`'s per-face constant `k` factor** (`:356-361`) is already a fake N·L. Once we have
  real normals it goes away, and the difference in look will be small — which is exactly why
  "add lighting" is not the answer. AO and rim are.

### 1.1 What "reads as a prototype" actually means, mechanically

Four properties, in the order a viewer notices them:

1. **Every surface is a single flat colour.** No texture, no variation, no detail frequency
   above 1 tile. → §6.
2. **Nothing is darker where geometry meets geometry.** Real rooms have contact darkening; the
   eye reads its absence as "untextured boxes". → §4.3. This is the big one.
3. **Silhouettes vanish into the fog colour.** Everything is mid-grey on dark grey. → §4.5.
4. **The crystals do not glow, they are merely a light purple.** → §4.6, §7.3.

Nothing on that list is "better lighting". Three of the four are free once the pipeline exists.

---

## 2. Tier 0 — the enabler

### 2.1 Do we take a dependency? A specific answer

| Option | Licence | Binary cost | Source cost | Verdict |
|---|---|---|---|---|
| GLEW | Modified BSD / MIT | ~200 KB static lib, or a DLL | pre-built binary or a CMake build | **Refuse.** A binary artefact in a repo whose whole thesis is readable source. |
| GLAD (generated) | tool MIT; **generated output is public domain / CC0 / WTFPL** | 0 (it is `.c`) | ~1,800–2,400 lines of generated `glad.c` + `glad.h` for `gl:core=3.3` | **Refuse, but note it is defensible.** It is checked-in source with no attribution obligation. If our loader list ever exceeds ~80 entry points, revisit. At 42 it is a 6:1 code-size loss. ([licence](https://github.com/Dav1dde/glad/blob/master/LICENSE)) |
| **Khronos `GL/glext.h` + `GL/wglext.h`** | **`SPDX-License-Identifier: MIT`** | **0 bytes** — tokens and typedefs only, no code | 2 files, dropped in `src/game3d/GL/` | **TAKE THESE.** ([glext.h](https://registry.khronos.org/OpenGL/api/GL/glext.h), [wglext.h](https://registry.khronos.org/OpenGL/api/GL/wglext.h)) |
| Hand-written token/typedef subset | ours | 0 | ~90 lines | Acceptable fallback; §2.3 gives it in full. |

**Recommendation: vendor the two Khronos headers and hand-write only the loader.** The
argument is precise: those headers contribute *no executable code whatsoever*. They are the
authoritative transcription of ~700 hex constants, and we would otherwise be transcribing 60
of them by hand from memory or from blog posts. `GL_R11F_G11F_B10F` is `0x8C3A`; being wrong
about that produces a black screen and no error. The headers eliminate the entire bug class
for a two-line `NOTICE.md` entry. Vendoring them is strictly *less* dependency than typing
their contents into our own file.

If policy still says no: §2.3 has the hand-written subset. It is 90 lines and it is fine. Just
diff it against the registry once.

MSVC's own `<GL/gl.h>` is **OpenGL 1.1 only** and the Windows SDK ships **no** `glext.h`. There
is no third option where the tokens come from Microsoft.

### 2.2 The context dance, in full

Two facts drive the whole ritual:

- `SetPixelFormat` may be called **exactly once per window**, ever
  ([MSDN/Khronos](https://wikis.khronos.org/opengl/Creating_an_OpenGL_Context_(WGL))). So you
  cannot pick a legacy format, discover `wglChoosePixelFormatARB`, and then re-pick on the
  same `HWND`.
- `wglGetProcAddress` **requires a current context** and returns `NULL` when there is none. So
  you cannot query the function that creates the good context without first having a bad one.

Hence: dummy window → dummy format → dummy context → query the two `wgl*ARB` functions → real
window → ARB format → ARB context → destroy dummy.

```cpp
// gl3_context.cpp -- creates a 3.3 core context. Depends on nothing but the Windows SDK.

// --- WGL tokens (present in wglext.h; listed here so this block is self-contained) -------
#define WGL_CONTEXT_MAJOR_VERSION_ARB             0x2091
#define WGL_CONTEXT_MINOR_VERSION_ARB             0x2092
#define WGL_CONTEXT_FLAGS_ARB                     0x2094
#define WGL_CONTEXT_PROFILE_MASK_ARB              0x9126
#define WGL_CONTEXT_DEBUG_BIT_ARB                 0x00000001
#define WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB    0x00000002
#define WGL_CONTEXT_CORE_PROFILE_BIT_ARB          0x00000001
#define WGL_DRAW_TO_WINDOW_ARB                    0x2001
#define WGL_ACCELERATION_ARB                      0x2003
#define WGL_SUPPORT_OPENGL_ARB                    0x2010
#define WGL_DOUBLE_BUFFER_ARB                     0x2011
#define WGL_PIXEL_TYPE_ARB                        0x2013
#define WGL_COLOR_BITS_ARB                        0x2014
#define WGL_ALPHA_BITS_ARB                        0x201B
#define WGL_DEPTH_BITS_ARB                        0x2022
#define WGL_STENCIL_BITS_ARB                      0x2023
#define WGL_FULL_ACCELERATION_ARB                 0x2027
#define WGL_TYPE_RGBA_ARB                         0x202B
#define WGL_SAMPLE_BUFFERS_ARB                    0x2041
#define WGL_SAMPLES_ARB                           0x2042
#define WGL_FRAMEBUFFER_SRGB_CAPABLE_ARB          0x20A9

// NOTE THE WINAPI. On x86 this is __stdcall and it is NOT optional. See section 2.4.
typedef HGLRC (WINAPI *PFNWGLCREATECONTEXTATTRIBSARBPROC)(HDC, HGLRC, const int*);
typedef BOOL  (WINAPI *PFNWGLCHOOSEPIXELFORMATARBPROC)(HDC, const int*, const FLOAT*,
                                                       UINT, int*, UINT*);
typedef BOOL  (WINAPI *PFNWGLSWAPINTERVALEXTPROC)(int);

static PFNWGLCREATECONTEXTATTRIBSARBPROC wglCreateContextAttribsARB = nullptr;
static PFNWGLCHOOSEPIXELFORMATARBPROC    wglChoosePixelFormatARB    = nullptr;
static PFNWGLSWAPINTERVALEXTPROC         wglSwapIntervalEXT         = nullptr;

// Step 1: a throwaway window whose only job is to hold a legacy context long enough for
// wglGetProcAddress to answer. It is never shown and never painted.
static bool LoadWglExtensions(HINSTANCE inst)
{
    WNDCLASSA wc{};
    wc.lpfnWndProc   = ::DefWindowProcA;
    wc.hInstance     = inst;
    wc.lpszClassName = "DeepCoreGLBootstrap";
    wc.style         = CS_OWNDC;          // required: we keep the DC
    if (!::RegisterClassA(&wc)) return false;

    HWND wnd = ::CreateWindowA(wc.lpszClassName, "", 0, 0, 0, 1, 1,
                               nullptr, nullptr, inst, nullptr);
    if (!wnd) return false;
    HDC dc = ::GetDC(wnd);

    PIXELFORMATDESCRIPTOR pfd{};
    pfd.nSize      = sizeof(pfd);
    pfd.nVersion   = 1;
    pfd.dwFlags    = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 32;
    pfd.cDepthBits = 24;
    const int pf = ::ChoosePixelFormat(dc, &pfd);
    if (!pf || !::SetPixelFormat(dc, pf, &pfd)) return false;

    HGLRC rc = ::wglCreateContext(dc);
    if (!rc) return false;
    if (!::wglMakeCurrent(dc, rc)) return false;

    // Step 2: the only two functions we actually need from this context.
    wglCreateContextAttribsARB = reinterpret_cast<PFNWGLCREATECONTEXTATTRIBSARBPROC>(
        ::wglGetProcAddress("wglCreateContextAttribsARB"));
    wglChoosePixelFormatARB = reinterpret_cast<PFNWGLCHOOSEPIXELFORMATARBPROC>(
        ::wglGetProcAddress("wglChoosePixelFormatARB"));
    wglSwapIntervalEXT = reinterpret_cast<PFNWGLSWAPINTERVALEXTPROC>(
        ::wglGetProcAddress("wglSwapIntervalEXT"));   // may legitimately be null

    // Step 3: tear the scaffolding down completely. The function pointers stay valid: they
    // belong to the ICD, not to this context. (Formally the spec permits per-context
    // pointers; in 25 years no Windows ICD has done so. If you are nervous, keep the dummy
    // context alive until the real one is current -- it costs one HWND.)
    ::wglMakeCurrent(nullptr, nullptr);
    ::wglDeleteContext(rc);
    ::ReleaseDC(wnd, dc);
    ::DestroyWindow(wnd);
    ::UnregisterClassA(wc.lpszClassName, inst);

    return wglCreateContextAttribsARB != nullptr && wglChoosePixelFormatARB != nullptr;
}

// Step 4: the real window. NOTE: this HWND has never had SetPixelFormat called on it.
bool CreateCoreContext(HINSTANCE inst, HWND wnd, HDC* outDC, HGLRC* outRC, bool debugBuild)
{
    if (!LoadWglExtensions(inst)) return false;

    HDC dc = ::GetDC(wnd);

    const int pixelAttribs[] = {
        WGL_DRAW_TO_WINDOW_ARB,           GL_TRUE,
        WGL_SUPPORT_OPENGL_ARB,           GL_TRUE,
        WGL_DOUBLE_BUFFER_ARB,            GL_TRUE,
        WGL_ACCELERATION_ARB,             WGL_FULL_ACCELERATION_ARB,
        WGL_PIXEL_TYPE_ARB,               WGL_TYPE_RGBA_ARB,
        WGL_COLOR_BITS_ARB,               32,
        WGL_ALPHA_BITS_ARB,               8,
        WGL_DEPTH_BITS_ARB,               24,
        WGL_STENCIL_BITS_ARB,             8,
        // Do NOT ask for MSAA here. We render to an offscreen FBO (section 7.1), so
        // multisampling the default framebuffer buys nothing and can cost a format.
        WGL_SAMPLE_BUFFERS_ARB,           GL_FALSE,
        // Do NOT ask for an sRGB default framebuffer either: request it and some drivers
        // hand back a format with no depth. We do the sRGB encode ourselves in the final
        // post pass (section 7.4), which is portable and free.
        0
    };
    int  formatID  = 0;
    UINT numFormats = 0;
    if (!wglChoosePixelFormatARB(dc, pixelAttribs, nullptr, 1, &formatID, &numFormats)
        || numFormats == 0) return false;

    PIXELFORMATDESCRIPTOR pfd{};
    ::DescribePixelFormat(dc, formatID, sizeof(pfd), &pfd);
    if (!::SetPixelFormat(dc, formatID, &pfd)) return false;   // the one and only time

    int flags = WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB;
    if (debugBuild) flags |= WGL_CONTEXT_DEBUG_BIT_ARB;

    const int ctxAttribs[] = {
        WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
        WGL_CONTEXT_MINOR_VERSION_ARB, 3,
        WGL_CONTEXT_PROFILE_MASK_ARB,  WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        WGL_CONTEXT_FLAGS_ARB,         flags,
        0
    };
    HGLRC rc = wglCreateContextAttribsARB(dc, nullptr, ctxAttribs);
    if (!rc) return false;
    if (!::wglMakeCurrent(dc, rc)) return false;

    if (wglSwapIntervalEXT) wglSwapIntervalEXT(1);   // vsync; replaces the ::Sleep(8) at :717

    *outDC = dc; *outRC = rc;
    return true;
}
```

Five things that are easy to get wrong and are correct above:

1. `CS_OWNDC` on **both** window classes. The existing code already has it at `:616`; keep it.
   Without it the `HDC` is recycled and the pixel format assignment can be lost.
2. The dummy window is 1×1, never `ShowWindow`n, and is `UnregisterClass`ed. Leaving the class
   registered is harmless but leaks a class atom on repeated init.
3. `WGL_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB` is requested. This is a *deliberate* choice: it
   removes every deprecated entry point from the context, so any surviving `glBegin` becomes a
   hard `GL_INVALID_OPERATION` at the call site instead of silently working on NVIDIA and
   silently failing on Intel. It converts a portability bug into a compile-and-run bug. Turn
   it off only if it costs you a format.
4. Anisotropic filtering (§6.5) and MSAA are *not* requested at format time.
5. `wglSwapIntervalEXT(1)` replaces `::Sleep(8)` at `:717`. `Sleep(8)` gives you a jittering
   ~110 fps cap with no relation to the display; vsync gives you a stable frame time, which is
   itself a large part of "feels finished".

### 2.3 The loader: exactly which 42 functions, and why

**The rule that trips everyone**: on the Microsoft ICD, `wglGetProcAddress` returns `NULL` for
**OpenGL 1.1 functions** — they are already exported from `opengl32.dll` and already linked by
`#pragma comment(lib, "opengl32.lib")`. Do not try to load `glDrawArrays`, `glDrawElements`,
`glGenTextures`, `glBindTexture`, `glTexImage2D`, `glTexParameteri`, `glViewport`, `glClear`,
`glEnable`, `glDepthFunc`, `glCullFace`, `glBlendFunc`, `glReadPixels`, `glDeleteTextures`, or
`glGetIntegerv`. You already have them.

And the reciprocal rule: some drivers return `1`, `2`, `3`, or `-1` instead of `NULL` on
failure, so a plain null check is insufficient
([Khronos WGL wiki](https://wikis.khronos.org/opengl/Creating_an_OpenGL_Context_(WGL))).

```cpp
// gl3_loader.h ------------------------------------------------------------------------
// APIENTRY expands to __stdcall on Win32. It is on every typedef for a reason: see 2.4.
typedef ptrdiff_t GLsizeiptr;   // (glext.h defines these; here for a self-contained sketch)
typedef ptrdiff_t GLintptr;
typedef char      GLchar;

#define GL_FUNCS(X)                                                                    \
    /* ---- VAO / VBO : 11 ---------------------------------------------------- */     \
    X(void,   glGenVertexArrays,        (GLsizei, GLuint*))                             \
    X(void,   glBindVertexArray,        (GLuint))                                       \
    X(void,   glDeleteVertexArrays,     (GLsizei, const GLuint*))                       \
    X(void,   glGenBuffers,             (GLsizei, GLuint*))                             \
    X(void,   glBindBuffer,             (GLenum, GLuint))                               \
    X(void,   glBufferData,             (GLenum, GLsizeiptr, const void*, GLenum))      \
    X(void,   glBufferSubData,          (GLenum, GLintptr, GLsizeiptr, const void*))    \
    X(void,   glDeleteBuffers,          (GLsizei, const GLuint*))                       \
    X(void,   glEnableVertexAttribArray,(GLuint))                                       \
    X(void,   glVertexAttribPointer,    (GLuint, GLint, GLenum, GLboolean, GLsizei,     \
                                         const void*))                                  \
    X(void,   glVertexAttribIPointer,   (GLuint, GLint, GLenum, GLsizei, const void*))  \
    /* ---- Shaders : 14 ------------------------------------------------------- */     \
    X(GLuint, glCreateShader,           (GLenum))                                       \
    X(void,   glShaderSource,           (GLuint, GLsizei, const GLchar* const*,         \
                                         const GLint*))                                 \
    X(void,   glCompileShader,          (GLuint))                                       \
    X(void,   glGetShaderiv,            (GLuint, GLenum, GLint*))                       \
    X(void,   glGetShaderInfoLog,       (GLuint, GLsizei, GLsizei*, GLchar*))           \
    X(void,   glDeleteShader,           (GLuint))                                       \
    X(GLuint, glCreateProgram,          (void))                                         \
    X(void,   glAttachShader,           (GLuint, GLuint))                               \
    X(void,   glLinkProgram,            (GLuint))                                       \
    X(void,   glGetProgramiv,           (GLuint, GLenum, GLint*))                       \
    X(void,   glGetProgramInfoLog,      (GLuint, GLsizei, GLsizei*, GLchar*))           \
    X(void,   glUseProgram,             (GLuint))                                       \
    X(void,   glDeleteProgram,          (GLuint))                                       \
    X(void,   glDetachShader,           (GLuint, GLuint))                               \
    /* ---- Uniforms : 8 ------------------------------------------------------- */     \
    X(GLint,  glGetUniformLocation,     (GLuint, const GLchar*))                        \
    X(void,   glUniform1i,              (GLint, GLint))                                 \
    X(void,   glUniform1f,              (GLint, GLfloat))                               \
    X(void,   glUniform2f,              (GLint, GLfloat, GLfloat))                      \
    X(void,   glUniform3f,              (GLint, GLfloat, GLfloat, GLfloat))             \
    X(void,   glUniform4f,              (GLint, GLfloat, GLfloat, GLfloat, GLfloat))    \
    X(void,   glUniform3fv,             (GLint, GLsizei, const GLfloat*))               \
    X(void,   glUniformMatrix4fv,       (GLint, GLsizei, GLboolean, const GLfloat*))    \
    /* ---- Textures beyond 1.1 : 2 ------------------------------------------- */     \
    X(void,   glActiveTexture,          (GLenum))            /* GL 1.3 */               \
    X(void,   glGenerateMipmap,         (GLenum))            /* GL 3.0 */               \
    /* ---- FBO / RBO : 10 ----------------------------------------------------- */     \
    X(void,   glGenFramebuffers,        (GLsizei, GLuint*))                             \
    X(void,   glBindFramebuffer,        (GLenum, GLuint))                               \
    X(void,   glFramebufferTexture2D,   (GLenum, GLenum, GLenum, GLuint, GLint))        \
    X(GLenum, glCheckFramebufferStatus, (GLenum))                                       \
    X(void,   glDeleteFramebuffers,     (GLsizei, const GLuint*))                       \
    X(void,   glGenRenderbuffers,       (GLsizei, GLuint*))                             \
    X(void,   glBindRenderbuffer,       (GLenum, GLuint))                               \
    X(void,   glRenderbufferStorage,    (GLenum, GLenum, GLsizei, GLsizei))             \
    X(void,   glFramebufferRenderbuffer,(GLenum, GLenum, GLenum, GLuint))               \
    X(void,   glDrawBuffers,            (GLsizei, const GLenum*))                       \
    /* ---- Optional but cheap : 4 --------------------------------------------- */     \
    X(void,   glRenderbufferStorageMultisample, (GLenum, GLsizei, GLenum, GLsizei,      \
                                                 GLsizei))   /* MSAA, 7.1 */            \
    X(void,   glBlitFramebuffer,        (GLint,GLint,GLint,GLint, GLint,GLint,GLint,    \
                                         GLint, GLbitfield, GLenum))                    \
    X(const GLubyte*, glGetStringi,     (GLenum, GLuint))    /* extension enumeration */ \
    X(void,   glDeleteRenderbuffers,    (GLsizei, const GLuint*))

// Declare a typedef + a global for every entry, and #define the bare name onto the pointer
// so call sites read exactly like normal GL.
#define GL_DECL(ret, name, args)  typedef ret (APIENTRY *PFN_##name) args; \
                                  extern PFN_##name p_##name;
GL_FUNCS(GL_DECL)
#undef GL_DECL

#define glGenVertexArrays  p_glGenVertexArrays
// ... one #define per entry; generate this block with the same X-macro if you prefer:
//   #define GL_ALIAS(ret,name,args) ... -- but the preprocessor cannot #define from a macro,
//   so this list is written out once, by hand, 42 lines. It is boring and it is correct.
```

```cpp
// gl3_loader.cpp ----------------------------------------------------------------------
#define GL_DEF(ret, name, args)  PFN_##name p_##name = nullptr;
GL_FUNCS(GL_DEF)
#undef GL_DEF

static HMODULE s_opengl32 = nullptr;

// The robust resolver. wglGetProcAddress is documented to return NULL on failure but real
// drivers have returned 1, 2, 3 and -1; and it never returns GL 1.1 entry points, which live
// in opengl32.dll. Check both.
static PROC GLResolve(const char* name)
{
    PROC p = ::wglGetProcAddress(name);
    if (p == nullptr   || p == reinterpret_cast<PROC>(1) ||
        p == reinterpret_cast<PROC>(2) || p == reinterpret_cast<PROC>(3) ||
        p == reinterpret_cast<PROC>(-1))
    {
        if (!s_opengl32) s_opengl32 = ::LoadLibraryA("opengl32.dll");
        p = s_opengl32 ? ::GetProcAddress(s_opengl32, name) : nullptr;
    }
    return p;
}

// Returns the name of the first function that failed to load, or nullptr on success.
// Fail loud, fail here, with a name: a null function pointer discovered 200 lines later is
// an access violation with no context.
const char* GL3_LoadAll()
{
    // MUST be called with the REAL 3.3 core context already current.
#define GL_LOAD(ret, name, args)                                                       \
    p_##name = reinterpret_cast<PFN_##name>(GLResolve(#name));                         \
    if (!p_##name) return #name;
    GL_FUNCS(GL_LOAD)
#undef GL_LOAD
    return nullptr;
}
```

Call site:

```cpp
if (const char* missing = GL3_LoadAll()) {
    char msg[256];
    std::snprintf(msg, sizeof(msg),
        "This machine's OpenGL driver does not provide %s.\n"
        "DeepCore 3D needs OpenGL 3.3. Update your graphics driver.", missing);
    ::MessageBoxA(nullptr, msg, "DeepCore 3D", MB_ICONERROR);
    return 1;
}
```

**Count: 11 (VAO/VBO) + 14 (shaders) + 8 (uniforms) + 2 (textures) + 10 (FBO) + 4 (optional)
= 49 declared, of which 42 are on the critical path.** `glRenderbufferStorageMultisample`,
`glBlitFramebuffer`, `glGetStringi`, `glDetachShader` and the two `glDelete*` cleanup entries
are the ones you can drop for a first cut.

Deliberately **not** loaded, with reasons:

| Not loaded | Why |
|---|---|
| `glDrawArrays`, `glDrawElements`, `glGenTextures`, `glBindTexture`, `glTexImage2D`, `glTexParameteri/fv`, `glViewport`, `glClear`, `glClearColor`, `glEnable/Disable`, `glDepthFunc`, `glCullFace`, `glFrontFace`, `glBlendFunc`, `glGetIntegerv`, `glGetString`, `glDeleteTextures`, `glPixelStorei` | GL 1.1 — already exported by `opengl32.dll` and already linked |
| `glBindAttribLocation`, `glBindFragDataLocation` | unnecessary: GLSL 330 has `layout(location = N)` on both `in` attributes and `out` fragment targets (ARB_explicit_attrib_location is core in 3.3) |
| `glDrawElementsInstanced`, `glVertexAttribDivisor` | worth two more entries **later** if we ever have hundreds of figures. At ~10 figures × 6-7 cubes it saves nothing |
| `glMapBuffer` / `glMapBufferRange` | `glBufferSubData` is sufficient: our largest upload is 210 KB and happens only when a wall is drilled |
| `glUniformBlockBinding`, UBOs | 42 uniforms across 5 programs. Not worth the alignment rules |

### 2.4 The x86 trap — read this before writing anything

`deepcore3d.vcxproj` builds **Win32 (x86) only** (`ProjectConfiguration Include="Debug|Win32"`).
On x86 Windows, every GL entry point is `__stdcall` — that is what `APIENTRY` and `WINAPI`
expand to. On x64 there is only one calling convention, so `APIENTRY` expands to nothing.

Consequence: **a function-pointer typedef that omits `APIENTRY` compiles clean at `/W4`, works
perfectly on an x64 build, and corrupts the stack on x86.** The callee pops the arguments, the
caller pops them again, and you get a crash somewhere else entirely — typically several
function calls later, in unrelated code, with a call stack that makes no sense. Every typedef
in §2.2 and §2.3 has `WINAPI` / `APIENTRY` on it. Keep it there. This is the single most
expensive mistake available in this whole document.

The second-order version of the same trap: `wglGetProcAddress` returns `PROC`, which is
`int (WINAPI*)()`. Casting `PROC` to a differently-shaped function pointer is what
`reinterpret_cast` is for; **do not launder it through `void*`** (that conversion between
object and function pointer types is conditionally-supported in C++ and MSVC will warn under
some settings). `reinterpret_cast<PFN_x>(::wglGetProcAddress(...))` is the correct form and is
`/W4`-clean. If your build ever enables the off-by-default `C4191`, suppress it locally around
the loader with `#pragma warning(push/disable:4191/pop)` and a comment pointing here.

### 2.5 KHR_debug: the GPU debugger the brief says we do not have

The brief says "we cannot use a GPU debugger". We can have 80% of one for 35 lines. With the
debug bit set on the context (§2.2), `glDebugMessageCallback` delivers driver-side errors,
performance warnings, and shader recompilation notices to a C callback, **with the offending
call still on the stack** when `GL_DEBUG_OUTPUT_SYNCHRONOUS` is enabled. That means a breakpoint
in the callback lands you on the exact GL call that was wrong.

This is core in 4.3 and available as `KHR_debug` on essentially every 3.3-capable driver
shipped since 2012, so query it rather than assume it.

```cpp
#define GL_DEBUG_OUTPUT              0x92E0
#define GL_DEBUG_OUTPUT_SYNCHRONOUS  0x8242
#define GL_DEBUG_SEVERITY_HIGH       0x9146
#define GL_DEBUG_SEVERITY_NOTIFICATION 0x826B
#define GL_DONT_CARE                 0x1100

typedef void (APIENTRY *GLDEBUGPROC)(GLenum source, GLenum type, GLuint id, GLenum severity,
                                     GLsizei length, const GLchar* message,
                                     const void* userParam);
typedef void (APIENTRY *PFN_glDebugMessageCallback)(GLDEBUGPROC, const void*);
typedef void (APIENTRY *PFN_glDebugMessageControl)(GLenum, GLenum, GLenum, GLsizei,
                                                   const GLuint*, GLboolean);

static void APIENTRY GLDebugCB(GLenum, GLenum type, GLuint id, GLenum severity,
                               GLsizei, const GLchar* msg, const void*)
{
    if (severity == GL_DEBUG_SEVERITY_NOTIFICATION) return;
    char line[1200];
    std::snprintf(line, sizeof(line), "[GL] type=0x%04X id=%u sev=0x%04X: %s\n",
                  type, id, severity, msg);
    ::OutputDebugStringA(line);
    if (severity == GL_DEBUG_SEVERITY_HIGH) ::DebugBreak();   // stop ON the bad call
}

void GL3_InstallDebugCallback()
{
    auto cb  = reinterpret_cast<PFN_glDebugMessageCallback>(
                   GLResolve("glDebugMessageCallback"));
    auto ctl = reinterpret_cast<PFN_glDebugMessageControl>(
                   GLResolve("glDebugMessageControl"));
    if (!cb) return;                                    // fine, just no diagnostics
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);              // required for the stack to be right
    cb(&GLDebugCB, nullptr);
    if (ctl) ctl(GL_DONT_CARE, GL_DONT_CARE, GL_DONT_CARE, 0, nullptr, GL_TRUE);
}
```

Cost: 35 lines and one afternoon saved the first time an FBO is incomplete.

### 2.6 Replacing GLU — the matrix library

A core context has no `glMatrixMode`, no `gluPerspective`, no `gluLookAt`, no `gluUnProject`,
and we should drop `glu32.lib` entirely. Column-major, to match GLSL's `mat4` and
`glUniformMatrix4fv(..., GL_FALSE, ...)`.

```cpp
struct Mat4 { float m[16]; };   // column-major: m[c*4 + r]

static Mat4 M4Identity()
{
    Mat4 r{}; r.m[0] = r.m[5] = r.m[10] = r.m[15] = 1.0f; return r;
}

static Mat4 M4Mul(const Mat4& a, const Mat4& b)   // returns a*b
{
    Mat4 r{};
    for (int c = 0; c < 4; ++c)
        for (int i = 0; i < 4; ++i) {
            float s = 0.0f;
            for (int k = 0; k < 4; ++k) s += a.m[k * 4 + i] * b.m[c * 4 + k];
            r.m[c * 4 + i] = s;
        }
    return r;
}

// Exactly gluPerspective, with fovy in DEGREES to keep the call site identical to :686.
static Mat4 M4Perspective(float fovyDeg, float aspect, float zn, float zf)
{
    const float f = 1.0f / std::tan(fovyDeg * 3.14159265f / 360.0f);
    Mat4 r{};
    r.m[0]  = f / aspect;
    r.m[5]  = f;
    r.m[10] = (zf + zn) / (zn - zf);
    r.m[11] = -1.0f;
    r.m[14] = (2.0f * zf * zn) / (zn - zf);
    return r;
}

static Mat4 M4Ortho(float l, float r_, float b, float t, float zn, float zf)
{
    Mat4 r{};
    r.m[0]  =  2.0f / (r_ - l);
    r.m[5]  =  2.0f / (t - b);
    r.m[10] = -2.0f / (zf - zn);
    r.m[12] = -(r_ + l) / (r_ - l);
    r.m[13] = -(t + b) / (t - b);
    r.m[14] = -(zf + zn) / (zf - zn);
    r.m[15] =  1.0f;
    return r;
}

static void V3Norm(float v[3])
{
    const float n = std::sqrt(v[0]*v[0] + v[1]*v[1] + v[2]*v[2]);
    if (n > 1e-8f) { v[0] /= n; v[1] /= n; v[2] /= n; }
}
static void V3Cross(const float a[3], const float b[3], float o[3])
{
    o[0] = a[1]*b[2] - a[2]*b[1];
    o[1] = a[2]*b[0] - a[0]*b[2];
    o[2] = a[0]*b[1] - a[1]*b[0];
}

// Exactly gluLookAt.
static Mat4 M4LookAt(const float eye[3], const float ctr[3], const float up[3])
{
    float f[3] = { ctr[0]-eye[0], ctr[1]-eye[1], ctr[2]-eye[2] };  V3Norm(f);
    float u[3] = { up[0], up[1], up[2] };                          V3Norm(u);
    float s[3]; V3Cross(f, u, s);                                  V3Norm(s);
    float uu[3]; V3Cross(s, f, uu);

    Mat4 r = M4Identity();
    r.m[0] = s[0];  r.m[4] = s[1];  r.m[8]  = s[2];
    r.m[1] = uu[0]; r.m[5] = uu[1]; r.m[9]  = uu[2];
    r.m[2] = -f[0]; r.m[6] = -f[1]; r.m[10] = -f[2];
    r.m[12] = -(s[0]*eye[0]  + s[1]*eye[1]  + s[2]*eye[2]);
    r.m[13] = -(uu[0]*eye[0] + uu[1]*eye[1] + uu[2]*eye[2]);
    r.m[14] =  (f[0]*eye[0]  + f[1]*eye[1]  + f[2]*eye[2]);
    return r;
}

// The normal matrix for a rotation+uniform-scale model matrix is just the upper-left 3x3;
// we never non-uniformly scale terrain, so pass mat3(model) and skip the inverse-transpose.
```

That is 90 lines and it replaces `glu32` for rendering. `gluUnProject` is replaced by
something better, next.

### 2.7 Picking without `gluUnProject`, without a GPU stall

Today's `PickTile` (`:510-527`) reads the depth buffer. Kill it. Our world is a heightfield on
a 1×1 lattice, so a ray/grid DDA (Amanatides–Woo) gives the *exact* tile, works while paused,
works when the depth buffer is stale, does not sync the GPU, and does not care what the
renderer is doing.

```cpp
// Screen pixel -> world ray. invVP = inverse(proj * view), computed once per frame.
static void ScreenRay(int mx, int my, int W, int H, const Mat4& invVP,
                      const float eye[3], float outDir[3])
{
    const float ndcX =  (2.0f * (float)mx / (float)W) - 1.0f;
    const float ndcY =  1.0f - (2.0f * (float)my / (float)H);   // Win32 y is top-down
    // Unproject the far plane point (z_ndc = +1).
    float p[4] = { ndcX, ndcY, 1.0f, 1.0f }, w[4] = {0,0,0,0};
    for (int i = 0; i < 4; ++i)
        w[i] = invVP.m[0*4+i]*p[0] + invVP.m[1*4+i]*p[1]
             + invVP.m[2*4+i]*p[2] + invVP.m[3*4+i]*p[3];
    outDir[0] = w[0]/w[3] - eye[0];
    outDir[1] = w[1]/w[3] - eye[1];
    outDir[2] = w[2]/w[3] - eye[2];
    V3Norm(outDir);
}

// Walk the ray across the tile grid. Returns the first tile whose column the ray enters,
// treating solids as full-height [0, WALL_H] boxes and open tiles as the plane y = 0.
static bool PickTileDDA(const Game& g, const float o[3], const float d[3],
                        int& outX, int& outZ)
{
    const float WALL_H = 1.24f;             // matches Cube(): y1 = cy + sy*2, sy = 0.62
    // 1. If the ray is going down, the floor plane y=0 gives an upper bound on t.
    float tFloor = 1e30f;
    if (d[1] < -1e-6f) tFloor = (0.0f - o[1]) / d[1];

    int   x  = (int)std::floor(o[0] + 0.5f);
    int   z  = (int)std::floor(o[2] + 0.5f);
    const int stepX = d[0] > 0 ? 1 : -1;
    const int stepZ = d[2] > 0 ? 1 : -1;
    const float invX = (std::fabs(d[0]) < 1e-8f) ? 1e30f : 1.0f / d[0];
    const float invZ = (std::fabs(d[2]) < 1e-8f) ? 1e30f : 1.0f / d[2];
    // Distance to the next x / z cell boundary (cells are centred on integers, so the
    // boundary nearest in the step direction is at x + stepX*0.5).
    float tMaxX = (( (float)x + stepX * 0.5f) - o[0]) * invX;
    float tMaxZ = (( (float)z + stepZ * 0.5f) - o[2]) * invZ;
    const float tDeltaX = std::fabs(invX);
    const float tDeltaZ = std::fabs(invZ);

    for (int guard = 0; guard < 256; ++guard) {
        if (!g.level.InBounds(x, z)) return false;
        const float t = (tMaxX < tMaxZ) ? tMaxX : tMaxZ;     // t at which we LEAVE this cell
        if (g.Solid(x, z)) {
            // Did the ray pass below the top of this column while inside it?
            const float tEnter = t - ((tMaxX < tMaxZ) ? tDeltaX : tDeltaZ);
            const float yAt    = o[1] + d[1] * (tEnter > 0.0f ? tEnter : 0.0f);
            if (yAt <= WALL_H) { outX = x; outZ = z; return true; }
        } else if (tFloor <= t && tFloor >= 0.0f) {
            outX = (int)std::floor(o[0] + d[0]*tFloor + 0.5f);
            outZ = (int)std::floor(o[2] + d[2]*tFloor + 0.5f);
            return g.level.InBounds(outX, outZ);
        }
        if (tMaxX < tMaxZ) { tMaxX += tDeltaX; x += stepX; }
        else               { tMaxZ += tDeltaZ; z += stepZ; }
    }
    return false;
}
```

~60 lines, removes `glu32.lib`, removes a pipeline stall from `WndProc`, and picks correctly
through the fog. Reference for the traversal: Amanatides & Woo, *A Fast Voxel Traversal
Algorithm for Ray Tracing* (1987), https://www.cse.chalmers.se/edu/year/2010/course/TDA361/grid.pdf

### 2.8 The HUD: replacing GDI text with a baked font atlas

`DrawText2D` (`:603`) uses `TextOutA` on the GL `HDC`. That cannot survive a core context, and
`wglUseFontBitmaps` is a display-list API (compatibility only). The zero-dependency answer is
to rasterise the font **once, at startup, with GDI, into a DIB** and upload it as a texture.
GDI is a Windows SDK component, so this adds nothing.

```cpp
struct FontAtlas {
    GLuint tex = 0;
    int    cellW = 0, cellH = 0;     // fixed cell; we use Consolas, which is monospaced
    int    texW = 0, texH = 0;
};

static bool BakeFontAtlas(FontAtlas& fa, const char* face, int pxHeight)
{
    HDC memDC = ::CreateCompatibleDC(nullptr);
    HFONT font = ::CreateFontA(pxHeight, 0, 0, 0, FW_SEMIBOLD, 0, 0, 0, ANSI_CHARSET,
        OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, FF_DONTCARE, face);
    HGDIOBJ oldFont = ::SelectObject(memDC, font);

    TEXTMETRICA tm{};
    ::GetTextMetricsA(memDC, &tm);
    fa.cellW = tm.tmMaxCharWidth;
    fa.cellH = tm.tmHeight;
    fa.texW  = fa.cellW * 16;      // 16x6 grid covers ASCII 32..126 with room to spare
    fa.texH  = fa.cellH * 6;

    BITMAPINFO bi{};
    bi.bmiHeader.biSize        = sizeof(bi.bmiHeader);
    bi.bmiHeader.biWidth       = fa.texW;
    bi.bmiHeader.biHeight      = -fa.texH;     // negative = top-down, matches GL if we flip V
    bi.bmiHeader.biPlanes      = 1;
    bi.bmiHeader.biBitCount    = 32;
    bi.bmiHeader.biCompression = BI_RGB;
    void* bits = nullptr;
    HBITMAP dib = ::CreateDIBSection(memDC, &bi, DIB_RGB_COLORS, &bits, nullptr, 0);
    HGDIOBJ oldBmp = ::SelectObject(memDC, dib);

    ::PatBlt(memDC, 0, 0, fa.texW, fa.texH, BLACKNESS);
    ::SetBkMode(memDC, TRANSPARENT);
    ::SetTextColor(memDC, RGB(255, 255, 255));
    for (int c = 32; c < 127; ++c) {
        const int idx = c - 32;
        const char ch = (char)c;
        ::TextOutA(memDC, (idx % 16) * fa.cellW, (idx / 16) * fa.cellH, &ch, 1);
    }
    ::GdiFlush();

    // GDI gave us white-on-black BGRA. Collapse to a single-channel coverage texture:
    // GL_ALPHA does not exist in 3.3 core, so use GL_R8 and sample .r in the shader.
    std::vector<unsigned char> a((size_t)fa.texW * fa.texH);
    const unsigned char* src = static_cast<const unsigned char*>(bits);
    for (size_t i = 0; i < a.size(); ++i) a[i] = src[i * 4];   // B channel == coverage

    glGenTextures(1, &fa.tex);
    glBindTexture(GL_TEXTURE_2D, fa.tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);                     // rows are not 4-aligned
    glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, fa.texW, fa.texH, 0,
                 GL_RED, GL_UNSIGNED_BYTE, a.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    ::SelectObject(memDC, oldBmp); ::DeleteObject(dib);
    ::SelectObject(memDC, oldFont); ::DeleteObject(font);
    ::DeleteDC(memDC);
    return true;
}
```

Text then draws as one dynamic VBO of quads with an orthographic projection, alpha blending,
and depth test off — roughly 60 more lines. `GL_R8` is `0x8229`; the swizzle-free `.r` sample
in the shader is `vec4(tint.rgb, tint.a * texture(uFont, uv).r)`.

Two upgrades that cost almost nothing once this exists and make the HUD look authored rather
than debug-printed: (a) draw each string twice, offset by (1,1) px in near-black, for a drop
shadow; (b) add a `uTime`-driven alpha fade on the banner instead of the hard cutoff at `:711`.

### 2.9 Staying `/W4`-clean, and the MSVC shader-literal trap

Known issues, all avoidable:

| Warning | Where it bites | Fix |
|---|---|---|
| **C2026** *string too big* — **not a warning, a hard error** | A GLSL shader as one `R"(...)"` literal. **MSVC's limit is 16,384 bytes per string literal.** Our post-process shader will approach it. | Split into adjacent literals: `R"(...)" R"(...)"`. Adjacent literals concatenate at translation time with no limit. |
| C4100 unreferenced formal parameter | `GLDebugCB` has 7 parameters, 4 unused | Omit the parameter *names* (as done in §2.5), not `(void)x` |
| C4244 / C4267 conversion, possible loss of data | `size_t` → `GLsizei`, `size_t` → `GLint` everywhere in the mesh builder | Explicit `(GLsizei)` casts; the existing file already does this at `:328`, `:270` |
| C4189 local variable initialised but not referenced | Debug-only FBO status checks | Wrap in `#ifdef _DEBUG` or actually check it |
| C4127 conditional expression is constant | `while(true)` / `do{}while(0)` macros in the loader | `for(;;)`; the X-macro above avoids this |
| C4706 assignment within conditional | `if (const char* m = GL3_LoadAll())` is fine (declaration, not assignment) | — |
| **`#version` must be the first line** | `R"(\n#version 330 core\n...)"` — a leading newline from formatting the raw literal | Start the literal immediately: `R"(#version 330 core` with no preceding newline. Costs an hour the first time. |

The mesh builder should also carry `assert(vertexCount < 65536)` so the `GLushort` index
assumption in §3.2 is checked rather than believed.

---

## 3. The port: terrain from `glBegin` to one VBO

### 3.1 Vertex format

20 bytes, 4-byte aligned, and it holds everything §4 needs:

```cpp
struct TerrainVertex          // 20 bytes
{
    float        x, y, z;     // 12  attribute 0  vec3  GL_FLOAT
    GLbyte       nx, ny, nz;  //  3  attribute 1  vec3  GL_BYTE, normalized -> [-1,1]
    GLbyte       _pad;        //  1
    GLubyte      ao;          //  1  attribute 2  float GL_UNSIGNED_BYTE, normalized
    GLubyte      mat;         //  1  attribute 3  uint  GL_UNSIGNED_BYTE, INTEGER pointer
    GLubyte      _pad2[2];    //  2
};
static_assert(sizeof(TerrainVertex) == 20, "terrain vertex must stay 20 bytes");
```

Face normals are axis-aligned, so `GL_BYTE` with `normalized = GL_TRUE` is exact: ±127/127 = ±1.
The material id indexes a small uniform array of base colours and texture-scale factors, so a
crystal seam and plain rock live in the **same draw call**.

```cpp
glBindVertexArray(vao);
glBindBuffer(GL_ARRAY_BUFFER, vbo);
glEnableVertexAttribArray(0);
glVertexAttribPointer (0, 3, GL_FLOAT,         GL_FALSE, 20, (void*)0);
glEnableVertexAttribArray(1);
glVertexAttribPointer (1, 3, GL_BYTE,          GL_TRUE,  20, (void*)12);
glEnableVertexAttribArray(2);
glVertexAttribPointer (2, 1, GL_UNSIGNED_BYTE, GL_TRUE,  20, (void*)16);
glEnableVertexAttribArray(3);
glVertexAttribIPointer(3, 1, GL_UNSIGNED_BYTE,           20, (void*)17);   // note: I-form
glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo);   // recorded in the VAO; bind it INSIDE the VAO
```

Two things people get wrong here: the material must use `glVertexAttribIPointer` (not the
float form) to reach a `flat in uint` in the vertex shader; and the element buffer binding is
**part of VAO state**, so bind it while the VAO is bound and never again.

### 3.2 Hidden-face culling — the numbers for our 40×40

Our world is a **single-layer heightfield**, not a true 3D voxel volume. That simplifies
everything: there is no "above" neighbour and no "below" neighbour, so the rules are:

```
For each tile (x,z):
  if SOLID:
      emit +Y (top)                             always  -- nothing occupies y > WALL_H
      emit -Y (bottom)                          never   -- unreachable by any legal camera
      emit +X/-X/+Z/-Z                          iff the neighbour is NOT solid
                                                (out of bounds counts as solid -> cull)
  else (floor / water / path / building):
      emit ONE quad at y = 0                    -- not a cube. Five faces vanish per tile.
      emit no sides                             -- the adjacent solid's base covers them
```

Note the second half: today floors are drawn as slabs (`:474`, `sy = 0.06`, so the top sits at
y = −0.02) while solid columns start at y = 0. **There is a 0.02-unit gap between the floor
top and the wall base**, which will show as a hairline crack at grazing angles. Collapsing the
floor to a single quad at exactly y = 0 fixes a latent artefact *and* removes 5 faces per open
tile. Free.

**ESTIMATE of the open/solid split.** The generator (`SyntheticLevel.cpp:50-140`) carves
`caverns = 8` integer discs of radius `r ∈ {2,3,4}` uniform, then L-shaped 1-wide corridors
between consecutive centres, then converts 5% of floor to water and 36 hidden tiles to seams.

- Integer disc areas: r=2 → 13, r=3 → 29, r=4 → 49; mean ≈ 30.3 → 8 × 30.3 = **243** tiles.
- Corridor length: two coordinates each roughly uniform on [0,40]; E|Δ| ≈ 13.3 per axis, so
  ≈ 26.7 tiles per corridor × 7 corridors = **187**, less ≈ 2r consumed inside the rooms at
  each end → ≈ **145** net.
- Room-room overlap at 243/1600 coverage ≈ 8% → **−20**.
- **Open tiles F ≈ 368. Solid tiles S ≈ 1,232.**
- Boundary edges E (solid↔open adjacencies): taxicab perimeter of a radius-r disc ≈ 8r → 24
  per room × 8 = 192; 1-wide corridors expose 2 lateral edges per tile → 145 × 2 = 290; plus
  caps. **E ≈ 500.**

| | Today (COUNTED) | Culled (ESTIMATE) | Ratio |
|---|---:|---:|---:|
| Top faces | 1,637 | 1,232 + 368 = 1,600 | 1.0× |
| Bottom faces | 1,637 | **0** | ∞ |
| Side faces | 6,548 | **~500** | **13.1×** |
| Seam topper cubes (36) | 216 | 180 (bottom culled) | 1.2× |
| **Total quads** | **9,822** | **~2,285** | **4.3×** |
| **Triangles** | 19,644 | **~4,570** | 4.3× |
| Draw calls | **1,637** | **1** | **1,637×** |
| API calls/frame | ~52,000 | **~12** | ~4,300× |
| Vertex bytes | (streamed, 39,288 calls) | 9,140 × 20 = **183 KB static** | — |
| Index bytes (`GLushort`, 9,140 < 65,536) | — | 2,285 × 6 × 2 = **27 KB** | — |

**Replace the ESTIMATE with a measurement in 12 lines** — this is the only number in the
document I would not ship on:

```cpp
// Drop into NewLevel() after level.Generate(). Prints to the debugger output window.
{
    int open = 0, solid = 0, edges = 0;
    for (int z = 0; z < level.Height(); ++z)
        for (int x = 0; x < level.Width(); ++x) {
            const bool s = Solid(x, z);
            s ? ++solid : ++open;
            if (!s) continue;
            static const int DX[4] = {1,-1,0,0}, DZ[4] = {0,0,1,-1};
            for (int d = 0; d < 4; ++d)
                if (level.InBounds(x+DX[d], z+DZ[d]) && !Solid(x+DX[d], z+DZ[d])) ++edges;
        }
    char b[160];
    std::snprintf(b, sizeof(b), "[mesh] open=%d solid=%d exposedFaces=%d culledQuads=%d\n",
                  open, solid, edges, open + solid + edges);
    ::OutputDebugStringA(b);
}
```

The mesh builder itself:

```cpp
enum MatId : GLubyte { MAT_ROCK=0, MAT_ROCK_HIDDEN=1, MAT_ORE=2, MAT_CRYSTAL=3,
                       MAT_FLOOR=4, MAT_PATH=5, MAT_WATER=6, MAT_BUILDING=7 };

struct TerrainMesh {
    std::vector<TerrainVertex> v;
    std::vector<GLushort>      i;
    GLuint vao = 0, vbo = 0, ibo = 0;
    void Clear() { v.clear(); i.clear(); }
    void PushQuad(const float p[4][3], const GLbyte n[3], const GLubyte ao[4], GLubyte mat)
    {
        const GLushort base = (GLushort)v.size();
        for (int k = 0; k < 4; ++k) {
            TerrainVertex t{};
            t.x = p[k][0]; t.y = p[k][1]; t.z = p[k][2];
            t.nx = n[0]; t.ny = n[1]; t.nz = n[2];
            t.ao = ao[k]; t.mat = mat;
            v.push_back(t);
        }
        // Section 4.3: flip the triangulation to follow the AO gradient, or the interpolation
        // produces a visible diagonal seam across every corner.
        if (ao[0] + ao[2] > ao[1] + ao[3]) {
            const GLushort idx[6] = { base, (GLushort)(base+1), (GLushort)(base+2),
                                      base, (GLushort)(base+2), (GLushort)(base+3) };
            i.insert(i.end(), idx, idx + 6);
        } else {
            const GLushort idx[6] = { (GLushort)(base+1), (GLushort)(base+2),
                                      (GLushort)(base+3), (GLushort)(base+1),
                                      (GLushort)(base+3), base };
            i.insert(i.end(), idx, idx + 6);
        }
    }
};
```

**Rebuild policy.** The whole mesh is 1,600 tile visits and ~2,285 quad emissions — order
tens of microseconds. **Do not chunk it.** Minecraft chunks because its world is 10^7 blocks;
ours is 1,600. Set a `dirty` flag in the drill completion path (`Update()`, `:244`, right after
`level.RecomputeWalls()`), rebuild once at the top of the next frame, `glBufferData` the whole
thing. One line of policy that saves an entire chunk-management subsystem.

### 3.3 Greedy meshing — the algorithm, and why not to use it

The algorithm (Lysenko, [*Meshing in a Minecraft Game*](https://0fps.net/2012/06/30/meshing-in-a-minecraft-game/)):
for each of the three axes and each slice perpendicular to it, build a 2D mask of face
ids; then repeatedly find the lexicographically-first unconsumed cell, extend it right while
the mask matches, then extend it down while the whole row matches, emit one quad, and zero out
the rectangle. For our single-layer world only the ±Y sweep and two ±X/±Z sweeps exist, and the
±Y sweep is a single slice.

```cpp
// Greedy rectangle decomposition of one 2D mask. mask[w*h] holds a material key, 0 = empty.
static void GreedySlice(int w, int h, const GLubyte* mask, std::vector<Rect>& out)
{
    std::vector<GLubyte> m(mask, mask + (size_t)w * h);
    for (int j = 0; j < h; ++j)
        for (int i = 0; i < w; ) {
            const GLubyte k = m[(size_t)j*w + i];
            if (!k) { ++i; continue; }
            int rw = 1;
            while (i + rw < w && m[(size_t)j*w + i + rw] == k) ++rw;
            int rh = 1;
            bool grow = true;
            while (j + rh < h && grow) {
                for (int q = 0; q < rw; ++q)
                    if (m[(size_t)(j+rh)*w + i + q] != k) { grow = false; break; }
                if (grow) ++rh;
            }
            for (int b = 0; b < rh; ++b)
                for (int a = 0; a < rw; ++a) m[(size_t)(j+b)*w + i + a] = 0;
            out.push_back({ i, j, rw, rh, k });
            i += rw;
        }
}
```

**Projected saving on our map (ESTIMATE):** solid tops ≈ 1,232 tiles → ~120 rectangles; open
tops ≈ 368 → ~70; side faces ≈ 500 with mean horizontal run ~2.5 → ~200. Total ≈ **390 quads**,
a further **5.9×** over culling, **~24×** over today. Sounds excellent. It is not.

Three reasons to decline:

1. **It saves 1,700 triangles.** Post-cull we are at ~4,570 triangles in one draw call. Any
   GPU that can run a 3.3 core context eats 4,570 triangles in well under 20 microseconds.
   Removing 1,700 of them changes no frame time that any human or any frame counter will see.
   The whole terrain buffer is 210 KB, which fits in L2.
2. **It fights per-vertex AO, which is the thing that actually shows.** Merging two quads is
   only legal if their AO agrees at every shared corner; interpolating one AO value across a
   4-tile-wide merged quad smears the contact darkening into a gradient, which is precisely the
   artefact AO exists to remove. AO-legal greedy merging on our map is realistically ~590 quads,
   not 390 — and on side faces, where AO varies per-tile, it merges almost nothing. You would
   spend 150 lines to save ~1,300 triangles while making the best-looking feature worse.
3. **It changes the material key semantics.** Once quads span N tiles, texture UVs must be
   derived from world position (which triplanar mapping already does, §6.6 — so that part is
   free), but per-tile animation (the crystal pulse at `:452`) can no longer be a per-vertex
   attribute.

**Verdict: implement §3.2 culling, skip greedy meshing.** Revisit only if the map grows past
~200×200, at which point culled quad count crosses ~50,000 and the argument inverts. Record the
decision so nobody re-litigates it: *greedy meshing is correct advice for voxel engines and
wrong for a 1,600-tile heightfield.*

---

## 4. Shaders

### 4.1 The default: lit, textured, fogged, per-pixel normals

```glsl
// terrain.vert -----------------------------------------------------------------------
#version 330 core
layout(location = 0) in vec3  aPos;
layout(location = 1) in vec3  aNrm;      // GL_BYTE normalized -> already unit length
layout(location = 2) in float aAO;       // GL_UNSIGNED_BYTE normalized -> [0,1]
layout(location = 3) in uint  aMat;

uniform mat4 uViewProj;
uniform mat4 uModel;                     // identity for terrain; used for figures
uniform vec3 uCamPos;

out vec3      vWorld;
out vec3      vNrm;
out float     vAO;
flat out uint vMat;

void main()
{
    vec4 wp = uModel * vec4(aPos, 1.0);
    vWorld  = wp.xyz;
    vNrm    = mat3(uModel) * aNrm;       // terrain never scales non-uniformly
    vAO     = aAO;
    vMat    = aMat;
    gl_Position = uViewProj * wp;
}
```

```glsl
// terrain.frag -----------------------------------------------------------------------
#version 330 core
in vec3      vWorld;
in vec3      vNrm;
in float     vAO;
flat in uint vMat;

layout(location = 0) out vec3 oColor;      // HDR scene
layout(location = 1) out vec3 oEmissive;   // section 7.3 -- bloom source, NOT a threshold

uniform sampler2D uAlbedo;      // triplanar atlas or per-material texture (section 6)
uniform sampler2D uNormalH;     // rgb = tangent-space normal, a = height (section 6.4)
uniform vec3  uMatColor[8];
uniform float uMatTexScale[8];
uniform float uMatEmissive[8];

uniform vec3  uSunDir;          // normalized, points FROM the surface TOWARD the light
uniform vec3  uSunColor;
uniform vec3  uSkyColor;        // hemisphere ambient, top
uniform vec3  uGroundColor;     // hemisphere ambient, bottom
uniform vec3  uCamPos;
uniform float uFogDensity;
uniform vec3  uFogColor;
uniform float uTime;

// --- point lights: helmet lamps + crystal glow (section 4.7) ---
#define MAX_LIGHTS 12
uniform int   uLightCount;
uniform vec3  uLightPos[MAX_LIGHTS];
uniform vec3  uLightColor[MAX_LIGHTS];   // pre-multiplied by intensity
uniform float uLightRadius[MAX_LIGHTS];

// Triplanar sample -- see section 6.6 for the full derivation.
vec4 Triplanar(sampler2D s, vec3 wp, vec3 n, float scale)
{
    vec3 bw = pow(abs(n), vec3(6.0));
    bw /= (bw.x + bw.y + bw.z);
    vec4 cx = texture(s, wp.zy * scale);
    vec4 cy = texture(s, wp.xz * scale);
    vec4 cz = texture(s, wp.xy * scale);
    return cx * bw.x + cy * bw.y + cz * bw.z;
}

void main()
{
    vec3  N = normalize(vNrm);
    vec3  V = normalize(uCamPos - vWorld);
    float scale = uMatTexScale[vMat];

    vec4 alb = Triplanar(uAlbedo, vWorld, N, scale);
    vec3 base = alb.rgb * uMatColor[vMat];

    // --- per-pixel normal perturbation (section 6.4) ---
    vec4 nh = Triplanar(uNormalH, vWorld, N, scale);
    N = normalize(N + (nh.xyz * 2.0 - 1.0) * 0.65);   // "whiteout"-style cheap blend

    // --- ambient: hemisphere, modulated by BAKED AO (section 4.3) ---
    float hemi   = N.y * 0.5 + 0.5;
    vec3  ambient = mix(uGroundColor, uSkyColor, hemi) * vAO;

    // --- key light ---
    float ndl     = max(dot(N, uSunDir), 0.0);
    // AO should not fully darken direct light, only ambient; a sqrt keeps contact shadows
    // present without making lit faces look dirty.
    vec3  direct  = uSunColor * ndl * sqrt(vAO);

    // --- point lights ---
    vec3 pl = vec3(0.0);
    for (int i = 0; i < uLightCount; ++i) {
        vec3  d   = uLightPos[i] - vWorld;
        float dd  = dot(d, d);
        float r   = uLightRadius[i];
        // Windowed inverse square: physical falloff, but reaches exactly zero at r so we
        // can cull lights on the CPU without a visible pop.
        float att = clamp(1.0 - dd / (r * r), 0.0, 1.0);
        att = att * att / (dd + 1.0);
        pl += uLightColor[i] * max(dot(N, normalize(d)), 0.0) * att;
    }

    vec3 color = base * (ambient + direct + pl);

    // --- specular (section 4.6) ---
    vec3  H    = normalize(uSunDir + V);
    float spec = pow(max(dot(N, H), 0.0), 48.0) * alb.a;   // a = gloss mask from the texture
    color += uSunColor * spec * 0.35 * vAO;

    // --- rim (section 4.5) : the highest gain-per-line item in this shader ---
    float rim = pow(1.0 - max(dot(N, V), 0.0), 3.0);
    rim *= (0.35 + 0.65 * vAO);           // do not rim-light the inside of a crevice
    color += uSkyColor * rim * 0.55;

    // --- emissive (section 4.6) ---
    float pulse = 0.72 + 0.28 * sin(uTime * 3.2 + vWorld.x * 1.7 + vWorld.z * 2.3);
    vec3  emis  = base * uMatEmissive[vMat] * pulse;
    color += emis;

    // --- fog: exp-squared distance + a height term so the floor of the cave sinks away ---
    float dist   = length(uCamPos - vWorld);
    float fd     = uFogDensity * dist;
    float fogAmt = 1.0 - exp(-fd * fd);
    float hgt    = exp(-max(vWorld.y, 0.0) * 0.45);   // low geometry fogs harder
    fogAmt       = clamp(fogAmt * mix(0.75, 1.0, hgt), 0.0, 1.0);
    color = mix(color, uFogColor, fogAmt);

    oColor    = color;
    oEmissive = emis * (1.0 - fogAmt);   // fog must dim the bloom source too, or crystals
                                         // glow *through* the fog and break the depth cue
}
```

Notes that are load-bearing:

- **`flat out uint`/`flat in uint`** for the material id. Without `flat` the driver
  interpolates it and you get material 3.5.
- **Exponential-squared fog** replaces the `GL_LINEAR` fog at `:647-649`. Linear fog has a
  visible onset plane at `GL_FOG_START = 26`; exp² has none, and in a cave the difference is
  large. Match the current look with `uFogDensity ≈ 0.028`.
- **Height fog** is 3 lines and is the single cheapest way to make a cave feel deep.
- The fog is applied to `oEmissive` as well. Forget that and distant crystals bloom through
  solid rock.

### 4.2 Do this before anything else: work in linear space

Today every colour in the file (`:389-397`, `:447-474`) is an sRGB value being used directly as
a linear radiance. That is why lit faces look chalky and shadowed faces look muddy — the whole
tonal range is compressed wrongly. Two fixes, both trivial:

1. **Convert the authored colours to linear once, on the CPU, at load:**
   `linear = pow(srgb, 2.2)` (or the exact piecewise sRGB curve; 2.2 is fine here). One
   `for` loop over `uMatColor[]`.
2. **Encode back to sRGB at the very end of the post chain** (§7.4), after tonemapping:
   `oFrag = pow(color, vec3(1.0/2.2))`.

Alternatively `glEnable(GL_FRAMEBUFFER_SRGB)` with an `GL_SRGB8_ALPHA8` default framebuffer
does step 2 in hardware — but it interacts badly with our offscreen HDR target, so do it in the
shader. Four lines total. **The gain is not subtle**: correct midtone rolloff is most of what
separates "flat-shaded prototype" from "rendered".

For procedural textures (§6), decode on sample: either upload them as `GL_SRGB8_ALPHA8`
(`0x8C43`, hardware decodes for free) or, better, **generate them directly in linear** — we
author them in code, so there is no sRGB to undo. Choose linear generation and never think
about it again.

### 4.3 Per-vertex ambient occlusion — the top item, and it works today

Voxel AO, per Lysenko's
[*Ambient occlusion for Minecraft-like worlds*](https://0fps.net/2013/07/03/ambient-occlusion-for-minecraft-like-worlds/):
for each vertex of a face, look at the three cells that touch that corner on the *outside* of
the face — two edge-adjacent (`side1`, `side2`) and one diagonal (`corner`) — and take:

```
vertexAO(side1, side2, corner) = (side1 && side2) ? 0 : 3 - (side1 + side2 + corner)
```

giving 0..3, which maps to four occlusion levels. The `side1 && side2` special case matters:
when both edges are solid the corner is fully enclosed regardless of the diagonal, so it must
clamp to fully dark.

For our single-layer heightfield the neighbourhood is 2D and even simpler:

```cpp
// ao level 0..3 -> a multiplier. The curve is a design choice; this one is deliberately
// strong, because subtle AO reads as "dirty texture" while strong AO reads as "geometry".
static const float AO_CURVE[4] = { 0.42f, 0.62f, 0.82f, 1.00f };

static GLubyte VertexAO(const Game& g, int cx, int cz, int dx, int dz, bool topFace)
{
    if (!topFace) {
        // Side faces of a wall: occlusion comes from whether the two tiles flanking this
        // corner (along the face) and the diagonal are solid.
        const bool s1 = g.Solid(cx + dx, cz);
        const bool s2 = g.Solid(cx, cz + dz);
        const bool cn = g.Solid(cx + dx, cz + dz);
        const int  lv = (s1 && s2) ? 0 : 3 - (int)s1 - (int)s2 - (int)cn;
        return (GLubyte)(AO_CURVE[lv] * 255.0f);
    }
    // Top face of a wall column, or a floor quad: the corner is darkened by SOLID
    // neighbours, because those are the columns rising beside it.
    const bool s1 = g.Solid(cx + dx, cz);
    const bool s2 = g.Solid(cx, cz + dz);
    const bool cn = g.Solid(cx + dx, cz + dz);
    const int  lv = (s1 && s2) ? 0 : 3 - (int)s1 - (int)s2 - (int)cn;
    return (GLubyte)(AO_CURVE[lv] * 255.0f);
}
```

**The anisotropy fix is mandatory.** A quad is two triangles; if the AO values at the four
corners are not planar, the shared diagonal produces a hard crease that flips direction
arbitrarily and looks like a rendering bug. Choose the diagonal that follows the gradient:

```
if (a00 + a11 > a01 + a10)  emit the flipped triangulation
else                        emit the normal triangulation
```

This is already in `PushQuad` in §3.2. It is four lines and without it the whole effect looks
broken.

**And here is the part that matters for scheduling: none of this needs the port.** In the
existing immediate-mode renderer, `Cube()` already calls `glColor3f` once per face (`:365`).
Move the colour call inside the vertex loop and multiply by the per-vertex AO:

```cpp
// Drop-in replacement for the inner loop of Cube() at DeepCore3D.cpp:363-368.
// Requires GL_SMOOTH shading (the default) -- do NOT call glShadeModel(GL_FLAT).
glBegin(GL_QUADS);
for (const F& f : faces) {
    float ao[4];
    ComputeFaceAO(gx, gz, f, ao);              // 4 lookups per face, all cached in L1
    for (int i = 0; i < 4; i++) {
        const float k = f.k * ao[i];
        glColor3f(r * k, gr * k, b * k);       // per-VERTEX colour, not per-face
        glVertex3fv(f.v[i]);
    }
}
glEnd();
```

**Cost: ~50 lines. Prerequisites: none. Perceived gain: the largest single item in this
document.** The immediate-mode version cannot do the diagonal-flip fix (`GL_QUADS`
triangulation is driver-chosen), so expect some corner creases — but flat cubes with AO look
dramatically more like carved rock than flat cubes without, and it costs one evening. Ship it
before the port, evaluate, and let it inform how much of the rest is worth doing.

### 4.4 Ambient: hemisphere, not a constant

`ambient = mix(groundColor, skyColor, N.y * 0.5 + 0.5)` — two lines, and it gives every
upward-facing surface a cool tint and every downward-facing one a warm bounce. In a cave, set
`uSkyColor` to a dim blue-grey and `uGroundColor` to a warm brown; the result reads as "lit by
the rock" rather than "lit by nothing". This is the cheapest believable global illumination
available. Do not replace it with SSAO — the baked AO in §4.3 is *better* for voxels (exact,
temporally stable, free at runtime) and SSAO would only add halos.

### 4.5 Rim light — 8 lines, enormous in a dark cave

```glsl
float rim = pow(1.0 - max(dot(N, V), 0.0), uRimPower);   // uRimPower ~ 3.0
rim *= (0.35 + 0.65 * vAO);        // suppress it inside crevices or it fills them in
color += uRimColor * rim * uRimStrength;                 // uRimStrength ~ 0.55
```

Why it matters more here than anywhere else: our scene is low-contrast (everything is a shade
of dark grey-brown) and heavily fogged, so **silhouettes disappear into the background**. Rim
light is a physically-motivated cheat — it approximates grazing-angle Fresnel and light
wrapping around the object — that reintroduces silhouette contrast exactly where the eye looks
for shape. Apply it to figures too, with a stronger `uRimStrength` (~0.9) and a colour tinted
toward the cave ambient, and the miners stop looking like they are pasted onto the terrain.

Rank: **highest gain-per-line of any shader item.**

### 4.6 Specular and emissive

**Specular.** Blinn-Phong, not GGX. `H = normalize(L + V); spec = pow(max(dot(N,H),0), n)`.
Reasons: we are stylised, our materials are dielectric rock plus one glassy crystal, and GGX
would cost a Smith visibility term and a Fresnel for a difference nobody will name. Gate it by
a gloss mask sampled from the texture's alpha channel (§6.3) so rock stays matte and crystal
facets flash. `n = 48` for crystal, `n = 8` and 0.1 strength for rock.

Add a wet-rock variant almost free: `spec *= mix(1.0, 4.0, wetness)` where `wetness` comes from
proximity to a `BLOCK_WATER` tile, baked into the vertex AO byte's unused high bits or a second
attribute. Water-adjacent rock that glistens is a strong "someone authored this" signal.

**Emissive.** Do **not** implement bloom by thresholding scene luminance. Threshold-based bloom
blooms bright *lit* surfaces (the top faces near the light) as well as the crystals, which is
exactly wrong. Instead write emissive to a **second render target** (`layout(location = 1) out
vec3 oEmissive`) and bloom *that*. It requires `glDrawBuffers` (already in the loader list) and
one extra colour attachment, and it means the artist — us, in code — controls exactly what
glows.

```cpp
GLenum bufs[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
glDrawBuffers(2, bufs);
```

`uMatEmissive[MAT_CRYSTAL] = 2.6` (an HDR value above 1.0, which is what makes bloom look like
light rather than like blur), everything else 0.

### 4.7 Point lights — the thing that will actually sell "cave"

`DrawMiner` already draws a lamp cube at `:392`. Make it a light. With ≤12 lights and a
windowed inverse-square falloff (in the shader above), the cost is one loop of ~10 ALU ops per
fragment — nothing at our fill rate.

```cpp
// Gather per frame, CPU-side. Sort by distance to the camera and take the nearest 12.
lights.clear();
for (const Miner& m : g.miners)
    lights.push_back({ {m.pos.x + sinf(m.facing)*0.14f, 0.66f, m.pos.z + cosf(m.facing)*0.14f},
                       {1.00f, 0.86f, 0.55f}, 7.0f });    // warm helmet lamp
for (each visible crystal seam)
    lights.push_back({ {x, 0.85f, z}, {0.55f, 0.30f, 1.00f}, 5.5f });  // cold crystal glow
```

Two lights of *different colour temperature* — warm lamps, cold crystals — is the oldest trick
in lighting and it is worth more than any amount of shader sophistication. It also makes the
gameplay read better: you can see at a glance where your crew is and where the crystals are.

---

## 5. Shadows: the honest argument, both directions

### The case against

- **The light is overhead.** With the sun near vertical and walls only 1.24 units tall on a
  1×1 lattice, every shadow lands on the tile immediately adjacent to its caster. That contact
  darkening is *precisely* what per-vertex AO (§4.3) already provides — for free, with no
  acne, no peter-panning, no bias tuning, no second matrix, and no second geometry pass.
- **It is a whole subsystem.** FBO, depth texture, a second view-projection, a light-space
  transform in the vertex shader, bias, PCF, a border-clamp trick so the world outside the map
  is not in shadow, and a `glCullFace(GL_FRONT)` pass to fight peter-panning. ~200 lines and,
  more to the point, **the highest-risk 200 lines in this document** — shadow acne is the
  classic bug that eats a week and cannot be debugged without a frame capture, which the brief
  says we do not have.
- **It competes for the same budget as items that show more.** 200 lines of shadow versus
  230 lines of procedural texture (§6). The texture wins on any honest viewing test.

### The case for

- **Our world is uniquely friendly to it.** The map is 40×40×~1.3 units, entirely static
  between drill events, and fully bounded. That means: **one orthographic shadow map covers the
  entire world. No cascades. No camera-following. No stability jitter.** Those are the three
  hard parts of shadow mapping and we get to skip all of them.
- **The resolution is luxurious.** A 60×60-unit ortho box at 2048² is 0.029 units/texel — **34
  texels across a single tile.** Compare a real game budgeting ~1 texel per 10 cm at 100 m.
  We have more shadow resolution per feature than most shipped titles.
- **It can be cached.** The terrain changes only when a wall is drilled. Render the terrain
  shadow map **once per level and once per drill**, not once per frame, and its runtime cost is
  literally zero. Only the figures need a per-frame pass, and they are ~65 cubes. This is a
  luxury almost no engine has.
- **And the killer counter-argument to "the light is overhead": don't put it overhead.** Tilt
  the key light to ~45–50° elevation and the walls throw long, directional shadows across the
  open floor — which is the surface a top-down camera spends most of its pixels on. That is a
  strong, immediately legible improvement, and it also gives the specular term something to do.

### Recommendation

**Do it, but do it last, and only after §4, §6 and §7 are in.** The static-cache trick makes it
cheap enough to be worth the 200 lines, but it is the item most likely to consume a day in bias
tuning, and every item ranked above it delivers more per hour. If time runs out, per-vertex AO
plus rim plus point lights already covers most of what shadows would have communicated.

**If you do it, tilt the light.** An overhead shadow map here really is not worth the trouble.

### The implementation, with the bias problem named

```cpp
#define GL_FRAMEBUFFER            0x8D40
#define GL_DEPTH_ATTACHMENT       0x8D00
#define GL_FRAMEBUFFER_COMPLETE   0x8CD5
#define GL_DEPTH_COMPONENT24      0x81A6
#define GL_CLAMP_TO_BORDER        0x812D
#define GL_TEXTURE_COMPARE_MODE   0x884C
#define GL_TEXTURE_COMPARE_FUNC   0x884D
#define GL_COMPARE_REF_TO_TEXTURE 0x884E

GLuint shadowFBO = 0, shadowTex = 0;
const GLsizei SHADOW_DIM = 2048;

void CreateShadowMap()
{
    glGenTextures(1, &shadowTex);
    glBindTexture(GL_TEXTURE_2D, shadowTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, SHADOW_DIM, SHADOW_DIM, 0,
                 GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    // LINEAR + compare mode gives free 2x2 PCF in the texture unit. Combined with the 3x3
    // loop below that is effectively 4x4 taps for the price of 9.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
    // Anything outside the map must be LIT, not shadowed -> border depth of 1.0.
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    const float border[4] = { 1.0f, 1.0f, 1.0f, 1.0f };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border);

    glGenFramebuffers(1, &shadowFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowTex, 0);
    glDrawBuffer(GL_NONE);    // GL 1.1, already linked
    glReadBuffer(GL_NONE);
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        ::OutputDebugStringA("[GL] shadow FBO incomplete\n");
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

// One ortho box covering the whole 40x40 world, forever. No cascades, no updates.
Mat4 BuildLightMatrix()
{
    const float cx = 20.0f, cz = 20.0f;             // map centre
    const float el = 48.0f * 3.14159265f / 180.0f;  // TILT IT. Overhead buys nothing.
    const float az = 0.85f;
    const float D  = 55.0f;
    const float eye[3] = { cx + cosf(el)*sinf(az)*D, sinf(el)*D, cz + cosf(el)*cosf(az)*D };
    const float ctr[3] = { cx, 0.0f, cz };
    const float up [3] = { 0.0f, 1.0f, 0.0f };
    // Half-extent must cover the map diagonal (40*sqrt(2)/2 = 28.3) plus wall height slop.
    return M4Mul(M4Ortho(-32.0f, 32.0f, -32.0f, 32.0f, 1.0f, 110.0f), M4LookAt(eye, ctr, up));
}
```

Depth-only pass: bind `shadowFBO`, `glViewport(0,0,2048,2048)`, `glClear(GL_DEPTH_BUFFER_BIT)`,
`glCullFace(GL_FRONT)`, draw the terrain VBO with a trivial position-only program, restore
`glCullFace(GL_BACK)`.

**The bias problem, stated precisely.** A shadow-map texel covers a finite area of the receiving
surface. On a surface tilted relative to the light, the single depth stored for that texel is
correct at one point and wrong across the rest of the footprint — so half the surface
self-shadows in a striped pattern (*shadow acne*). Add a constant bias and flat surfaces are
fine but thin geometry detaches from its shadow (*peter panning*). The three mitigations, all
of which you want:

1. **Slope-scaled bias** in the shader:
   `float bias = max(0.05 * (1.0 - dot(N, L)), 0.005);`
   (LearnOpenGL's formulation; scale both constants by your world units —
   for our 1-unit tiles and 0.029 units/texel, `max(0.0035*(1-NdL), 0.0004)` is a better
   starting point, because the standard constants are tuned for a much coarser map.)
2. **Front-face culling in the depth pass** (`glCullFace(GL_FRONT)`), which pushes the stored
   depth to the *back* of each solid and eliminates acne on all closed geometry. Our cubes are
   closed, so this works perfectly — but note it is incompatible with §3.2 culling, which
   removes the back faces. **You must render the shadow pass from a separate, unculled
   terrain buffer, or use `glPolygonOffset(2.0f, 4.0f)` instead.** `glPolygonOffset` is GL 1.1,
   already linked, and given our closed axis-aligned geometry it is the simpler correct choice
   here. Use it.
3. **PCF**, which softens both the acne and the aliasing.

```glsl
// In terrain.frag, with `uniform sampler2DShadow uShadow;` and `in vec4 vLightPos;`
float ShadowFactor(vec4 lp, vec3 N, vec3 L)
{
    vec3 p = lp.xyz / lp.w;
    p = p * 0.5 + 0.5;                       // NDC -> [0,1]
    if (p.z > 1.0) return 1.0;               // beyond the far plane: lit

    float bias = max(0.0035 * (1.0 - dot(N, L)), 0.0004);
    vec2 texel = 1.0 / vec2(textureSize(uShadow, 0));

    float lit = 0.0;
    for (int x = -1; x <= 1; ++x)
        for (int y = -1; y <= 1; ++y)
            lit += texture(uShadow, vec3(p.xy + vec2(x, y) * texel, p.z - bias));
    return lit / 9.0;                        // sampler2DShadow: each tap is already 2x2 PCF
}
// ... then:  vec3 direct = uSunColor * ndl * sqrt(vAO) * ShadowFactor(vLightPos, N, uSunDir);
```

Using `sampler2DShadow` with `GL_LINEAR` means each of the 9 taps is hardware-filtered across
2×2 texels, so the 3×3 loop yields a 4×4-texel penumbra for 9 samples. That is the correct
default; do not write a manual `>` comparison against a `sampler2D`.

Source: [LearnOpenGL — Shadow Mapping](https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping).

---

## 6. Procedural textures — we ship no art, so we compute it

Everything here runs **once, at load**, on the CPU, into an `unsigned char` buffer, and gets
uploaded with `glTexImage2D` + `glGenerateMipmap`. 256×256 RGBA8 is 256 KB per texture; three
textures is 768 KB and about 4 ms of generation time. Nothing streams; nothing is a file.

### 6.1 The hash and value noise

```cpp
// A 2D integer hash. Cheap, deterministic across platforms (no float rounding), and free of
// the axis-aligned artefacts you get from the classic sin(dot(p,k))*43758.5 trick.
static inline unsigned int Hash2(int x, int y, unsigned int seed)
{
    unsigned int h = (unsigned int)x * 0x8DA6B343u
                   ^ (unsigned int)y * 0xD8163841u
                   ^ seed             * 0xCB1AB31Fu;
    h ^= h >> 15; h *= 0x2C1B3C6Du;
    h ^= h >> 12; h *= 0x297A2D39u;
    h ^= h >> 15;
    return h;
}
static inline float Hash2f(int x, int y, unsigned int seed)   // -> [0,1)
{
    return (float)(Hash2(x, y, seed) & 0x00FFFFFFu) / 16777216.0f;
}

// Quintic fade -- Perlin's improved interpolant. C2-continuous, so fBm derivatives (and thus
// the normal map in 6.4) are smooth. smoothstep (3t^2-2t^3) is C1 only and produces visible
// creases in the derived normals; use the quintic.
static inline float Fade(float t) { return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f); }
static inline float Lerp(float a, float b, float t) { return a + (b - a) * t; }

static float ValueNoise2D(float x, float y, unsigned int seed)
{
    const int   ix = (int)std::floor(x), iy = (int)std::floor(y);
    const float fx = x - (float)ix,      fy = y - (float)iy;
    const float u = Fade(fx), v = Fade(fy);
    const float n00 = Hash2f(ix,   iy,   seed);
    const float n10 = Hash2f(ix+1, iy,   seed);
    const float n01 = Hash2f(ix,   iy+1, seed);
    const float n11 = Hash2f(ix+1, iy+1, seed);
    return Lerp(Lerp(n00, n10, u), Lerp(n01, n11, u), v);
}

// fBm: a fractal sum of octaves. lacunarity 2.0 = each octave doubles frequency; gain 0.5 =
// each octave halves amplitude. Both are the classic values; nudging lacunarity to 1.97
// breaks up the grid alignment that exact doubling produces.
static float FBM(float x, float y, int octaves, unsigned int seed)
{
    float sum = 0.0f, amp = 0.5f, freq = 1.0f, norm = 0.0f;
    for (int o = 0; o < octaves; ++o) {
        sum  += amp * ValueNoise2D(x * freq, y * freq, seed + (unsigned)o * 977u);
        norm += amp;
        freq *= 1.97f;
        amp  *= 0.5f;
    }
    return sum / norm;                     // -> [0,1]
}
```

Reference: [Inigo Quilez, *fBm*](https://iquilezles.org/articles/fbm/) and
[*Value noise derivatives*](https://iquilezles.org/articles/morenoise/).

**Tileability matters** — these textures repeat across a 40-unit floor. Make the noise periodic
by wrapping the lattice coordinates: `Hash2(ix & (P-1), iy & (P-1), seed)` with the noise
sampled over exactly `P` lattice cells per texture edge. Two lines, and without them you get a
visible seam grid.

### 6.2 Rock

Rock is not one noise. It is a low-frequency mass, a mid-frequency stratification, and a
high-frequency grain, plus a *ridged* term for the fracture lines that make rock read as rock.

```cpp
// Ridged noise: fold the noise about 0.5 and square it. The creases become sharp lines.
static float Ridged(float x, float y, int oct, unsigned int seed)
{
    float sum = 0.0f, amp = 0.5f, freq = 1.0f, norm = 0.0f;
    for (int o = 0; o < oct; ++o) {
        float n = 1.0f - std::fabs(ValueNoise2D(x*freq, y*freq, seed + (unsigned)o*613u)
                                   * 2.0f - 1.0f);
        sum  += amp * n * n;
        norm += amp;
        freq *= 2.03f;
        amp  *= 0.5f;
    }
    return sum / norm;
}

static float RockHeight(float u, float v, unsigned int seed)
{
    const float mass   = FBM(u *  3.0f, v *  3.0f, 4, seed);          // boulder scale
    const float strata = FBM(u *  2.0f, v * 11.0f, 3, seed + 91u);    // anisotropic bedding
    const float grain  = FBM(u * 24.0f, v * 24.0f, 3, seed + 7717u);  // surface tooth
    const float cracks = Ridged(u * 5.0f, v * 5.0f, 4, seed + 313u);  // fracture network
    float h = mass * 0.45f + strata * 0.20f + grain * 0.12f;
    h -= std::pow(cracks, 3.0f) * 0.34f;             // cracks CUT into the surface
    return h < 0.0f ? 0.0f : (h > 1.0f ? 1.0f : h);
}
```

Then a **colour ramp** driven by height, not a flat tint. Two or three stops (dark damp
crevice → mid rock → pale dry ridge) is the difference between "noise" and "material":

```cpp
static void Ramp(float t, const float a[3], const float b[3], const float c[3], float o[3])
{
    if (t < 0.5f) { const float k = t * 2.0f;
        for (int i=0;i<3;++i) o[i] = a[i] + (b[i]-a[i]) * k; }
    else          { const float k = (t - 0.5f) * 2.0f;
        for (int i=0;i<3;++i) o[i] = b[i] + (c[i]-b[i]) * k; }
}
// Linear-space stops (already gamma-decoded -- see 4.2), warm grey-brown:
const float ROCK_LO[3] = { 0.031f, 0.026f, 0.022f };
const float ROCK_MI[3] = { 0.118f, 0.099f, 0.081f };
const float ROCK_HI[3] = { 0.276f, 0.243f, 0.206f };
```

### 6.3 Crystal facets, and the dirt floor

**Crystal.** Facets are a cellular (Worley) pattern: jitter one feature point per lattice cell,
find the nearest (F1) and second-nearest (F2), and use **F2 − F1** — that quantity is near zero
exactly on the cell boundaries, so it draws the facet *edges*.

```cpp
static void Worley2(float x, float y, unsigned int seed, float& f1, float& f2)
{
    const int ix = (int)std::floor(x), iy = (int)std::floor(y);
    f1 = f2 = 1e9f;
    for (int dy = -1; dy <= 1; ++dy)
        for (int dx = -1; dx <= 1; ++dx) {
            const int cx = ix + dx, cy = iy + dy;
            const float px = (float)cx + Hash2f(cx, cy, seed);
            const float py = (float)cy + Hash2f(cx, cy, seed + 5501u);
            const float d  = (px-x)*(px-x) + (py-y)*(py-y);
            if (d < f1) { f2 = f1; f1 = d; } else if (d < f2) { f2 = d; }
        }
    f1 = std::sqrt(f1); f2 = std::sqrt(f2);
}

// Crystal texel: flat facet interiors, bright edges, gloss high everywhere.
// rgb = colour, a = gloss mask (feeds the specular term in 4.1).
static void CrystalTexel(float u, float v, unsigned int seed, unsigned char out[4])
{
    float f1, f2;
    Worley2(u * 7.0f, v * 7.0f, seed, f1, f2);
    const float edge  = 1.0f - std::min((f2 - f1) * 3.4f, 1.0f);   // 1 on facet borders
    const float facet = Hash2f((int)(u*7.0f), (int)(v*7.0f), seed + 41u);  // per-facet value
    const float lum   = 0.34f + facet * 0.30f + edge * 0.85f;
    out[0] = (unsigned char)(std::min(lum * 0.62f, 1.0f) * 255.0f);
    out[1] = (unsigned char)(std::min(lum * 0.30f, 1.0f) * 255.0f);
    out[2] = (unsigned char)(std::min(lum * 1.00f, 1.0f) * 255.0f);
    out[3] = 255;                                                   // fully glossy
}
```

**Dirt / floor.** Different statistics from rock: no strata, no fracture ridges; instead
low-frequency patchiness plus discrete pebbles. Pebbles are Worley F1 thresholded:

```cpp
static float FloorHeight(float u, float v, unsigned int seed)
{
    const float patch = FBM(u * 4.0f, v * 4.0f, 4, seed);
    const float fines = FBM(u * 30.0f, v * 30.0f, 2, seed + 8081u);
    float f1, f2; Worley2(u * 15.0f, v * 15.0f, seed + 2u, f1, f2);
    const float pebble = (f1 < 0.30f) ? (0.30f - f1) * 2.4f : 0.0f;   // scattered stones
    return patch * 0.55f + fines * 0.15f + pebble;
}
```

Also generate a **path/scuff mask** in the alpha channel and drive it from `BLOCK_PATH`, so
walked routes look worn. Two lines of extra generation, and it is the kind of detail that reads
as authored.

### 6.4 The normal map, derived from the height field

We generated a height for every texel. Central differences turn it into a tangent-space normal
for free — no art tool, no baker.

```cpp
// After filling float height[H][W], derive the normal map. `bumpScale` is in "height units
// per texel"; 3-8 is a good range for rock, 1-2 for the floor.
static void HeightToNormalRGBA(const float* h, int W, int Hh, float bumpScale,
                               unsigned char* rgba /* W*Hh*4 */)
{
    for (int y = 0; y < Hh; ++y)
        for (int x = 0; x < W; ++x) {
            const int xm = (x - 1 + W) % W, xp = (x + 1) % W;      // wrap: tileable
            const int ym = (y - 1 + Hh) % Hh, yp = (y + 1) % Hh;
            const float dx = (h[y*W + xp] - h[y*W + xm]) * bumpScale;
            const float dy = (h[yp*W + x] - h[ym*W + x]) * bumpScale;
            float n[3] = { -dx, -dy, 1.0f };
            const float inv = 1.0f / std::sqrt(n[0]*n[0] + n[1]*n[1] + n[2]*n[2]);
            unsigned char* p = rgba + ((size_t)y * W + x) * 4;
            p[0] = (unsigned char)((n[0]*inv * 0.5f + 0.5f) * 255.0f);
            p[1] = (unsigned char)((n[1]*inv * 0.5f + 0.5f) * 255.0f);
            p[2] = (unsigned char)((n[2]*inv * 0.5f + 0.5f) * 255.0f);
            p[3] = (unsigned char)(h[y*W + x] * 255.0f);            // keep height in alpha
        }
}
```

Height in alpha is not wasted: it can drive a parallax offset later, or a wetness/dirt mask, or
the gloss term.

### 6.5 Upload, mips, anisotropy

```cpp
static GLuint UploadRGBA(const unsigned char* px, int w, int h, bool linearData)
{
    GLuint t = 0;
    glGenTextures(1, &t);
    glBindTexture(GL_TEXTURE_2D, t);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
    // We GENERATE in linear space (4.2), so GL_RGBA8, not GL_SRGB8_ALPHA8.
    // Normal maps must NEVER be sRGB regardless.
    (void)linearData;
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, px);
    glGenerateMipmap(GL_TEXTURE_2D);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    // Anisotropic filtering: an EXTENSION, not core. Query, do not assume. Our camera looks
    // at the floor at a grazing angle, which is the exact case aniso fixes -- high value.
    static float maxAniso = -1.0f;
    if (maxAniso < 0.0f) {
        maxAniso = 0.0f;
        if (GL3_HasExtension("GL_EXT_texture_filter_anisotropic"))
            glGetFloatv(0x84FF /*GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT*/, &maxAniso);
    }
    if (maxAniso > 1.0f)
        glTexParameterf(GL_TEXTURE_2D, 0x84FE /*GL_TEXTURE_MAX_ANISOTROPY_EXT*/,
                        maxAniso < 8.0f ? maxAniso : 8.0f);
    return t;
}
```

`GL3_HasExtension` uses `glGetStringi(GL_EXTENSIONS, i)` for `i < GL_NUM_EXTENSIONS` — the
GL 3.0+ form. **`glGetString(GL_EXTENSIONS)` returns `NULL` in a core profile**; do not use it.

### 6.6 Triplanar mapping — why a voxel needs no UVs

Every terrain face is axis-aligned, so its texture coordinates could be trivially derived — but
if you also want the *figures*, the seam toppers, and any future non-axis-aligned geometry to
share the same material with no authored UVs, triplanar is the answer, and it costs two extra
texture fetches.

The idea: sample the texture three times, projecting world position onto the XY, YZ and XZ
planes, and blend by how much the surface normal points along each axis.

```glsl
vec4 Triplanar(sampler2D s, vec3 wp, vec3 n, float scale)
{
    // Blend weights. The exponent sharpens the transition; 4-8 is the usable range.
    // Too low: all three projections visible everywhere, mush. Too high: hard seams.
    vec3 bw = pow(abs(n), vec3(6.0));
    bw /= (bw.x + bw.y + bw.z);

    vec4 cx = texture(s, wp.zy * scale);    // projected along X  (weighted by |n.x|)
    vec4 cy = texture(s, wp.xz * scale);    // projected along Y  (weighted by |n.y|)
    vec4 cz = texture(s, wp.xy * scale);    // projected along Z  (weighted by |n.z|)
    return cx * bw.x + cy * bw.y + cz * bw.z;
}
```

For **axis-aligned voxel faces, `bw` is exactly (1,0,0), (0,1,0) or (0,0,1)** — the blend never
actually blends and the two off-axis fetches are wasted bandwidth. Two consequences:

- **Fast path:** for the terrain specifically, replace `Triplanar` with a single fetch selected
  by the dominant axis. That is a branch-free `mix` chain and it costs one fetch, not three:
  ```glsl
  vec2 uv = abs(n.y) > 0.5 ? wp.xz : (abs(n.x) > 0.5 ? wp.zy : wp.xy);
  vec4 c  = texture(s, uv * scale);
  ```
  This is correct because every terrain normal is exactly axis-aligned. Keep the full
  `Triplanar` for figures and anything rotated.
- **Mirroring:** on the −X and −Z faces the derived UV runs backwards, so the texture is
  mirrored. For noise-based rock nobody can tell. If it ever matters, flip the U by the sign
  of the normal: `uv.x *= sign(n.x)`.

**One real gotcha** worth naming since we have no GPU debugger: procedurally-derived UVs inside
a fragment shader break the hardware's mip-level selection at the seams between projections,
producing a bright line of over-sharp texels. The fix is to compute the derivatives from the
world position *once* and pass them explicitly with `textureGrad`. Ben Golus,
[*Distinctive Derivative Differences*](https://bgolus.medium.com/distinctive-derivative-differences-cce38d36797b),
covers this precisely. With the axis-aligned fast path above the issue does not arise, so this
is a note for later, not work for now.

Normal-map blending under triplanar has a subtlety too (a tangent-space normal from the XY
projection is not in the same frame as one from the XZ projection). The cheap "whiteout" blend
in §4.1 — perturbing the geometric normal directly — is good enough for rock and is one line.
The correct methods are catalogued at
https://github.com/bgolus/Normal-Mapping-for-a-Triplanar-Shader.

---

## 7. Post-processing

### 7.1 The offscreen target, and the fullscreen quad that needs no vertex buffer

```
scene ──> HDR FBO ─┬─> colour  (GL_RGBA16F,    attachment 0)
                   ├─> emissive(GL_R11F_G11F_B10F, attachment 1)  <- bloom source
                   └─> depth   (renderbuffer GL_DEPTH_COMPONENT24)
                        │
                        ├──> bloom chain (5 downsample + 5 upsample, section 7.3)
                        └──> composite: tonemap + grade + vignette + dither -> backbuffer
```

`GL_RGBA16F` = `0x881A`, `GL_R11F_G11F_B10F` = `0x8C3A` (11/11/10-bit float, no alpha,
half the bandwidth of RGBA16F, plenty for a bloom chain).

**The fullscreen pass needs no VBO at all.** GL 3.3 core requires *a* VAO to be bound, but it
may be empty; generate the triangle from `gl_VertexID`:

```glsl
// fullscreen.vert -- glDrawArrays(GL_TRIANGLES, 0, 3) with an empty VAO bound.
#version 330 core
out vec2 vUV;
void main()
{
    // vertex 0 -> (0,0), 1 -> (2,0), 2 -> (0,2): one oversized triangle covering the screen.
    vUV = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);
    gl_Position = vec4(vUV * 2.0 - 1.0, 0.0, 1.0);
}
```

A single oversized triangle beats two triangles: no diagonal seam, and the GPU shades each 2×2
quad once instead of twice along the diagonal.

**MSAA.** With an offscreen target you lose the default framebuffer's multisampling, and voxel
silhouettes are the worst possible case for aliasing (long straight high-contrast edges).
Two options:

- **Multisample FBO + blit resolve** (correct, ~40 lines): `glRenderbufferStorageMultisample`
  for colour and depth at 4×, then `glBlitFramebuffer` from the MS FBO into the single-sample
  HDR texture before the post chain. Both entry points are already in the loader list.
- **Supersample** (simpler, ~8 lines): render the HDR target at 1.5× or 2× the window size and
  let the final composite's bilinear filter downsample. At 1,600 tiles we have the fill budget
  to spare, and 2× SSAA is *better* quality than 4× MSAA because it also antialiases the
  shading, not just the geometry.

**Recommendation: 1.5× supersampling.** It is 8 lines, it cannot be got wrong, it antialiases
the procedural texture aliasing too, and our fill cost is negligible. Fall back to MSAA only if
frame time becomes a problem, which at ~4,570 triangles it will not.

### 7.2 Bloom that reads as light, not as blur

The naive "blur the bright pixels with a Gaussian" produces the fringed, banded, resolution-
dependent bloom that reads as cheap. The Call of Duty: Advanced Warfare method — progressive
downsample with a 13-tap filter, then progressive upsample with a 3×3 tent, accumulating
additively — produces a wide, smooth, resolution-independent glow for less cost. Implementation
per [LearnOpenGL's writeup of Jimenez's talk](https://learnopengl.com/Guest-Articles/2022/Phys.-Based-Bloom).

Chain: 5 mips down from half-resolution, then back up, adding at each level.

```glsl
// bloom_down.frag -- 13 taps, 5 weighted regions.
#version 330 core
uniform sampler2D uSrc;
uniform vec2 uSrcRes;
in  vec2 vUV;
layout(location = 0) out vec3 oColor;
void main()
{
    vec2 t = 1.0 / uSrcRes;
    float x = t.x, y = t.y;
    vec3 a = texture(uSrc, vUV + vec2(-2*x,  2*y)).rgb;
    vec3 b = texture(uSrc, vUV + vec2(   0,  2*y)).rgb;
    vec3 c = texture(uSrc, vUV + vec2( 2*x,  2*y)).rgb;
    vec3 d = texture(uSrc, vUV + vec2(-2*x,    0)).rgb;
    vec3 e = texture(uSrc, vUV                    ).rgb;
    vec3 f = texture(uSrc, vUV + vec2( 2*x,    0)).rgb;
    vec3 g = texture(uSrc, vUV + vec2(-2*x, -2*y)).rgb;
    vec3 h = texture(uSrc, vUV + vec2(   0, -2*y)).rgb;
    vec3 i = texture(uSrc, vUV + vec2( 2*x, -2*y)).rgb;
    vec3 j = texture(uSrc, vUV + vec2(  -x,    y)).rgb;
    vec3 k = texture(uSrc, vUV + vec2(   x,    y)).rgb;
    vec3 l = texture(uSrc, vUV + vec2(  -x,   -y)).rgb;
    vec3 m = texture(uSrc, vUV + vec2(   x,   -y)).rgb;
    oColor  = e * 0.125;
    oColor += (a + c + g + i) * 0.03125;
    oColor += (b + d + f + h) * 0.0625;
    oColor += (j + k + l + m) * 0.125;
}
```

```glsl
// bloom_up.frag -- 3x3 tent, additively blended onto the next-larger mip.
// Set glEnable(GL_BLEND); glBlendFunc(GL_ONE, GL_ONE); glBlendEquation(GL_FUNC_ADD).
#version 330 core
uniform sampler2D uSrc;
uniform float uRadius;          // in UV units; 0.005 is a good default
in  vec2 vUV;
layout(location = 0) out vec3 oColor;
void main()
{
    float x = uRadius, y = uRadius;
    vec3 a = texture(uSrc, vUV + vec2(-x,  y)).rgb;
    vec3 b = texture(uSrc, vUV + vec2( 0,  y)).rgb;
    vec3 c = texture(uSrc, vUV + vec2( x,  y)).rgb;
    vec3 d = texture(uSrc, vUV + vec2(-x,  0)).rgb;
    vec3 e = texture(uSrc, vUV               ).rgb;
    vec3 f = texture(uSrc, vUV + vec2( x,  0)).rgb;
    vec3 g = texture(uSrc, vUV + vec2(-x, -y)).rgb;
    vec3 h = texture(uSrc, vUV + vec2( 0, -y)).rgb;
    vec3 i = texture(uSrc, vUV + vec2( x, -y)).rgb;
    oColor = (e * 4.0 + (b + d + f + h) * 2.0 + (a + c + g + i)) * (1.0 / 16.0);
}
```

Three implementation notes that are the difference between working and not:

1. **Source the chain from the emissive attachment, not from a luminance threshold** (§4.6).
   Then bloom is a *material property* and the pulsing crystals glow while the lit rock does
   not. This is the single decision that makes the effect look intentional.
2. **Use one texture with a real mip chain**, sized `w>>1 … w>>5`, allocated once with five
   `glTexImage2D` levels; bind level *n* as the FBO colour attachment and level *n−1* as the
   source. Guard with `glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_LEVEL, 5)` or sampling
   reads garbage from unallocated levels.
3. **Clamp the source.** One `inf` or `NaN` from an emissive value multiplied by an exploding
   light will smear across the entire screen. `oEmissive = min(emis, vec3(60.0));` in the
   terrain shader.

Composite: `scene + bloom * uBloomStrength` with `uBloomStrength ≈ 0.06`. Bloom is one of those
effects where the correct amount looks like almost none.

### 7.3 Volumetric-ish light shafts — the optional flourish

Once bloom exists, ~30 more lines of radial blur on the emissive buffer, centred on the
screen-space position of the brightest crystal, gives god rays. High visual payoff, low risk,
but it needs the emissive buffer and bloom to exist first. Listed here rather than ranked; do
it only if everything above is done.

### 7.4 Tonemap, grade, vignette

```glsl
// composite.frag
#version 330 core
uniform sampler2D uScene;
uniform sampler2D uBloom;
uniform float uExposure;        // ~1.1
uniform float uBloomStrength;   // ~0.06
uniform vec3  uLift;            // shadow tint,   e.g. (0.006, 0.008, 0.020)  -- cool shadows
uniform vec3  uGain;            // highlight tint,e.g. (1.04, 1.00, 0.94)     -- warm lights
uniform float uVignette;        // ~0.42
uniform vec2  uRes;
in  vec2 vUV;
out vec4 oFrag;

// Narkowicz's fitted ACES filmic curve. Expects LINEAR BT.709 input.
vec3 ACESFilm(vec3 x)
{
    const float a = 2.51, b = 0.03, c = 2.43, d = 0.59, e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

// Ordered dithering: a 4x4 Bayer matrix, ~1/255 amplitude, applied AFTER the sRGB encode.
// In a dark, foggy cave this removes the concentric banding rings that 8-bit output
// otherwise shows in the fog gradient. Six lines. Do not skip it.
float Bayer4(vec2 p)
{
    const mat4 M = mat4( 0.0,  8.0,  2.0, 10.0,
                        12.0,  4.0, 14.0,  6.0,
                         3.0, 11.0,  1.0,  9.0,
                        15.0,  7.0, 13.0,  5.0);
    ivec2 i = ivec2(mod(p, 4.0));
    return M[i.x][i.y] * (1.0 / 16.0) - 0.5;
}

void main()
{
    vec3 col = texture(uScene, vUV).rgb;
    col += texture(uBloom, vUV).rgb * uBloomStrength;

    col *= uExposure;
    col  = ACESFilm(col);                             // -> [0,1], still linear-ish

    // Lift/gain grade. Cheap, controllable, and a full 3D LUT is not worth the plumbing at
    // our scale. Cool the shadows, warm the highlights: the classic cave/torch contrast.
    col = uLift + col * (uGain - uLift);

    // Vignette: a smooth radial falloff in aspect-corrected screen space. Focuses attention
    // on the crew and hides the fact that the map edge is a hard boundary.
    vec2  q = (vUV - 0.5) * vec2(uRes.x / uRes.y, 1.0);
    float r = length(q);
    col *= mix(1.0, smoothstep(0.92, 0.28, r), uVignette);

    col = pow(col, vec3(1.0 / 2.2));                  // linear -> sRGB (section 4.2)
    col += vec3(Bayer4(gl_FragCoord.xy) / 255.0);     // dither AFTER the encode
    oFrag = vec4(col, 1.0);
}
```

Curve source: [Krzysztof Narkowicz, *ACES Filmic Tone Mapping Curve*](https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/)
(public domain / CC0 / MIT; a luminance-only fit that oversaturates very bright values — fine
for us, since almost nothing here is very bright except the crystals, which we *want* saturated).

**Ranking within this section:** dither (6 lines) and ACES + vignette (25 lines) are near the
top of the whole document by gain-per-line. Bloom (160 lines) is excellent but is an order of
magnitude more work. Do the composite pass *first*, with `uBloom` bound to a 1×1 black texture,
and add the bloom chain afterwards — that way the tonemap ships in an hour and the bloom slots
in without touching it.

---

## 8. Honest cost table

LOC counts are for a competent implementation with comments in this repo's style, not
minified. "Risk" is the probability of losing more than half a day to a bug that cannot be
seen without a frame capture.

| Item | § | LOC | Risk | Perceived gain | Notes |
|---|---|---:|---|---|---|
| **Per-vertex AO, immediate mode** | 4.3 | **50** | very low | **very high** | **No prerequisites. Do tonight.** |
| sRGB / linear-space correctness | 4.2 | 4 | very low | high | one `pow` each side |
| Dither before output | 7.4 | 6 | very low | med-high | kills fog banding |
| Rim light | 4.5 | 8 | very low | very high | needs the port |
| exp² + height fog | 4.1 | 12 | very low | medium | replaces `GL_LINEAR` fog |
| ACES + exposure + lift/gain + vignette | 7.4 | 25 | low | high | needs §7.1 |
| Blinn specular + gloss mask | 4.6 | 12 | very low | medium | |
| Per-pixel normal perturbation | 4.1/6.4 | 18 | low | medium-high | needs §6 |
| Point lights (12, windowed falloff) | 4.7 | 45 | low | very high | warm lamps + cold crystals |
| Emissive MRT | 4.6 | 20 | low | medium (enables bloom) | `glDrawBuffers` |
| Supersampled offscreen target | 7.1 | 8 | very low | medium | replaces MSAA |
| Fullscreen-triangle plumbing | 7.1 | 25 | very low | zero (enabler) | no VBO needed |
| HDR FBO + attachments + resize | 7.1 | 90 | low | zero (enabler) | check FBO status! |
| Bloom chain (5 down + 5 up) | 7.2 | 160 | low-medium | high | mip-level FBO binding is fiddly |
| Value/ridged/Worley noise + fBm | 6.1-6.3 | 150 | low | — | pure CPU, testable in isolation |
| Rock + floor + crystal textures | 6.2-6.3 | 60 | low | very high | |
| Height→normal derivation | 6.4 | 25 | very low | high | |
| Upload + mips + aniso | 6.5 | 30 | low | medium | query the extension |
| Triplanar / axis-select sampling | 6.6 | 25 | low | (part of textures) | fast path is 2 lines |
| Mesh builder + hidden-face culling | 3.2 | 180 | low | zero visual; 4.3× / 1637× draw calls | |
| Vertex format + VAO/VBO setup | 3.1 | 60 | low | zero (enabler) | `glVertexAttribIPointer` for mat |
| **Context creation (dummy dance)** | 2.2 | **110** | **medium** | zero (enabler) | the `SetPixelFormat` rule |
| **Loader (49 entry points)** | 2.3 | **210** | **medium** | zero (enabler) | **the `APIENTRY` trap, §2.4** |
| Mat4 library | 2.6 | 90 | low | zero (enabler) | replaces GLU |
| Shader compile/link/uniform cache | — | 90 | low | zero (enabler) | log the info logs! |
| Font atlas + text VBO | 2.8 | 150 | low-medium | medium (HUD looks authored) | GDI→DIB→`GL_R8` |
| Picking by DDA | 2.7 | 60 | low | zero visual; removes a stall | drops `glu32` |
| KHR_debug callback | 2.5 | 35 | very low | zero visual; saves days | **do it in the first hour** |
| Shadow map + PCF | 5 | 200 | **medium-high** | low-medium (medium if tilted) | bias is the time sink |
| Greedy meshing | 3.3 | 150 | medium | **zero** | **do not** |
| SSAO | — | 220 | high | **negative** | **do not** — baked AO is better |
| **Recommended total (everything except shadows, greedy, SSAO)** | | **~1,700** | | | current file is 725 lines |

### The single highest-value thing to do first

**Per-vertex ambient occlusion, in the existing immediate-mode renderer, tonight (§4.3).**
It is ~50 lines, it requires none of the port, and it converts "flat-shaded cubes" into
"carved rock" — which is the dominant term in the perceived gap. Do it, look at it, and let the
result calibrate how much of the remaining ~1,650 lines is worth spending.

**If the question is instead "what is the first thing to do once we have committed to the
port":** install the KHR_debug callback (§2.5) in the first hour, before writing any rendering
code. Every subsequent bug becomes a message with a line number instead of a black screen.

---

## 9. Suggested order of work, in three checkpoints

**Checkpoint A — "it looks better and nothing changed structurally" (1 evening, ~60 lines)**
1. Per-vertex AO in `Cube()` (§4.3).
2. Linearise the authored colours; gamma-encode with `glPixelTransferf`… actually no — in the
   1.x path just pre-multiply the constants and accept it. (§4.2)
3. Swap `GL_LINEAR` fog for `GL_EXP2` (`glFogi(GL_FOG_MODE, GL_EXP2); glFogf(GL_FOG_DENSITY, 0.028f);`).
   Two lines, and it removes the visible fog onset plane.

Stop and look. This is a real decision point.

**Checkpoint B — "the pipeline is modern and the screen is identical" (~900 lines, no pixels gained)**
4. KHR_debug (§2.5) → context (§2.2) → loader (§2.3) → mat4 (§2.6).
5. Shader compile/link helper with info-log reporting.
6. Terrain mesh builder with hidden-face culling + one static VBO (§3.1, §3.2).
7. Font atlas (§2.8), picking DDA (§2.7). Delete `glu32`.
8. Base shader from §4.1, minus textures — flat colours, real normals, baked AO, hemisphere
   ambient, exp² fog.

Do not stop mid-way through B. A half-ported renderer is worse than either endpoint.

**Checkpoint C — "it stops reading as a prototype" (~800 lines, all the visible gain)**
9. Rim light (§4.5) + specular (§4.6) + point lights (§4.7). *Three items, ~65 lines, the
   biggest jump in the whole plan.*
10. Procedural textures + normal derivation + axis-select sampling (§6).
11. HDR target + fullscreen triangle + ACES/grade/vignette/dither (§7.1, §7.4).
12. Emissive MRT + bloom chain (§4.6, §7.2).
13. Supersampling (§7.1).
14. *Optional, last:* shadow map with a tilted light (§5).

---

## 10. Risks, named

| Risk | Likelihood | Mitigation |
|---|---|---|
| **`APIENTRY` omitted from a typedef** → x86 stack corruption, crash far from the cause | **high if unwarned** | §2.4. Grep the loader for `APIENTRY` count == entry count. |
| Shader compiles but link fails silently | high | Always read `GL_INFO_LOG_LENGTH` and `glGetShaderInfoLog`/`glGetProgramInfoLog`, on **success** too — some drivers emit useful warnings on a successful compile. |
| FBO incomplete, screen black, no error | high | `glCheckFramebufferStatus` after **every** attachment change, with the status printed as hex. |
| `#version` not on line 1 of the shader string | medium | Never start a raw string literal with a newline. |
| MSVC C2026: shader literal > 16,384 chars | medium | Split into adjacent literals. |
| Shadow acne / peter panning eats a day | **high if shadows attempted** | Defer shadows to last; use `glPolygonOffset(2.0, 4.0)` with §3.2's culled mesh rather than front-face culling. |
| Forward-compatible context rejects something we still use | medium | It is *supposed* to. Drop the flag only as a last resort and record why. |
| Driver has no 3.3 (very old Intel integrated) | low | The `MessageBoxA` in §2.3 names the missing function. Consider keeping `DeepCore3D.cpp` as a `-gl1` fallback build target — it is 725 self-contained lines and costs nothing to keep. |
| `glGetString(GL_EXTENSIONS)` returns NULL in core | medium | Use `glGetStringi` + `GL_NUM_EXTENSIONS`. §6.5. |
| Emissive `inf`/`NaN` smears the whole screen | medium | Clamp `oEmissive`. §7.2 note 3. |
| The 40×40 tile-count estimates in §3.2 are wrong | medium | They are marked ESTIMATE and §3.2 gives the 12-line instrumentation. Run it before quoting the numbers to anyone. |

---

## 11. Sources

- Creating an OpenGL Context (WGL) — Khronos OpenGL Wiki. The `SetPixelFormat`-once rule,
  `wglGetProcAddress` failure values, and the ARB attribute lists.
  https://wikis.khronos.org/opengl/Creating_an_OpenGL_Context_(WGL)
- Nick Rolfe, minimal Win32 modern-context sample (the dummy-context dance in ~120 lines).
  https://gist.github.com/nickrolfe/1127313ed1dbf80254b614a721b3ee9c
- Mariusz Bartosik, *OpenGL 4.x Initialization in Windows without a Framework*. Step ordering
  and the pixel-format gotcha. https://mariuszbartosik.com/opengl-4-x-initialization-in-windows-without-a-framework/
- Khronos OpenGL Registry, `GL/glext.h` and `GL/wglext.h` (`SPDX-License-Identifier: MIT`).
  https://registry.khronos.org/OpenGL/api/GL/glext.h ·
  https://registry.khronos.org/OpenGL/api/GL/wglext.h
- GLAD licence (tool MIT; **generated output public domain / CC0 / WTFPL**).
  https://github.com/Dav1dde/glad/blob/master/LICENSE
- Mikola Lysenko, *Meshing in a Minecraft Game* — culled vs greedy meshing, the lexicographic
  sweep, and the 31× / 2.3× / 1.3× quad-count benchmarks.
  https://0fps.net/2012/06/30/meshing-in-a-minecraft-game/
- Mikola Lysenko, *Ambient Occlusion for Minecraft-like Worlds* — the `vertexAO` function,
  the `side1 && side2` special case, and the quad-flip anisotropy fix.
  https://0fps.net/2013/07/03/ambient-occlusion-for-minecraft-like-worlds/
- LearnOpenGL, *Shadow Mapping* — depth-only FBO, `GL_CLAMP_TO_BORDER` trick, slope-scaled
  bias, front-face culling, 3×3 PCF.
  https://learnopengl.com/Advanced-Lighting/Shadows/Shadow-Mapping
- LearnOpenGL, *Physically Based Bloom* (after Jorge Jimenez, *Next Generation Post Processing
  in Call of Duty: Advanced Warfare*, SIGGRAPH 2014) — 13-tap downsample, 3×3 tent upsample,
  `GL_R11F_G11F_B10F` mip chain.
  https://learnopengl.com/Guest-Articles/2022/Phys.-Based-Bloom
- Krzysztof Narkowicz, *ACES Filmic Tone Mapping Curve* — the fitted curve, its constants,
  its colour-space expectations and its oversaturation caveat.
  https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
- Inigo Quilez, *fBm* and *Value Noise Derivatives* — octaves, lacunarity/gain, and why the
  quintic interpolant matters when derivatives are taken.
  https://iquilezles.org/articles/fbm/ · https://iquilezles.org/articles/morenoise/
- Ben Golus, *Distinctive Derivative Differences* — mip selection with procedurally-derived
  UVs; the triplanar seam artefact and `textureGrad`.
  https://bgolus.medium.com/distinctive-derivative-differences-cce38d36797b
- Ben Golus, *Normal Mapping for a Triplanar Shader* — whiteout / RNM / derivative-cotangent
  normal blends under triplanar projection.
  https://github.com/bgolus/Normal-Mapping-for-a-Triplanar-Shader
- Amanatides & Woo, *A Fast Voxel Traversal Algorithm for Ray Tracing* (1987) — the DDA used
  for picking in §2.7.
  https://www.cse.chalmers.se/edu/year/2010/course/TDA361/grid.pdf

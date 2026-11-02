# Vulkan renderer — maintainer notes

This document is for people maintaining the Vulkan backend. How to build, enable, run and configure it is documented in the README ("Vulkan renderer" and "Frame pacing (VSYNC and FRAME LIMIT)" sections); nothing here duplicates that. What this records is *why* the code looks the way it does, what the validation baseline is, which versions the work was measured on, and which known residuals are accepted.

**AI authorship.** The Vulkan backend was written mostly with Qwen3.8-Flash-Next and in part with Qwen3.8-27B (both local), with initial scaffolding from Qwen3.6-35B-A3B, then reviewed over many human-directed rounds with review assistance from Claude. This document was also written with AI assistance. Commits do not mark which changes were AI-generated; see the README's "AI usage disclosure".

**Evidence scope.** Every quantitative claim in this document was measured only on NVIDIA (Wayland/KWin, SDL2 API) and the Mesa llvmpipe software driver, both on Linux. AMD and Intel GPUs are unmeasured.

# Architecture anchors

- `src/render_vulkan.c` — device selection (`render_init`), backbuffer sizing, swapchain management, pipeline creation, frame lifecycle, push-constant sites.
- `src/shaders/` — GLSL sources, compiled into the binary at build time: `game.vert`, `game.frag`, `post.frag` and the CRT variants.
- `src/wipeout/game.h` / `src/wipeout/game.c` — save data and the renderer-independent settings that reach the renderer and platform layers.

The game renders into a fixed PSX-resolution backbuffer; a post pass upscales it to window size using a floor()-based canonical texel map (see "Texel ties and the upscale").

Environment variables the renderer responds to (user-facing detail in README):

| variable | effect |
|---|---|
| `WIPEOUT_GPU=<index or name substring>` | force the Vulkan physical device; a pure integer is an enumeration index, anything else a case-insensitive substring of the device name (see README) |
| `WIPEOUT_FRAME_DIAG=<N>` | print presented-frame interval statistics every N frames |
| `VK_INSTANCE_LAYERS`, `VK_LAYER_SETTINGS_PATH` | enable the validation layer and point it at a settings file (validation runs only; the path must be absolute) |

# Other design decisions (read before "simplifying")

Each item below looks removable and is not.

## Shader build

- Shaders are compiled with `glslc -mfmt=c -O` at build time into C initializer headers (`.spvh`) that `render_vulkan.c` `#include`s. There are no runtime shader files and no path resolution.
- **Do not add `--target-env=vulkan1.3` to the `glslc` command.** At that target glslang pre-declares the SPIR-V `DemoteToHelperInvocation` capability in every fragment module (even where unused), which the validation layer rejects unless the matching device feature is negotiated. The comment above the shader list in `CMakeLists.txt` carries the same warning. Plain `-O` output validates clean.
- The executable depends on the shader generation target (`add_dependencies`): the generated headers must exist before `render_vulkan.c` compiles.

## GL-to-Vulkan convention differences

- **Depth range.** The CPU-side projection matrices are GL-style (NDC z spans [-1, +1]) and are shared with the GL renderer; `game.vert` remaps `gl_Position.z = 0.5*z + 0.5*w` for Vulkan's [0, 1] depth buffer. Removing the remap clips everything closer than about twice the near plane and halves depth precision.
- **Orientation.** Three coupled choices that form one decision — change all or none: the game pass uses a negative-height viewport so the backbuffer is upright with row 0 at the scene top; the front face is counter-clockwise, matching GL; the post quad has `v_uv.y = 1` at the window top and `post.frag` samples at `1.0 - v_uv.y + bias`.
- **Distance fade.** `game.vert` reproduces the GL distance fade, computed in view space from the top three rows of `view*model` passed as push constants. That equals the world-space distance to the camera only because the view matrix is a rigid transform (`mat4_translate` post-multiplies, `mat4_set_roll_pitch_yaw` writes rotation only); if the view matrix ever gains scale or shear the fade drifts. Near and far are specialization constants derived from the same `RENDER_FADEOUT_NEAR`/`RENDER_FADEOUT_FAR` defines the C code uses, so they cannot diverge. The fade uses an explicit polynomial of `smoothstep(far, near, d)` because GLSL leaves `smoothstep` undefined when edge0 >= edge1.
- **Push-constant layout is shared between C and shaders.** The game block is `mat4 u_mvp` + three `vec4` view-model rows + `vec2 u_screen` = 120 bytes; the post block is `vec2 u_resolution` + `float u_time` + one pad slot = 16 bytes. The C push sites and every shader declaring the block must agree exactly.

## Frame lifecycle and synchronization

- One **timeline semaphore** (`timeline_next_value`, `slot_fence_value`) paces the frames in flight. Acquire semaphores are per frame slot; render-complete semaphores are per swapchain **image** and are **binary**, because `vkQueuePresentKHR` only accepts binary semaphores.
- **Texture uploads** go through a separate upload command buffer and a persistent staging buffer, batched and flushed once before the first draw that could use them. The upload submit signals the timeline; the CPU waits lazily, only before the staging buffer is rewritten or the upload pool is reset (`upload_wait_prev_complete`). Do not reintroduce a `vkQueueWaitIdle` after each upload: it also waits out the previous frame and stalls level loads.
- Idle waits (`queue_wait_idle`) exist only at: the deferred swapchain/backbuffer recreate, shutdown, paths that destroy resources the GPU may still read, and init (temporary command buffers). During a throttled VRAM-fallback period the recreate-path wait is skipped on frames where nothing is being recreated. The "recreate due" decision is computed once per frame from a single clock read (`backbuffer_attempt_due`) and reused; recomputing it from a second clock read can destroy the backbuffer without the idle wait having run.
- `render_texture_replace_pixels` submits and waits at call time, so a draw already recorded earlier in the same frame sees the new pixels — this differs from GL draw-order semantics. Its sole game caller is the `intro.c` video callback, which always runs before the pushes that use the texture.

## Presentation and frame pacing

- **VSYNC ON selects plain FIFO**, the only present mode the spec guarantees, matching GL's strict swap interval. **VSYNC OFF selects MAILBOX when offered, otherwise FIFO.** The choice is made at swapchain creation (`choose_present_mode`), so toggling vsync flags a deferred recreate (`render_set_vsync`).
- The frame limiter lives in the platform layer (`platform_limit_frame` in `platform_sdl.c`) so it covers every renderer; limit 0 means no limiter. While vsync is on, a cap at or within 1 fps of the display refresh is ignored so the limiter cannot fight the present path. The refresh rate is cached on window events, not queried every frame. README "Frame pacing (VSYNC and FRAME LIMIT)" is the user-facing documentation.
- `WIPEOUT_FRAME_DIAG=<N>` prints interval statistics over the presented-frame intervals every N frames; it costs one cached check when unset. The HUD frame rate and DEBUG MS read the presented-frame interval, not CPU work time, so DEBUG MS shows about 6 on a 165 Hz display with vsync, not 0.

## Settings and save data

- `save_t` (`src/wipeout/game.h`) gained two `int` fields at the end, `vsync` (default 1) and `max_fps` (default 0, "UNLIMITED"), backing the VSYNC and FRAME LIMIT menu rows. Appending kept the older layout byte-identical. Fields must only ever be **appended** to `save_t`, never inserted or reordered.
- Loading accepts an older, shorter save file as a prefix (the accepted size window is keyed to `offsetof(save_t, vsync)`), copying only the common prefix so each appended field keeps its compiled-in default, and marks the save dirty so the next write migrates the file to the current size — exactly `sizeof(save_t)` bytes, 4136 on the tested build. The migration is one-way: a build that predates these fields checks the exact size, rejects the longer file ("unexpected size/magic") and falls back to defaults, so settings made on this fork do not carry back to such a build.
- Menu settings are renderer-independent: VSYNC reaches the renderer through `render_set_vsync`; FRAME LIMIT reaches the platform layer through `platform_set_fps_limit`.

## Swapchain and device robustness

- **Device selection** (`render_init`) requires a Vulkan 1.3 device with the swapchain extension and a graphics queue that can present to the surface; it prefers discrete over integrated over other types (how virtualized GPUs typically report) over CPU, logs every rejection reason, and `WIPEOUT_GPU` overrides the choice.
- **Swapchain recreation** is deferred to `render_frame_prepare` behind an idle wait. Acquire and present results are handled as: `OUT_OF_DATE` sets the recreate flags; `SUBOPTIMAL` on acquire is a *success* code (render the frame, then flag recreate, throttled by `SUBOPTIMAL_RETRY_INTERVAL_MS`); acquire has a bounded timeout and a forced recreate after a streak of timeouts; a recreate that produces an extent persistently different from the window size retries on an interval instead of every frame; a zero extent skips creation and retries later.
- **Backbuffer VRAM fallback.** If creating the backbuffer at the requested size fails, the renderer falls back to the previous size and then to repeated halving (floor 320x240) so presentation stays live. While the backbuffer is below the requested size, full-size retries are throttled to about one per second (`backbuffer_retry_after_ms`); a genuine size or resolution change bypasses the throttle, and the renderer climbs back when memory frees.
- Vertex data is written directly into per-frame persistent mapped buffers (host-visible, `HOST_COHERENT`, device-local preferred; no host-side mirror). Capacity is 131072 triangles per frame, versus 2048 **per flush** in GL — hence GL reports one extra draw call on the reference scene (66 versus 65). Overflow triangles are dropped with a warning.

# Texel ties and the upscale

- The upscale resolves to the **canonical lower texel** at exact texel-boundary sampling ties, on every driver and window size. Matching the GL renderer pixel-for-pixel at ties was considered and rejected (see the measured tie behavior below); the canonical floor()-based map is the maintainer-chosen target.
- **Tie arithmetic.** Reduce the window-height / backbuffer-height ratio to p/q. Window rows sample exactly on a backbuffer texel boundary (a tie) **iff p is even and q is odd**; ties then occur every q window rows, first at d = (q-1)/2 mod q, and the nearest non-tie row sits 1/q texel from a boundary. Coprime ratios have exactly one tie row.
- **Bias form.** `post.frag` applies a small sub-texel sampling bias, `bias = -0.001 / u_resolution.y`: step-relative (−C/winH with C = 0.001), so the shift in backbuffer texels is C·p/q — scale-free across window sizes. A fixed texel bias is provably wrong: at 480P in a 721-row window the nearest non-tie coordinate is 0.001387 texel from a boundary, so the smallest fixed shift that removes the tie flips non-tie rows. The step-relative form never flips a non-tie row while C < 1/p; the margin is at least 2x at every measured size, and the bias sits at least 6x above observed cross-driver coordinate jitter.
- **Measured tie behavior.** NVIDIA's GL renderer resolves ties **up** at every measured ratio and is content-unstable off 2/3 (the pick changes with race content); Mesa GL resolves down only at 2/3. No measured renderer has a stable tie direction away from 2/3, so Vulkan-vs-NVIDIA-GL divergence at tie rows is expected and accepted. What is deterministic: the Vulkan backend resolves ties to the canonical lower texel on every driver and size measured, and two different Vulkan stacks agree on tie rows once the bias is applied.

# Validation baseline

A validated run must not produce anything beyond the noise floor below. Three runs per configuration:

1. Set `VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation`, and `VK_LAYER_SETTINGS_PATH` to the **absolute** path of one of the settings files below — a relative path silently disables the settings file and gives a false pass.

   Core validation:

   ```
   [khronos_validation]
   khronos_validation.validate_core = true
   khronos_validation.validate_best_practices = false
   khronos_validation.validate_sync = false
   ```

   Synchronization validation:

   ```
   [khronos_validation]
   khronos_validation.validate_sync = true
   ```

   Best-practices validation:

   ```
   [khronos_validation]
   khronos_validation.validate_best_practices = true
   ```

2. Run the game to a race frame (the HUD stats readout shows CALLS and TRIS).
3. Compare the validation output against the expected table:

| run | expected output |
|---|---|
| core validation | exactly 2 loader environment-variable WARN lines, nothing else |
| synchronization validation | 0 SYNC messages |
| best-practices | the same 2 loader WARNs plus exactly one accepted hint about a 96-byte dedicated allocation |

Any additional WARN, ERROR or SYNC line, or any best-practices hint beyond the accepted one, is a regression.

The two WARN lines in the core and best-practices runs come from the Vulkan loader's environment-variable handling (how the run enables the layer and supplies the settings file), not from this renderer. A loader upgrade can change their text or count. If only the loader-warning count changes and nothing else does, re-baseline the expected count instead of treating it as a regression; any other new WARN, ERROR or SYNC line, or any extra best-practices hint, is still a regression.

Race-frame HUD counters (CALLS 65, TRIS roughly 6023 to 6031; llvmpipe lands at 6023, NVIDIA at 6031) are a smoke check, not an invariant — treat a big change as a signal to investigate, not as a precise threshold.

For any shader change, run the full battery on both NVIDIA and llvmpipe; for a renderer-only change you can justify, one driver is enough.

# Tested versions

Recorded 2026-11-02:

| component | version |
|---|---|
| OS and kernel | Artix Linux, Linux 7.2.11-zen1-1-zen |
| Compositor | KDE Plasma 6.8.2 / KWin (Wayland) |
| GPU and driver | NVIDIA GeForce RTX 5080, driver 615.78.08 |
| Software fallback | Mesa 26.2.6, `llvmpipe (LLVM 23.1.4, 256 bits)` |
| Vulkan loader | 1.4.363.0 (instance version 1.4.363) |
| Khronos validation layer | 1.4.363.0 |
| `glslc` | shaderc 2026.4 (glslang 1.4.363.0) |
| SDL2 | sdl2-compat 2.32.74 (SDL2 API on Wayland, not classic SDL2) |
| Build type | Release (CMake default when unset) |

# Known limitations

- **Platforms.** Linux only so far, with the SDL2 platform. Windows and macOS are untested; shader embedding removed the old path-handling concern, but nothing else has been built or run on those platforms. The code has been prepared for portability drivers (see "Windows and macOS notes") but that is read from the specification, not measured.
- **Hardware.** A Vulkan 1.3 device with swapchain support is required. AMD and Intel GPUs are unmeasured. The texel-tie bias is driver-agnostic by construction (it removes ties rather than compensating one driver), but that generality is untested on other vendors; a single frozen capture pair at a 1080-row window on such hardware would close the gap.
- **Present modes.** FIFO_RELAXED is deliberately unused: some Wayland drivers do not offer it, and strict FIFO matches GL's swap interval.
- **Very large windows.** The −C/winH bias keeps working, but its shift in normalized texture coordinates shrinks with window height. By arithmetic (not measured), the worst case near the bottom of the window is about 8 float ulps at 2160 rows and about 4 at 4320, which leaves less margin against rasterizer jitter. Raising C restores margin; it must stay below 1/p for the window to backbuffer ratio (p is the reduced numerator).
- **Tie analysis assumes the swapchain has `screen_size.y` rows.** The bias and the tie arithmetic use the window height pushed to the post shader (`u_resolution` = `screen_size` at the push site). If a driver clamps the swapchain extent so the two differ, the predicted tie rows do not line up with the real ones; the bias is harmless in that case but does nothing.
- **Untested situations.** Multi-monitor refresh-rate changes (the limiter's cached refresh follows window events only), a zero-extent window on platforms that report one, and windows larger than the test display allows.
- **Vulkan versus NVIDIA GL at texel ties** (one source row, every q-th row at certain ratios) is expected and accepted; see "Texel ties and the upscale".
- The 240P llvmpipe raster micro-diff (0.22 to 0.30% against GL) is still uninvestigated.
- `WIPEOUT_GPU` index mode is still enumeration-order dependent — the order a loader lists devices in is not a stable contract across machines, which is why the device-name substring form exists and is the portable choice for scripts and reproducible reports.

# Windows and macOS notes

Prepared but untested; nothing here has been built or run on those platforms.

- **Portability drivers.** MoltenVK and KosmicKrisp are portability drivers. The instance enables `VK_KHR_portability_enumeration` with `VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR` when the loader offers it, and the device enables `VK_KHR_portability_subset` when the selected device advertises it. Without the former, the loader can fail `vkCreateInstance` with `VK_ERROR_INCOMPATIBLE_DRIVER`. No subset-restricted feature is used by the renderer.
- **Driver identification.** Startup prints `Vulkan: driver [name] [info]` after the selected device. Both ICDs can report the same GPU name, so this line is what tells them apart.
- **No system loader on macOS.** Install the Vulkan SDK for development. For a shipped app, bundle the loader, the driver dylib(s) and the ICD json files; do not rely on a system install. On Windows the GPU driver package installs the loader.
- **Minimum drivers.** MoltenVK 1.3.0 or later (older releases report Vulkan 1.1 and are rejected by the 1.3 gate with "Vulkan version < 1.3"). KosmicKrisp needs Apple Silicon and macOS 26. Intel Macs can only use MoltenVK. Test the two drivers separately.
- **Debug builds** request `VK_EXT_debug_utils`, which the loader provides; a bundled loader without the validation layer is fine.
- **Windows compilers.** The `LOAD_FN` macros use `typeof`, so build with clang-cl or MinGW, not MSVC `cl.exe`.
- **Unverified.** Whether MoltenVK offers IMMEDIATE (VSYNC OFF only tries MAILBOX, then FIFO), and whether the `extendedDynamicState` feature gate holds on current Apple drivers.

# Test harness methodology

- Content comparisons between renderers need a deterministic frame. They were run with a **local, uncommitted freeze patch** that makes engine state a pure function of the rendered-frame index: it fixes the time step and rebases the clock off a frame counter, enabled with `WIPEOUT_R6_FREEZE=<n>` (typically frame 120), so every build sees exactly the same frame. It is intentionally not in the repository; recreate it from that description when needed.
- Free-running comparisons are not valid. Time-dependent post effects (the CRT effect) are only comparable with the frozen harness.
- Per-row differences are attributed with pattern-decode captures plus a row ruler: an on-grid pattern whose rows map to window rows. Window-size overrides and alternative post shaders require throwaway probe builds, which are never committed.
- Captures are 1:1 — there is no compositor scaling of the client area. Anchor crops to a known pattern, not to a titlebar guess.
- Harnesses share `save.dat` (the 4136-byte migrated file) and must never run concurrently. Snapshot it before and restore it byte-identically afterwards.
- Practical trap: a large VRAM-resident workload on the machine can cause a transient `vkAllocateMemory` host-memory failure at startup. Free VRAM and retry; it is not a renderer bug.
- Per-harness scripts and evidence trees are kept *outside* the repository, in an untracked artifacts directory alongside it. Nothing harness-related ships in the repo.

# WWE 2K16 XDK Direct3D function map

The Xbox 360 D3D library (XDK) is linked statically into `default.xex`. A native renderer hooks these functions
instead of emulating the console's GPU, so their addresses are the first thing it needs. Found on 2026-11-12.

## Result

* The only `VdSwap` caller is `D3DDevice_Swap`: `sub_82C2A458` in the base game, `sub_82C32510` with title update 3.
  It also calls `VdGetSystemCommandBuffer` and `VdPersistDisplay`.
* The 16 D3D functions that the WWE '12 port names (`wwe12_d3d_names.h`) all match a 2K16 function with a mnemonic
  similarity of **1.00** (Swap 0.99), in both the base game and the title update code. Every best match is unique and
  the second-best is well below it (except `SetPixelShader` / `SetVertexShader`, which differ by 0.91 and are told
  apart by their length, 111 and 115 instructions).
* The 2K16 title update library has **exactly the same layout as WWE 2K15's TU4 library**: for all 16 matched
  functions `2K16 TU = 2K15 TU4 + 0x845F0`. The 6 other functions that the 2K15 port hooks follow from that offset;
  each prediction lands on a plausible function start (see below), but those 6 are not yet confirmed by a call census.
* The title update library sits `0x80B8` above the base game library (`TU = base + 0x80B8` for every matched function).

## Addresses

| Function | Base game | Title update (TU) | WWE 2K15 TU4 | How |
|---|---|---|---|---|
| `D3DDevice_SetTexture` | `sub_82C1F7D8` | `sub_82C27890` | `sub_82BA32A0` | matched 1.00 |
| `D3DDevice_SetStreamSource` | `sub_82C22060` | `sub_82C2A118` | `sub_82BA5B28` | matched 1.00 |
| `D3DDevice_SetIndices` | `sub_82C22208` | `sub_82C2A2C0` | `sub_82BA5CD0` | matched 1.00 |
| `D3DDevice_SetRenderTarget` | `sub_82C22990` | `sub_82C2AA48` | `sub_82BA6458` | matched 1.00 |
| `D3DDevice_SetDepthStencilSurface` | `sub_82C22D20` | `sub_82C2ADD8` | `sub_82BA67E8` | matched 1.00 |
| `D3DDevice_Resolve` | `sub_82C26820` | `sub_82C2E8D8` | `sub_82BAA2E8` | matched 1.00 |
| `D3DDevice_SetPixelShader` | `sub_82C27648` | `sub_82C2F700` | `sub_82BAB110` | matched 1.00 |
| `D3DDevice_SetVertexShader` | `sub_82C27850` | `sub_82C2F908` | `sub_82BAB318` | matched 1.00 |
| `D3DDevice_SetVertexDeclaration` | `sub_82C27A68` | `sub_82C2FB20` | `sub_82BAB530` | matched 1.00 |
| `D3DDevice_EndVertices` | `sub_82C281A8` | `sub_82C30260` | `sub_82BABC70` | matched 1.00 |
| `D3DDevice_BeginVertices` | `sub_82C281B8` | `sub_82C30270` | `sub_82BABC80` | matched 1.00 |
| `D3DDevice_DrawIndexedVertices` | `sub_82C28678` | `sub_82C30730` | `sub_82BAC140` | matched 1.00 |
| `D3DDevice_SynchronizeToPresentationInterval` | `sub_82C29F98` | `sub_82C32050` | `sub_82BADA60` | matched 1.00 |
| `D3DDevice_Swap` | `sub_82C2A458` | `sub_82C32510` | `sub_82BADF20` | matched 0.99, only `VdSwap` caller |
| `D3DDevice_Present` | `sub_82C2ABC0` | `sub_82C32C78` | `sub_82BAE688` | matched 1.00 (never called in WWE '12 / 2K15) |
| `D3DDevice_Clear` | `sub_82C2BF50` | `sub_82C34008` | `sub_82BAFA18` | matched 1.00 |
| `D3DIndexBuffer_Unlock` | `sub_82C1D740` | `sub_82C257F8` | `sub_82BA1208` | predicted (2K15 + 0x845F0) |
| `D3DDevice_SetScissorRect` | `sub_82C21F18` | `sub_82C29FD0` | `sub_82BA59E0` | predicted |
| `D3DVertexBuffer_Unlock` | `sub_82C23AE8` | `sub_82C2BBA0` | `sub_82BA75B0` | predicted |
| `D3DDevice_CreateVertexDeclaration` | `sub_82C27B70` | `sub_82C2FC28` | `sub_82BAB638` | predicted |
| `D3DDevice_CreatePixelShader` | `sub_82C27E80` | `sub_82C2FF38` | `sub_82BAB948` | predicted |
| `D3DDevice_CreateVertexShader` | `sub_82C28068` | `sub_82C30120` | `sub_82BABB30` | predicted |

Predicted functions checked in the instruction index of the TU code: all six start at a function boundary with a
plausible prologue and size (`VBUnlock` 4 instructions ending in a tail call, `IBUnlock` 38, `SetScissorRect` 62,
`CreateVertexDeclaration` 32, `CreatePixelShader` and `CreateVertexShader` 60 each, with the same shape).

## Method

1. `python -I tools/asmindex.py build` indexes the PowerPC instructions kept as comments in `generated/`.
   The same script pointed at `generated_tu/` gives the title update index.
2. `tools/match_d3d_functions.py <WWE '12 checkout> 82C10000 82C48000 [work/asm_tu.json]` ranks every function of the
   window around the `VdSwap` caller by `difflib` similarity of its mnemonics with the 16 functions named in
   `wwe12_d3d_names.h`.
3. The offset to 2K15 follows from the 16 matches, which all share the same difference.

## Still to do

* Confirm the 6 predicted functions with a call census in game (`gen_d3d_census.py` of the WWE '12 / 2K15 ports), as
  was done for 2K15: device pointer in `r3`, call rates, object types behind the pointers.
* `D3DDevice_DrawVertices` and the resource description getters are not identified (2K15 does not have them either).
* Device object offsets, shader container layout and vertex declaration reading are expected to follow 2K15's
  (same library), but they are not checked on 2K16.

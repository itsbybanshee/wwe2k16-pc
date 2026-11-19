// WWE 2K16 (title update 3 code) addresses for the XDK D3D functions that re:Blue's renderer hooks by name.
//
// The 2K16 title update library has exactly the layout of WWE 2K15's TU4 library (+0x845F0 for every function), so this
// is the 2K15 header shifted. 16 functions were matched against WWE '12's named functions (similarity 1.00, Swap 0.99,
// tools/match_d3d_functions.py); the other 6 (SetScissorRect, the two Unlock, the three Create*) are 2K15's + 0x845F0 and
// still wait for a call census in game. See docs/d3d_functions.md.
// Force-included into the native renderer sources, as wwe12_d3d_names.h is in WWE '12 PC.
#pragma once

#define WWE2K16_NATIVE 1

// Not identified in 2K15 yet: re:Blue compiles these hooks out.
#define SVR_NO_DRAW_VERTICES 1   // D3DDevice_DrawVertices
#define WWE12_NO_DESC_HOOKS 1    // the resource description getters

#define D3DDevice_Present                           sub_82C32C78  // 0.83 match; never called in the census
#define D3DDevice_Swap                              sub_82C32510  // the only VdSwap caller (base sub_82C2A598); also hooked by framerate_hook.cpp
#define D3DDevice_SynchronizeToPresentationInterval sub_82C32050  // 0.97
#define D3DDevice_Resolve                           sub_82C2E8D8  // the public Resolve (SvR's arguments): its only callee
                                                                   // sub_82C2D780 (0.83 match with SvR's) takes two more
                                                                   // arguments in r6/r7 (0 here), so hooking it read the
                                                                   // destination from the wrong register
#define D3DDevice_Clear                             sub_82C34008  // 1.00
#define D3DDevice_SetRenderTarget                   sub_82C2AA48  // 0.96
#define D3DDevice_SetDepthStencilSurface            sub_82C2ADD8  // 1.00
#define D3DDevice_SetScissorRect                    sub_82C29FD0  // 1.00
#define D3DDevice_BeginVertices                     sub_82C30270  // 0.92; 2K15 does use it (~2,000 calls/s)
#define D3DDevice_EndVertices                       sub_82C30260  // 1.00
#define D3DDevice_DrawIndexedVertices               sub_82C30730  // 0.95; the hottest draw (~67,000 calls/s in a scene)
#define D3DVertexBuffer_Unlock                      sub_82C2BBA0  // 1.00
#define D3DIndexBuffer_Unlock                       sub_82C257F8  // 1.00
#define D3DDevice_CreateVertexDeclaration           sub_82C2FC28  // 1.00
#define D3DDevice_CreateVertexShader                sub_82C30120  // 1.00 (290 shaders created by the title screen)
#define D3DDevice_CreatePixelShader                 sub_82C2FF38  // 1.00 (399 created)
#define D3DDevice_SetTexture                        sub_82C27890  // 1.00
#define D3DDevice_SetVertexShader                   sub_82C2F908  // 1.00
#define D3DDevice_SetPixelShader                    sub_82C2F700  // 1.00
#define D3DDevice_SetVertexDeclaration              sub_82C2FB20  // 1.00
#define D3DDevice_SetStreamSource                   sub_82C2A118  // 1.00
#define D3DDevice_SetIndices                        sub_82C2A2C0  // 1.00

// Original bodies (__imp__ = the recompiled function, for hooks that call through).
#define __imp__D3DDevice_Present                           __imp__sub_82C32C78
#define __imp__D3DDevice_Swap                              __imp__sub_82C32510
#define __imp__D3DDevice_SynchronizeToPresentationInterval __imp__sub_82C32050
#define __imp__D3DDevice_Resolve                           __imp__sub_82C2E8D8
#define __imp__D3DDevice_Clear                             __imp__sub_82C34008
#define __imp__D3DDevice_SetRenderTarget                   __imp__sub_82C2AA48
#define __imp__D3DDevice_SetDepthStencilSurface            __imp__sub_82C2ADD8
#define __imp__D3DDevice_SetScissorRect                    __imp__sub_82C29FD0
#define __imp__D3DDevice_BeginVertices                     __imp__sub_82C30270
#define __imp__D3DDevice_EndVertices                       __imp__sub_82C30260
#define __imp__D3DDevice_DrawIndexedVertices               __imp__sub_82C30730
#define __imp__D3DVertexBuffer_Unlock                      __imp__sub_82C2BBA0
#define __imp__D3DIndexBuffer_Unlock                       __imp__sub_82C257F8
#define __imp__D3DDevice_CreateVertexDeclaration           __imp__sub_82C2FC28
#define __imp__D3DDevice_CreateVertexShader                __imp__sub_82C30120
#define __imp__D3DDevice_CreatePixelShader                 __imp__sub_82C2FF38
#define __imp__D3DDevice_SetTexture                        __imp__sub_82C27890
#define __imp__D3DDevice_SetVertexShader                   __imp__sub_82C2F908
#define __imp__D3DDevice_SetPixelShader                    __imp__sub_82C2F700
#define __imp__D3DDevice_SetVertexDeclaration              __imp__sub_82C2FB20
#define __imp__D3DDevice_SetStreamSource                   __imp__sub_82C2A118
#define __imp__D3DDevice_SetIndices                        __imp__sub_82C2A2C0

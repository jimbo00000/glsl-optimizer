/**************************************************************************
 *
 * Copyright 2010 VMware, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 **************************************************************************/


#ifndef LP_STATE_FS_H_
#define LP_STATE_FS_H_


#include "pipe/p_compiler.h"
#include "pipe/p_state.h"
#include "tgsi/tgsi_scan.h" /* for tgsi_shader_info */
#include "gallivm/lp_bld_sample.h" /* for struct lp_sampler_static_state */


struct tgsi_token;
struct lp_fragment_shader;


/** Indexes into jit_function[] array */
#define RAST_WHOLE 0
#define RAST_EDGE_TEST 1


struct lp_fragment_shader_variant_key
{
   struct pipe_depth_state depth;
   struct pipe_stencil_state stencil[2];
   struct pipe_alpha_state alpha;
   struct pipe_blend_state blend;
   enum pipe_format zsbuf_format;
   unsigned nr_cbufs:8;
   unsigned flatshade:1;
   unsigned scissor:1;
   unsigned occlusion_count:1;

   struct {
      ubyte colormask;
   } cbuf_blend[PIPE_MAX_COLOR_BUFS];

   struct lp_sampler_static_state sampler[PIPE_MAX_SAMPLERS];
};


struct lp_fragment_shader_variant
{
   struct lp_fragment_shader_variant_key key;

   boolean opaque;

   LLVMValueRef function[2];

   lp_jit_frag_func jit_function[2];

   struct lp_fragment_shader_variant *next;

   /* For debugging/profiling purposes */
   unsigned no;
};


/** Subclass of pipe_shader_state */
struct lp_fragment_shader
{
   struct pipe_shader_state base;

   struct tgsi_shader_info info;

   struct lp_fragment_shader_variant *variants;

   /* For debugging/profiling purposes */
   unsigned no;
   unsigned variant_no;
};


#endif /* LP_STATE_FS_H_ */
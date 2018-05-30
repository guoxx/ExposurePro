/***************************************************************************
# Copyright (c) 2015, NVIDIA CORPORATION. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of NVIDIA CORPORATION nor the names of its
#    contributors may be used to endorse or promote products derived
#    from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY
# EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
# PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
# CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
# EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
# PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
# PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
# OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
***************************************************************************/

__import Helpers;

#define NUM_THREAD_ 16
#define NUM_SAMPLES_ 256 

Texture2D gImage : register(t0);
RWStructuredBuffer<int> stats : register(u0);

groupshared int gs_stats[NUM_SAMPLES_ * 4];

[numthreads(NUM_THREAD_, NUM_THREAD_, 1)]
void main(uint3 groupID : SV_GroupID, uint3 threadID : SV_GroupThreadID, uint3 dispatchThreadID : SV_DispatchThreadID)
{
    if (threadID.x == 0 && threadID.y == 0)
    {
        for (int i = 0; i < NUM_SAMPLES_ * 4; ++i)
        {
            gs_stats[i] = 0;
        }
    }
    GroupMemoryBarrierWithGroupSync();

    float imgWidth, imgHeight;
    gImage.GetDimensions(imgWidth, imgHeight);

    for (int x = threadID.x; x < imgWidth; x += NUM_THREAD_)
    {
        for (int y = threadID.y; y < imgHeight; y += NUM_THREAD_)
        {
            float3 color = gImage[int2(x, y)].rgb;
            color = linearToSRGB(color);
            //color = sRGBToLinear(color);
            float lum = calcLuminance(color);

            int idxLum = min(int(lum * NUM_SAMPLES_), NUM_SAMPLES_ - 1);
            int idxR = NUM_SAMPLES_ + min(int(color.r * NUM_SAMPLES_), NUM_SAMPLES_ - 1);
            int idxG = NUM_SAMPLES_ * 2 + min(int(color.g * NUM_SAMPLES_), NUM_SAMPLES_ - 1);
            int idxB = NUM_SAMPLES_ * 3 + min(int(color.b * NUM_SAMPLES_), NUM_SAMPLES_ - 1);

            int origValue;
            InterlockedAdd(gs_stats[idxLum], 1, origValue);
            InterlockedAdd(gs_stats[idxR], 1, origValue);
            InterlockedAdd(gs_stats[idxG], 1, origValue);
            InterlockedAdd(gs_stats[idxB], 1, origValue);
        }
    }

    GroupMemoryBarrierWithGroupSync();

    if (threadID.x == 0 && threadID.y == 0)
    {
        for (int i = 0; i < NUM_SAMPLES_ * 4; ++i)
        {
            stats[i] = gs_stats[i];
        }
    }
}

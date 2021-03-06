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
#pragma once
#include "Falcor.h"
#include "SampleTest.h"
using namespace Falcor;

class ExposurePlayground : public Renderer
{
public:
    void onLoad(SampleCallbacks* pSample, RenderContext::SharedPtr pRenderContext) override;
    void onFrameRender(SampleCallbacks* pSample, RenderContext::SharedPtr pRenderContext, Fbo::SharedPtr pTargetFbo) override;
    void onResizeSwapChain(SampleCallbacks* pSample, uint32_t width, uint32_t height) override;
    void onGuiRender(SampleCallbacks* pSample, Gui* pGui) override;

private:
    void resizeHdrFbo(uint32_t width, uint32_t height);

    enum HdrImage
    {
        Radiometry,
        Photometry,
    };

    enum HistogramMode
    {
        None,
        Luminance = 0x01,
        R = 0x01 << 1,
        G = 0x01 << 2,
        B = 0x01 << 3,
        All = Luminance | R | G | B,
    };

    HistogramMode mHistogramMode = None;
    float mHistogramScaleFactor = 2.0f;

    std::string mHdrFilename;
    bool mIsHdrImageSRGB = false;
    HdrImage mHdrImageUnit = Photometry;
    Texture::SharedPtr mpHdrImage;

    GraphicsVars::SharedPtr mpPassthroughProgVars = nullptr;
    FullScreenPass::UniquePtr mpPassthroughPass;

    enum
    {
        ImageAnalysisSamples = 256,

        HistogramWindowWidth = 480,
        HistogramWindowHeight = 240,
        HistogramWindowRightMargin = 20,
        HistogramWindowTopMargin = 20,

        HistogramGraphWidth = 400,
        HistogramGraphHeight = 100,
    };
    ComputeState::SharedPtr mpImageAnalysisState;
    ComputeProgram::SharedPtr mpImageAnalysisProg;
    ComputeVars::SharedPtr mpImageAnalysisVars;
    StructuredBuffer::SharedPtr mpImageAnalysisBuffer;

    static const Gui::DropdownList kImageList;
    static const Gui::DropdownList kHistogramList;

    Fbo::SharedPtr mpHdrFbo;
    ToneMapping::UniquePtr mpToneMapper;

    bool loadImage(SampleCallbacks* pSample);

    void renderHistogram(Gui* pGui) const;
};

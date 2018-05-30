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
#include "ExposurePlayground.h"

using namespace Falcor;

const Gui::DropdownList ExposurePlayground::kImageList = {
    { HdrImage::Radiometry, "Radiometry" },
    { HdrImage::Photometry, "Photometry" },
};

const Gui::DropdownList ExposurePlayground::kHistogramList = {
    { HistogramMode::None, "None" },
    { HistogramMode::All, "All" },
    { HistogramMode::Luminance, "Luminance" },
    { HistogramMode::R, "R" },
    { HistogramMode::G, "G" },
    { HistogramMode::B, "B" },
};

void ExposurePlayground::onLoad(SampleCallbacks* pSample, RenderContext::SharedPtr pRenderContext)
{
    mpPassthroughPass = FullScreenPass::create("ExposurePro.ps.slang");

    //Program
    mpPassthroughProgVars = GraphicsVars::create(mpPassthroughPass->getProgram()->getActiveVersion()->getReflector());
    
    //Sampler
    Sampler::Desc samplerDesc;
    samplerDesc.setFilterMode(Sampler::Filter::Linear, Sampler::Filter::Linear, Sampler::Filter::Linear);
    const auto samplerState = Sampler::create(samplerDesc);
    mpPassthroughProgVars->setSampler("gSampler", samplerState);

    mpToneMapper = ToneMapping::create(ToneMapping::Operator::HableUc2);
    mpToneMapper->setExposureKey(0.104f);

    mpImageAnalysisProg = ComputeProgram::createFromFile("ImageAnalysis.cs.slang", "main");
    mpImageAnalysisVars = ComputeVars::create(mpImageAnalysisProg->getActiveVersion()->getReflector());
    mpImageAnalysisBuffer = StructuredBuffer::create(mpImageAnalysisProg, "stats", ImageAnalysisSamples * 4);

    mpImageAnalysisVars->setStructuredBuffer("stats", mpImageAnalysisBuffer);

    mpImageAnalysisState = ComputeState::create();
    mpImageAnalysisState->setProgram(mpImageAnalysisProg);

    loadImage(pSample);
}

bool ExposurePlayground::loadImage(SampleCallbacks* pSample)
{
    if (mHdrFilename.length() > 0)
    {
        mpHdrImage = createTextureFromFile(mHdrFilename, false, mIsHdrImageSRGB, Resource::BindFlags::ShaderResource);
        resizeHdrFbo(mpHdrImage->getWidth(), mpHdrImage->getHeight());
        pSample->resizeSwapChain(mpHdrImage->getWidth(), mpHdrImage->getHeight());
        return true;
    }
    return false;
}

void ExposurePlayground::renderHistogram(Gui * pGui) const
{
    int* pData = (int*)mpImageAnalysisBuffer->map(Buffer::MapType::Read);

    int32_t flags[4] = { HistogramMode::Luminance, HistogramMode::R, HistogramMode::G, HistogramMode::B };
    int32_t offset[4] = { 0, ImageAnalysisSamples, ImageAnalysisSamples * 2, ImageAnalysisSamples * 3 };
    const char* desc[4] = { "Luminance", "R", "G", "B" };
    glm::vec3 graphColor[4] = { glm::vec3(0.7, 0.7, 0.7), glm::vec3(1, 0, 0), glm::vec3(0, 1, 0), glm::vec3(0, 0, 1) };

    float scaleMax = 0.0f;
    for (int i = 1; i < arraysize(flags); ++i)
    {
        // only analysis RGB component for scaleMax
        float sum = 0.0f;
        float cnt = 0.0f;

        int* pCurData = pData + offset[i];
        for (int j = 0; j < ImageAnalysisSamples; ++j)
        {
            float fValue = (float)(pCurData[j]);
            if (fValue > 0)
            {
                sum += fValue;
                cnt += 1;
            }
        }
        scaleMax = std::max(scaleMax, sum / cnt);
    }

    for (int i = 0; i < arraysize(flags); ++i)
    {
        if (mHistogramMode & flags[i])
        {
            std::vector<float> vec;
            vec.reserve(ImageAnalysisSamples);
            int* pCurData = pData + offset[i];
            for (int j = 0; j < ImageAnalysisSamples; ++j)
            {
                float fValue = (float)(pCurData[j]);
                vec.push_back(fValue);
            }
            pGui->addHistogram(desc[i], vec, nullptr, HistogramGraphWidth, HistogramGraphHeight, 0, scaleMax * mHistogramScaleFactor, graphColor[i]);
        }
    }

    mpImageAnalysisBuffer->unmap();
}

void ExposurePlayground::onGuiRender(SampleCallbacks* pSample, Gui* pGui)
{
    if (pGui->beginGroup("Scene Referred Image"))
    {
        if (pGui->addButton("Load HDR Image"))
        {
            if (openFileDialog(nullptr, mHdrFilename))
            {
                loadImage(pSample);
            }
        }

        if (pGui->addCheckBox("sRGB", mIsHdrImageSRGB))
        {
            loadImage(pSample);
        }

        uint32_t uHdrUnit = static_cast<uint32_t>(mHdrImageUnit);
        if (pGui->addDropdown("Radiometry/Photometry", kImageList, uHdrUnit))
        {
            mHdrImageUnit = static_cast<HdrImage>(uHdrUnit);
        }

        pGui->endGroup();
    }

    mpToneMapper->renderUI(pGui, "HDR");

    if (pGui->beginGroup("Statistics"))
    {
        uint32_t histogramModeIdx = (uint32_t)mHistogramMode;
        pGui->addDropdown("Histogram", kHistogramList, histogramModeIdx);
        mHistogramMode = (HistogramMode)histogramModeIdx;

        pGui->addFloatVar("Graph Scale", mHistogramScaleFactor, 0.1f, 10.0f, 0.1f);

        pGui->endGroup();
    }

    if (mHistogramMode != HistogramMode::None)
    {
        pGui->pushWindow("Histogram",
            HistogramWindowWidth,
            HistogramWindowHeight,
            pSample->getCurrentFbo()->getWidth() - HistogramWindowRightMargin - HistogramWindowWidth, HistogramWindowTopMargin,
            false);

        renderHistogram(pGui);

        pGui->popWindow();
    }
}

void ExposurePlayground::onFrameRender(SampleCallbacks* pSample, RenderContext::SharedPtr pRenderContext, Fbo::SharedPtr pTargetFbo)
{
    const glm::vec4 clearColor(0.38f, 0.52f, 0.10f, 1);
    pRenderContext->clearFbo(mpHdrFbo.get(), clearColor, 1.0f, 0, FboAttachmentType::All);

    if (mpHdrImage)
    {
        pRenderContext->getGraphicsState()->pushFbo(mpHdrFbo);
        pRenderContext->pushGraphicsVars(mpPassthroughProgVars);
        mpPassthroughProgVars->setTexture("gTexture", mpHdrImage);
        mpPassthroughProgVars["PerFrameCB"]["gHdrImageUnit"] = static_cast<int32_t>(mHdrImageUnit);
        mpPassthroughPass->execute(pRenderContext.get());
        pRenderContext->popGraphicsVars();
        pRenderContext->getGraphicsState()->popFbo();
    }

    //Run tone mapping
    mpToneMapper->execute(pRenderContext.get(), mpHdrFbo, pTargetFbo);

    {
        pRenderContext->clearUAV(mpImageAnalysisBuffer->getUAV(0).get(), vec4(0));
        mpImageAnalysisVars->setTexture("gImage", pTargetFbo->getColorTexture(0));
        pRenderContext->pushComputeState(mpImageAnalysisState);
        pRenderContext->pushComputeVars(mpImageAnalysisVars);
        pRenderContext->dispatch(1, 1, 1);
        pRenderContext->popComputeVars();
        pRenderContext->popComputeState();
    }

    std::string txt = pSample->getFpsMsg() + '\n';
    pSample->renderText(txt, glm::vec2(10, 10));
}

void ExposurePlayground::resizeHdrFbo(uint32_t width, uint32_t height)
{
    //recreate hdr fbo
    ResourceFormat format = ResourceFormat::RGBA32Float;
    Fbo::Desc desc;
    desc.setDepthStencilTarget(ResourceFormat::D16Unorm);
    desc.setColorTarget(0u, format);
    mpHdrFbo = FboHelper::create2D(width, height, desc);
}

void ExposurePlayground::onResizeSwapChain(SampleCallbacks* pSample, uint32_t width, uint32_t height)
{
    if (mpHdrFbo == nullptr)
    {
        // only resize it if HDR fbo is not created
        resizeHdrFbo(width, height);
    }
}

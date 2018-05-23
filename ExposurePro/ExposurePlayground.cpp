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

const Gui::DropdownList ExposurePlayground::kImageList = { { HdrImage::EveningSun, "Evening Sun" },
                                                    { HdrImage::AtTheWindow, "Window" },
                                                    { HdrImage::OvercastDay, "Overcast Day" } };

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

    loadImage(pSample);
}

bool ExposurePlayground::loadImage(SampleCallbacks* pSample)
{
    if (mHdrFilename.length() > 0)
    {
        mpHdrImage = createTextureFromFile(mHdrFilename, false, mIsHdrImageSRGB, Resource::BindFlags::ShaderResource);
        pSample->resizeSwapChain(mpHdrImage->getWidth(), mpHdrImage->getHeight());
        return true;
    }
    return false;
}

void ExposurePlayground::onGuiRender(SampleCallbacks* pSample, Gui* pGui)
{
    if (pGui->beginGroup("Input HDR Image"))
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

        pGui->endGroup();
    }

    mpToneMapper->renderUI(pGui, "HDR");
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
        mpPassthroughPass->execute(pRenderContext.get());
        pRenderContext->popGraphicsState();
        pRenderContext->getGraphicsState()->popFbo();
    }

    //Run tone mapping
    mpToneMapper->execute(pRenderContext.get(), mpHdrFbo, pTargetFbo);

    std::string txt = pSample->getFpsMsg() + '\n';
    pSample->renderText(txt, glm::vec2(10, 10));
}

void ExposurePlayground::onResizeSwapChain(SampleCallbacks* pSample, uint32_t width, uint32_t height)
{
    //recreate hdr fbo
    ResourceFormat format = ResourceFormat::RGBA32Float;
    Fbo::Desc desc;
    desc.setDepthStencilTarget(ResourceFormat::D16Unorm);
    desc.setColorTarget(0u, format);
    mpHdrFbo = FboHelper::create2D(width, height, desc);
}

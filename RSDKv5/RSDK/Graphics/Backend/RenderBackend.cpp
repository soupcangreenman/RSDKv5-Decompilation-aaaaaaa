#include "../../Core/RetroEngine.hpp"

using namespace RSDK;

// always include the software render backend
#include "./Software/SoftwareBackend.cpp"

RenderBackend* RSDK::renderBackend;

bool32 RSDK::InitRenderBackend() {
  // TODO: eventually, allow switching render backends from the settings file

  // software-only for now
  renderBackend = new SoftwareBackend();
  if (renderBackend == 0)
    return false;

  renderBackend->InitBackend();

  return true;
}

void RSDK::FreeRenderBackend() {
  renderBackend->FreeBackend();
  delete renderBackend;
}



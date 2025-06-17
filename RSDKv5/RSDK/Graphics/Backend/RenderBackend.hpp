#ifndef RENDER_BACKEND
#define RENDER_BACKEND

// idk I'm just guessing
#define BACKEND_VERT_MAX (64)

// bitmasks used to determine available rendering backends depending on platform
#define RENDER_BACKEND_SW  (0b00000001)
#define RENDER_BACKEND_C3D (0b00000010)

#if RETRO_PLATFORM == RETRO_3DS
#define RENDER_BACKENDS (RENDER_BACKEND_SW | RENDER_BACKEND_C3D)
#endif

namespace RSDK {

extern uint8 renderBackendType;

class RenderBackend {
  public:
    virtual bool InitBackend() = 0;
    virtual void FreeBackend() = 0;

    
    virtual void FillScreen(uint8 drawGroup, uint32 color, int32 alphaR, int32 alphaG, 
                            int32 alphaB) = 0;

    virtual void DrawLine(uint8 drawGroup, int32 x1, int32 y1, int32 x2, 
                  int32 y2, uint32 color, int32 alpha, int32 inkEffect, 
                  bool32 screenRelative) = 0; 
    virtual void DrawRectangle(uint8 drawGroup, int32 x, int32 y, int32 width, 
                  int32 height, uint32 color, 
                  int32 alpha, int32 inkEffect, bool32 screenRelative) = 0;
    virtual void DrawCircle(uint8 drawGroup, int32 x, int32 y, int32 radius, 
                  uint32 color, int32 alpha, int32 inkEffect, 
                  bool32 screenRelative) = 0;
    virtual void DrawCircleOutline(uint8 drawGroup, int32 x, int32 y, int32 innerRadius, 
                  int32 outerRadius, uint32 color, 
                  int32 alpha, int32 inkEffect, bool32 screenRelative) = 0;

    virtual void DrawFace(uint8 drawGroup, Vector2 *vertices, int32 vertCount, 
                  int32 r, int32 g, int32 b, 
                  int32 alpha, int32 inkEffect) = 0;
    virtual void DrawBlendedFace(uint8 drawGroup, Vector2 *vertices, uint32 *colors, 
                  int32 vertCount, int32 alpha, 
                  int32 inkEffect) = 0;

    virtual void DrawSpriteFlipped(uint8 drawGroup, int32 x, int32 y, int32 width, 
                  int32 height, int32 sprX, int32 sprY, 
                  int32 direction, int32 inkEffect, int32 alpha, int32 sheetID) = 0;
    virtual void DrawSpriteRotozoom(uint8 drawGroup, int32 x, int32 y, int32 pivotX, 
                  int32 pivotY, int32 width, int32 height, int32 sprX, int32 sprY, 
                  int32 scaleX, int32 scaleY, int32 direction, int16 rotation, 
                  int32 inkEffect, int32 alpha, int32 sheetID) = 0;

    virtual void DrawDeformedSprite(uint8 drawGroup, uint16 sheetID, int32 inkEffect, 
                  int32 alpha) = 0;

    virtual void DrawTile(uint8 drawGroup, uint16 *tiles, int32 countX, int32 countY, 
                  Vector2 *position, 
                  Vector2 *offset, bool32 screenRelative) = 0;
    virtual void DrawAniTile(uint8 drawGroup, uint16 sheetID, uint16 tileIndex, uint16 srcX, 
                  uint16 srcY, uint16 width, uint16 height) = 0;

    virtual bool32 LoadSpritesheetData(void* bufPtr, int id) = 0;
  protected:
    Vector2 vertRefs[BACKEND_VERT_MAX];
};

extern RenderBackend* renderBackend;

bool32 InitRenderBackend();
void FreeRenderBackend();

// always include the software render backend
#include "./Software/SoftwareBackend.hpp"

} // ! namespace RSDK

#endif

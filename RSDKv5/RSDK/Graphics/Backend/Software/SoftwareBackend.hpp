#ifndef SOFTWARE_BACKEND_HPP
#define SOFTWARE_BACKEND_HPP

class SoftwareBackend : public RenderBackend {
  public:
    bool InitBackend() { return true; };
    void FreeBackend() { };
    
    void FillScreen(uint8 drawGroup, uint32 color, int32 alphaR, int32 alphaG, int32 alphaB);

    void DrawLine(uint8 drawGroup, int32 x1, int32 y1, int32 x2, 
                  int32 y2, uint32 color, int32 alpha, int32 inkEffect, 
                  bool32 screenRelative); 
    void DrawRectangle(uint8 drawGroup, int32 x, int32 y, int32 width, 
                  int32 height, uint32 color, 
                  int32 alpha, int32 inkEffect, bool32 screenRelative);
    void DrawCircle(uint8 drawGroup, int32 x, int32 y, int32 radius, 
                  uint32 color, int32 alpha, int32 inkEffect, 
                  bool32 screenRelative);
    void DrawCircleOutline(uint8 drawGroup, int32 x, int32 y, int32 innerRadius, 
                  int32 outerRadius, uint32 color, 
                  int32 alpha, int32 inkEffect, bool32 screenRelative);

    void DrawFace(uint8 drawGroup, Vector2 *vertices, int32 vertCount, 
                  int32 r, int32 g, int32 b, 
                  int32 alpha, int32 inkEffect);
    void DrawBlendedFace(uint8 drawGroup, Vector2 *vertices, uint32 *colors, 
                  int32 vertCount, int32 alpha, 
                  int32 inkEffect);

    void DrawSpriteFlipped(uint8 drawGroup, int32 x, int32 y, int32 width, 
                  int32 height, int32 sprX, int32 sprY, 
                  int32 direction, int32 inkEffect, int32 alpha, int32 sheetID);
    void DrawSpriteRotozoom(uint8 drawGroup, int32 x, int32 y, int32 pivotX, 
                  int32 pivotY, int32 width, int32 height, int32 sprX, int32 sprY, 
                  int32 scaleX, int32 scaleY, int32 direction, int16 rotation, 
                  int32 inkEffect, int32 alpha, int32 sheetID);

    void DrawDeformedSprite(uint8 drawGroup, uint16 sheetID, int32 inkEffect, 
                  int32 alpha);

    void DrawTile(uint8 drawGroup, uint16 *tiles, int32 countX, int32 countY, 
                  Vector2 *position, 
                  Vector2 *offset, bool32 screenRelative);
    void DrawAniTile(uint8 drawGroup, uint16 sheetID, uint16 tileIndex, uint16 srcX, 
                  uint16 srcY, uint16 width, uint16 height);

    bool32 LoadSpritesheetData(void* bufPtr, int id);

  protected:
    // TODO: stuff. i'm stuff
};

#endif

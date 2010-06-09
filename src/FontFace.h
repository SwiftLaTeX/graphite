#ifndef FONTFACE_INCLUDE
#define FONTFACE_INCLUDE

#include "GlyphFace.h"
#include "TtfUtil.h"
#include "graphiteng/Types.h"

#define ktiCmap MAKE_TAG('c','m','a','p')
#define ktiHead MAKE_TAG('h','e','a','d')
#define ktiGlyf MAKE_TAG('g','l','y','f')
#define ktiHdmx MAKE_TAG('h','d','m','x')
#define ktiHhea MAKE_TAG('h','h','e','a')
#define ktiHmtx MAKE_TAG('h','m','t','x')
#define ktiLoca MAKE_TAG('l','o','c','a')
#define ktiKern MAKE_TAG('k','e','r','n')
#define ktiMaxp MAKE_TAG('m','a','x','p')
#define ktiName MAKE_TAG('n','a','m','e')
#define ktiOs2  MAKE_TAG('O','S','/','2')
#define ktiPost MAKE_TAG('p','o','s','t')
#define ktiFeat MAKE_TAG('F','e','a','t')
#define ktiGlat MAKE_TAG('G','l','a','t')
#define ktiGloc MAKE_TAG('G','l','o','c')
#define ktiSilf MAKE_TAG('S','i','l','f')
#define ktiSile MAKE_TAG('S','i','l','e')
#define ktiSill MAKE_TAG('S','i','l','l')

class FontFace
{
public:
    virtual void *getTable(unsigned int name, size_t *len) = 0;
    virtual float pixelAdvance(unsigned short id, float ppm);
    virtual void readGlyphs();

public:
    GlyphFace *glyph(unsigned short glyphid) { return m_glyphs + glyphid; } // m_glyphidx[glyphid]; }
    float getAdvance(unsigned short glyphid, float scale) { return pixelAdvance(glyphid, scale * m_upem); }
    unsigned short upem() { return m_upem; }
    unsigned short numGlyphs() { return m_numglyphs; }

protected:

    unsigned short m_numglyphs;     // number of glyphs in the font
    // unsigned short *m_glyphidx;     // index for each glyph id in the font
    // unsigned short m_readglyphs;    // how many glyphs have we in m_glyphs?
    // unsigned short m_capacity;      // how big is m_glyphs
    GlyphFace *m_glyphs;            // list of actual glyphs indexed by glyphidx, 1 base
    unsigned short m_upem;          // design units per em
};

#endif // FONTFACE_INCLUDE
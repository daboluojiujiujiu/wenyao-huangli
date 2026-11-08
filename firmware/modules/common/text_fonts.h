#pragma once
#include "font_fallback_data.h"

namespace TextFonts {
class CoveredFont : public lgfx::IFont {
  const lgfx::IFont& base;
  const lgfx::IFont& traditional;
  const lgfx::IFont& japanese;

  const lgfx::IFont& select(uint16_t code) const {
    lgfx::FontMetrics metric{};
    if(base.updateFontMetric(&metric,code)) return base;
    if(traditional.updateFontMetric(&metric,code)) return traditional;
    if(japanese.updateFontMetric(&metric,code)) return japanese;
    return base;
  }
public:
  CoveredFont(const lgfx::IFont& b,const lgfx::IFont& t,const lgfx::IFont& j)
    : base(b), traditional(t), japanese(j) {}
  font_type_t getType() const override { return ft_u8g2; }
  void getDefaultMetric(lgfx::FontMetrics* m) const override { base.getDefaultMetric(m); }
  bool updateFontMetric(lgfx::FontMetrics* m,uint16_t c) const override {
    return select(c).updateFontMetric(m,c);
  }
  size_t drawChar(lgfx::LGFXBase* g,int32_t x,int32_t y,uint16_t c,
                  const lgfx::TextStyle* s,lgfx::FontMetrics* m,int32_t& filled) const override {
    return select(c).drawChar(g,x,y,c,s,m,filled);
  }
};

static const CoveredFont cn10(fonts::efontCN_10,fallback_tw_10,fallback_ja_10);
static const CoveredFont cn12(fonts::efontCN_12,fallback_tw_12,fallback_ja_12);
static const CoveredFont cn14(fonts::efontCN_14,fallback_tw_14,fallback_ja_14);
static const CoveredFont cn16(fonts::efontCN_16,fallback_tw_16,fallback_ja_16);
static const CoveredFont cn24(fonts::efontCN_24,fallback_tw_24,fallback_ja_24);
}

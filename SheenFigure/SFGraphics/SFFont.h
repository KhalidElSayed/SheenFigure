/*
 * Copyright (C) 2013 SheenFigure
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _SF_FONT_H
#define _SF_FONT_H

#include "SFConfig.h"
#include "SFTypes.h"

#ifndef _SF_FONT_PRIVATE_H
#define _SF_FONT_REF

typedef void *SFFontRef;

#endif

#ifdef SF_IOS_CG

#include <CoreGraphics/CoreGraphics.h>

SFFontRef SFFontCreateWithCGFont(CGFontRef cgFont, SFFloat size);
SFFontRef SFFontMakeCloneForCGFont(SFFontRef sfFont, CGFontRef cgFont, SFFloat size);
CGFontRef SFFontGetCGFont(SFFontRef sfFont);

#else

#include <ft2build.h>
#include <freetype/freetype.h>

SFFontRef SFFontCreateWithFTFace(FT_Face ftFace, SFFloat size);
SFFontRef SFFontMakeCloneForFTFace(SFFontRef sfFont, FT_Face ftFace, SFFloat size);
FT_Face SFFontGetFTFace(SFFontRef sfFont);

#endif

SFFloat SFFontGetSize(SFFontRef sfFont);
SFFloat SFFontGetSizeByEm(SFFontRef sfFont);

SFFloat SFFontGetAscender(SFFontRef sfFont);
SFFloat SFFontGetDescender(SFFontRef sfFont);
SFFloat SFFontGetLeading(SFFontRef sfFont);

SFFontRef SFFontRetain(SFFontRef sfFont);
void SFFontRelease(SFFontRef sfFont);

#endif

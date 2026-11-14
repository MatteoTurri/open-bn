/*
 * RawImgGenerator: factory functions for the RawImg subclasses.
 */

#ifndef RAWIMGGENERATOR_H
#define RAWIMGGENERATOR_H

#include "rawimg.h"

namespace RawImgGenerator {
RawImg *GenerateCompatible(RawImg *src);
RawImg *Generate(RawImg::RawImgType type);
RawImg *GenerateSizedImage(RawImg::RawImgType type, int w, int h);
RawImg *GenerateClone(RawImg *src);
}

#endif

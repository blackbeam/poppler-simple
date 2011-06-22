//========================================================================
//
// ImageQualityDev.cc
//
// Copyright 1998-2003 Glyph & Cog, LLC
//
//========================================================================

//========================================================================
//
// Modified under the Poppler project - http://poppler.freedesktop.org
//
// All changes made under the Poppler project to this file are licensed
// under GPL version 2 or later
//
// Copyright (C) 2005, 2007 Albert Astals Cid <aacid@kde.org>
// Copyright (C) 2006 Rainer Keller <class321@gmx.de>
// Copyright (C) 2008 Timothy Lee <timothy.lee@siriushk.com>
// Copyright (C) 2008 Vasile Gaburici <gaburici@cs.umd.edu>
// Copyright (C) 2009 Carlos Garcia Campos <carlosgc@gnome.org>
// Copyright (C) 2009 William Bader <williambader@hotmail.com>
// Copyright (C) 2010 Jakob Voss <jakob.voss@gbv.de>
//
// To see a description of the changes please see the Changelog file that
// came with your tarball or type make ChangeLog if you are building from git
//
//========================================================================

#include "config.h"
#include <poppler-config.h>

#ifdef USE_GCC_PRAGMAS
#pragma implementation
#endif

#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <ctype.h>
#include "goo/gmem.h"
#include "Error.h"
#include "GfxState.h"
#include "Object.h"
#include "Stream.h"
#ifdef ENABLE_LIBJPEG
#include "DCTStream.h"
#endif
#include "ImageQualityDev.h"

ImageQualityDev::ImageQualityDev(double phisical_width, PopplerImageMapping* mapping, v8::Local<v8::Object> out ) {
  this->mapping = mapping;
  this->phisical_width = phisical_width;
  this->out = out;
  imgNum = 0;
  pageNum = 0;
  ok = gTrue;
}

ImageQualityDev::~ImageQualityDev() {
}

void ImageQualityDev::drawImageMask(GfxState *state, Object *ref, Stream *str,
				   int width, int height, GBool invert,
				   GBool interpolate, GBool inlineImg) {
  double PPI = (mapping->area.x2 - mapping->area.x1) / phisical_width;
  enum StreamKind kind = str->getKind();
   
  if ( kind != strCCITTFax && kind != strDCT ) {
    out->Set(v8::String::NewSymbol("quality"), v8::String::New("UNNECESSARY_FILTERS"));
    switch ( kind ) {
      case strFile:
        out->Set(v8::String::NewSymbol("strKind"), v8::String::New("strFile"));
        break;
      case strCachedFile:
        out->Set(v8::String::NewSymbol("strKind"), v8::String::New("strCachedFile"));
        break;
      case strASCIIHex:
        out->Set(v8::String::NewSymbol("strKind"), v8::String::New("ASCIIHexDecode"));
        break;
      case strASCII85:
        out->Set(v8::String::NewSymbol("strKind"), v8::String::New("ASCII85Decode"));
        break;
      case strLZW:
        out->Set(v8::String::NewSymbol("strKind"), v8::String::New("LZWDecode"));
        break;
      case strRunLength:
        out->Set(v8::String::NewSymbol("strKind"), v8::String::New("RunLengthDecode"));
        break;
      case strCCITTFax:
        out->Set(v8::String::NewSymbol("strKind"), v8::String::New("CCITTFaxDecode"));
        break;
      case strDCT:
        out->Set(v8::String::NewSymbol("strKind"), v8::String::New("DCTDecode"));
        break;
      case strFlate:
        out->Set(v8::String::NewSymbol("strKind"), v8::String::New("FlateDecode"));
        break;
      case strJBIG2:
        out->Set(v8::String::NewSymbol("strKind"), v8::String::New("JBIG2Decode"));
        break;
      case strJPX:
        out->Set(v8::String::NewSymbol("strKind"), v8::String::New("JPXDecode"));
        break;
      case strWeird:
        out->Set(v8::String::NewSymbol("strKind"), v8::String::New("WeirdDecode"));
    }
  } else if ( kind == strDCT && PPI > 140.0 ) {
    out->Set(v8::String::NewSymbol("quality"), v8::String::New("TOO_BIG"));
    out->Set(v8::String::NewSymbol("PPI"), v8::Number::New( PPI ));
  } else {
    out->Set(v8::String::NewSymbol("quality"), v8::String::New("NORMAL"));
  }
}

void ImageQualityDev::drawImage(GfxState *state, Object *ref, Stream *str,
			       int width, int height,
			       GfxImageColorMap *colorMap,
			       GBool interpolate, int *maskColors, GBool inlineImg) {
  double PPI = (mapping->area.x2 - mapping->area.x1) / phisical_width;
  enum StreamKind kind = str->getKind();
  
  if ( kind != strCCITTFax && kind != strDCT ) {
    out->Set(v8::String::NewSymbol("quality"), v8::String::New("UNNECESSARY_FILTERS"));
    switch ( kind ) {
      case strFile:
        out->Set(v8::String::NewSymbol("strKind"), v8::String::New("strFile"));
        break;
      case strCachedFile:
        out->Set(v8::String::NewSymbol("strKind"), v8::String::New("strCachedFile"));
        break;
      case strASCIIHex:
        out->Set(v8::String::NewSymbol("strKind"), v8::String::New("strASCIIHex"));
        break;
      case strASCII85:
        out->Set(v8::String::NewSymbol("strKind"), v8::String::New("strASCII85"));
        break;
      case strLZW:
        out->Set(v8::String::NewSymbol("strKind"), v8::String::New("strLZW"));
        break;
      case strRunLength:
        out->Set(v8::String::NewSymbol("strKind"), v8::String::New("strRunLength"));
        break;
      case strCCITTFax:
        out->Set(v8::String::NewSymbol("strKind"), v8::String::New("strCCITTFax"));
        break;
      case strDCT:
        out->Set(v8::String::NewSymbol("strKind"), v8::String::New("strDCT"));
        break;
      case strFlate:
        out->Set(v8::String::NewSymbol("strKind"), v8::String::New("strFlate"));
        break;
      case strJBIG2:
        out->Set(v8::String::NewSymbol("strKind"), v8::String::New("strJBIG2"));
        break;
      case strJPX:
        out->Set(v8::String::NewSymbol("strKind"), v8::String::New("strJPX"));
        break;
      case strWeird:
        out->Set(v8::String::NewSymbol("strKind"), v8::String::New("strWeird"));
    }
  } else if ( kind == strDCT && PPI > 140.0 ) {
    out->Set(v8::String::NewSymbol("quality"), v8::String::New("TOO_BIG"));
    out->Set(v8::String::NewSymbol("PPI"), v8::Number::New( PPI ));
  } else {
    out->Set(v8::String::NewSymbol("quality"), v8::String::New("NORMAL"));
  }
}

void ImageQualityDev::drawMaskedImage(
    GfxState *state, Object *ref, Stream *str,
    int width, int height, GfxImageColorMap *colorMap, GBool interpolate,
    Stream *maskStr, int maskWidth, int maskHeight, GBool maskInvert, GBool maskInterpolate) {
  drawImage(state, ref, str, width, height, colorMap, interpolate, NULL, gFalse);
  drawImageMask(state, ref, maskStr, maskWidth, maskHeight, maskInvert,
      maskInterpolate, gFalse);
}

void ImageQualityDev::drawSoftMaskedImage(
    GfxState *state, Object *ref, Stream *str,
    int width, int height, GfxImageColorMap *colorMap, GBool interpolate,
    Stream *maskStr, int maskWidth, int maskHeight,
    GfxImageColorMap *maskColorMap, GBool maskInterpolate) {
  drawImage(state, ref, str, width, height, colorMap, interpolate, NULL, gFalse);
  drawImage(state, ref, maskStr, maskWidth, maskHeight,
      maskColorMap, maskInterpolate, NULL, gFalse);
}

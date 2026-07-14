#pragma once

#include "alchemy/led/panel.h"

namespace autophage {
namespace palette {

using Rgb = alchemy::LedPanel::Rgb;

/* Base Palette */
constexpr Rgb kBrickEmber = {0xD0, 0x00, 0x00};
constexpr Rgb kAmber = {0xFF, 0xBA, 0x08};
constexpr Rgb kOrange = {0xFF, 0x40, 0x00};
constexpr Rgb kSteelBlue = {0x3F, 0x88, 0xC5};
constexpr Rgb kDeepSpaceBlue = {0x03, 0x2B, 0x43};
constexpr Rgb kPurple = {128, 0, 128};
constexpr Rgb kSpruceBlue = {0x13, 0x6F, 0x63};
constexpr Rgb kPaleGreen = {0x62, 0x9C, 0x14};
constexpr Rgb kWhite = {0xFF, 0xFF, 0xFF};
constexpr Rgb kOff = {0x00, 0x00, 0x00};
constexpr Rgb kDimWhite = {0x10, 0x10, 0x10};

/* Semantic Knobs - Page 1 */
constexpr Rgb kFold = kBrickEmber;
constexpr Rgb kOffsetPos = kAmber;
constexpr Rgb kOffsetNeg = kDeepSpaceBlue;
constexpr Rgb kOffsetCenter = kWhite;
constexpr Rgb kSymmetry = kPaleGreen;

/* Semantic Knobs - Page 2 */
constexpr Rgb kFeedback = kPurple;
constexpr Rgb kDistortion = kOrange;
constexpr Rgb kFilter = kSpruceBlue;

/* Button Pips */
constexpr Rgb kBtnStereoLink = kPaleGreen;
constexpr Rgb kBtnDistPre = {0x80, 0x20, 0x00};   // Dim Orange
constexpr Rgb kBtnDistPost = {0x09, 0x37, 0x31};  // Dim Spruce Blue
constexpr Rgb kBtnFilterLp = {0x68, 0x00, 0x00};  // Dim Brick Ember
constexpr Rgb kBtnFilterBp = {0x80, 0x20, 0x00};  // Dim Orange
constexpr Rgb kBtnFilterHp = {0x09, 0x37, 0x31};  // Dim Spruce Blue
constexpr Rgb kBtnBypass = {0x80, 0x80, 0x80};    // Grey

}  // namespace palette
}  // namespace autophage

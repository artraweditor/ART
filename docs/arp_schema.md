# ART Raw Profile (.arp) Schema Specification

This document specifies the configuration groups and keys found in `.arp` files.

## [General]
- `Rank`
- `ColorLabel`
- `InTrash`

## [Exposure]
- `Enabled`
- `Compensation`
- `Black`
- `HLRecovery`
- `HLRecoveryBlur`

## [Saturation]
- `Enabled`
- `Saturation`
- `Vibrance`

## [ToneCurve]
- `Enabled`
- `Contrast`
- `HistogramMatching`
- `CurveFromHistogramMatching`
- `CurveMode`
- `CurveMode2`
- `Curve`
- `Curve2`
- `Saturation`
- `Saturation2`
- `PerceptualStrength`
- `ContrastLegacyMode`
- `WhitePoint`
- `BaseCurve`

## [Local Contrast]
- `Enabled`
- `ShowMask`
- `SelectedRegion`

## [Channel Mixer]
- `Enabled`
- `Mode`

## [Black & White]
- `Enabled`
- `Setting`
- `Filter`
- `MixerRed`
- `MixerGreen`
- `MixerBlue`
- `GammaRed`
- `GammaGreen`
- `GammaBlue`
- `ColorCast`

## [HSL Equalizer]
- `Enabled`
- `HCurve`
- `SCurve`
- `LCurve`
- `Smoothing`

## [Luminance Curve]
- `Enabled`
- `Brightness`
- `Contrast`
- `Chromaticity`
- `LCurve`
- `aCurve`
- `bCurve`

## [Sharpening]
- `Enabled`
- `Contrast`
- `Method`
- `Radius`
- `Amount`
- `Threshold`
- `OnlyEdges`
- `EdgedetectionRadius`
- `EdgeTolerance`
- `HalocontrolEnabled`
- `HalocontrolAmount`
- `DeconvRadius`
- `DeconvAmount`
- `DeconvAutoRadius`
- `DeconvCornerBoost`
- `DeconvCornerLatitude`
- `PSFKernel`
- `PSFIterations`

## [White Balance]
- `Enabled`
- `Setting`
- `Temperature`
- `Green`
- `Equal`
- `Multipliers`

## [Impulse Denoising]
- `Enabled`
- `Threshold`

## [Defringing]
- `Enabled`
- `Radius`
- `Threshold`
- `HueCurve`

## [Dehaze]
- `Enabled`
- `Strength`
- `Blackpoint`
- `Luminance`
- `Depth`
- `ShowDepthMap`

## [Denoise]
- `Enabled`
- `ColorSpace`
- `Aggressive`
- `Gamma`
- `Luminance`
- `LuminanceDetail`
- `LuminanceDetailThreshold`
- `ChrominanceMethod`
- `ChrominanceAutoFactor`
- `Chrominance`
- `ChrominanceRedGreen`
- `ChrominanceBlueYellow`
- `SmoothingEnabled`
- `GuidedChromaRadius`
- `NLDetail`
- `NLStrength`

## [TextureBoost]
- `Enabled`
- `ShowMask`
- `SelectedRegion`

## [FattalToneMapping]
- `Enabled`
- `Threshold`
- `Amount`
- `SaturationControl`

## [LogEncoding]
- `Enabled`
- `Auto`
- `AutoGain`
- `Gain`
- `TargetGray`
- `BlackEv`
- `WhiteEv`
- `Regularization`
- `SaturationControl`
- `HighlightCompression`

## [ToneEqualizer]
- `Enabled`
- `Band`
- `Regularization`
- `Pivot`

## [Crop]
- `Enabled`
- `X`
- `Y`
- `W`
- `H`
- `FixedRatio`
- `Ratio`
- `Orientation`
- `Guide`

## [Coarse Transformation]
- `Rotate`
- `HorizontalFlip`
- `VerticalFlip`

## [Common Properties for Transformations]
- `AutoFill`

## [Rotation]
- `Enabled`
- `Degree`

## [Distortion]
- `Enabled`
- `Amount`
- `Auto`

## [LensProfile]
- `LcMode`
- `LCPFile`
- `UseDistortion`
- `UseVignette`
- `UseCA`
- `LFCameraMake`
- `LFCameraModel`
- `LFLens`

## [Perspective]
- `Enabled`
- `Horizontal`
- `Vertical`
- `Angle`
- `Shear`
- `FocalLength`
- `CropFactor`
- `Aspect`
- `ControlLines`

## [Gradient]
- `Enabled`
- `Degree`
- `Feather`
- `Strength`
- `CenterX`
- `CenterY`

## [PCVignette]
- `Enabled`
- `Strength`
- `Feather`
- `Roundness`
- `CenterX`
- `CenterY`

## [CACorrection]
- `Enabled`
- `Red`
- `Blue`

## [Vignetting Correction]
- `Enabled`
- `Amount`
- `Radius`
- `Strength`
- `CenterX`
- `CenterY`

## [Resize]
- `Enabled`
- `Scale`
- `AppliesTo`
- `DataSpecified`
- `Width`
- `Height`
- `AllowUpscaling`
- `PPI`
- `CopyPPIToExif`
- `Unit`

## [OutputSharpening]
- `Enabled`
- `Contrast`
- `Method`
- `Radius`
- `Amount`
- `Threshold`
- `OnlyEdges`
- `EdgedetectionRadius`
- `EdgeTolerance`
- `HalocontrolEnabled`
- `HalocontrolAmount`
- `DeconvRadius`
- `DeconvAmount`

## [Color Management]
- `InputProfile`
- `ToneCurve`
- `ApplyLookTable`
- `ApplyBaselineExposureOffset`
- `ApplyHueSatMap`
- `DCPIlluminant`
- `DCPLookEarly`
- `WorkingProfile`
- `OutputProfile`
- `OutputProfileIntent`
- `OutputBPC`
- `InputProfileCAT`

## [SoftLight]
- `Enabled`
- `Strength`

## [Film Simulation]
- `Enabled`
- `ClutFilename`
- `Strength`
- `AfterToneCurve`
- `ClutParams`

## [RGB Curves]
- `Enabled`
- `rCurve`
- `gCurve`
- `bCurve`

## [Grain]
- `Enabled`
- `ISO`
- `Strength`
- `Color`

## [Smoothing]
- `Enabled`
- `ShowMask`
- `SelectedRegion`

## [ColorCorrection]
- `Enabled`
- `ShowMask`
- `SelectedRegion`

## [RAW]
- `DarkFrameEnabled`
- `DarkFrame`
- `DarkFrameAuto`
- `FlatFieldEnabled`
- `FlatFieldFile`
- `FlatFieldAutoSelect`
- `FlatFieldBlurRadius`
- `FlatFieldBlurType`
- `FlatFieldAutoClipControl`
- `FlatFieldClipControl`
- `FlatFieldUseEmbedded`
- `CAEnabled`
- `CA`
- `CAAvoidColourshift`
- `CAAutoIterations`
- `CARed`
- `CABlue`
- `HotDeadPixelEnabled`
- `HotPixelFilter`
- `DeadPixelFilter`
- `HotDeadPixelThresh`
- `PreExposureEnabled`
- `PreExposure`

## [RAW Bayer]
- `Border`
- `ImageNum`
- `CcSteps`
- `PreBlackEnabled`
- `PreBlack0`
- `PreBlack1`
- `PreBlack2`
- `PreBlack3`
- `PreTwoGreen`
- `PreprocessingEnabled`
- `LineDenoise`
- `LineDenoiseDirection`
- `GreenEqThreshold`
- `DCBIterations`
- `LMMSEIterations`
- `DualDemosaicAutoContrast`
- `DualDemosaicContrast`
- `PixelShiftEperIso`
- `PixelShiftSigma`
- `PixelShiftShowMotion`
- `PixelShiftShowMotionMaskOnly`
- `pixelShiftHoleFill`
- `pixelShiftMedian`
- `pixelShiftGreen`
- `pixelShiftBlur`
- `pixelShiftSmoothFactor`
- `pixelShiftEqualBright`
- `pixelShiftEqualBrightChannel`
- `pixelShiftNonGreenCross`
- `pixelShiftDemosaicMethod`
- `PDAFLinesFilter`
- `DynamicRowNoiseFilter`

## [RAW X-Trans]
- `Method`
- `DualDemosaicAutoContrast`
- `DualDemosaicContrast`
- `Border`
- `CcSteps`
- `PreBlackEnabled`
- `PreBlackRed`
- `PreBlackGreen`
- `PreBlackBlue`

## [Film Negative]
- `Enabled`
- `RedRatio`
- `GreenExponent`
- `BlueRatio`
- `RedBase`
- `GreenBase`
- `BlueBase`
- `ColorSpace`
- `RefInput`
- `RefOutput`
- `BackCompat`

## [MetaData]
- `Mode`
- `ExifKeys`
- `Notes`

## [Spot Removal]
- `Enabled`


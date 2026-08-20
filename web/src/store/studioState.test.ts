import { describe, expect, it } from 'vitest';
import { OFFICIAL_PRESETS, SAMPLE_MEDIA } from './studioState';

describe('Studio Presets & Media', () => {
  it('defines 4 official presets with valid step structures', () => {
    expect(OFFICIAL_PRESETS).toHaveLength(4);

    const fastSwap = OFFICIAL_PRESETS.find((p) => p.id === 'fast_swap');
    expect(fastSwap).toBeDefined();
    expect(fastSwap?.steps[0].step).toBe('face_swapper');

    const multiFace = OFFICIAL_PRESETS.find((p) => p.id === 'multi_face_swap');
    expect(multiFace).toBeDefined();
    // Multi-face swap has at least two face_swapper steps
    const swappers = multiFace?.steps.filter((s) => s.step === 'face_swapper');
    expect(swappers?.length).toBeGreaterThanOrEqual(2);
  });

  it('contains valid built-in sample media', () => {
    expect(SAMPLE_MEDIA.sources.length).toBeGreaterThan(0);
    expect(SAMPLE_MEDIA.targets.length).toBeGreaterThan(0);
    expect(SAMPLE_MEDIA.sources[0].path).toContain('lenna.bmp');
  });

  it('constructs valid pipeline payload structure', () => {
    const preset = OFFICIAL_PRESETS[1]; // hd_portrait
    const payload = {
      source_paths: ['s.jpg'],
      target_paths: ['t.jpg'],
      pipeline_steps: preset.steps.map((s) => ({
        step: s.step,
        name: s.name,
        enabled: true,
        params: s.params,
      })),
    };

    expect(payload.pipeline_steps).toHaveLength(3);
    expect(payload.pipeline_steps[0].step).toBe('face_swapper');
    expect(payload.pipeline_steps[1].step).toBe('face_enhancer');
    expect(payload.pipeline_steps[2].step).toBe('expression_restorer');
  });
});

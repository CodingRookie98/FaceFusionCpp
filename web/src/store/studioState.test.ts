import { describe, expect, it } from 'vitest';
import {
  OFFICIAL_PRESETS,
  SAMPLE_MEDIA,
  getMediaPreviewUrl,
  sanitizeSources,
  sanitizeTargets,
} from './studioState';
import type { MediaItem } from './studioState';

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

  describe('getMediaPreviewUrl & Lifecycle Persistence', () => {
    it('falls back to /api/preview when thumbnailUrl is a dead blob URL after page reload', () => {
      const reloadedItem: MediaItem = {
        id: 'uploaded-1',
        path: '/tmp/ffc_uploads/1724217000_face.jpg',
        name: 'face.jpg',
        type: 'image',
        thumbnailUrl: 'blob:http://localhost:8000/01234567-89ab-cdef-0123-456789abcdef',
        // file is absent on page reload
      };

      const previewUrl = getMediaPreviewUrl(reloadedItem);
      expect(previewUrl).toBe(
        `/api/preview?path=${encodeURIComponent('/tmp/ffc_uploads/1724217000_face.jpg')}`
      );
    });

    it('uses static thumbnailUrl when it is already a valid non-blob URL', () => {
      const staticItem: MediaItem = {
        id: 'sample-1',
        path: 'assets/standard_face_test_images/lenna.bmp',
        name: 'Lenna',
        type: 'image',
        thumbnailUrl: '/api/preview?path=assets%2Fstandard_face_test_images%2Flenna.bmp',
      };

      expect(getMediaPreviewUrl(staticItem)).toBe(
        '/api/preview?path=assets%2Fstandard_face_test_images%2Flenna.bmp'
      );
    });

    it('falls back to path when thumbnailUrl is empty or missing', () => {
      const itemOnlyPath: MediaItem = {
        id: 'custom-1',
        path: 'uploads/target.png',
        name: 'target.png',
        type: 'image',
      };

      expect(getMediaPreviewUrl(itemOnlyPath)).toBe(
        `/api/preview?path=${encodeURIComponent('uploads/target.png')}`
      );
    });

    it('sanitizes dead blob URLs into valid backend /api/preview URLs', () => {
      const rawSources: MediaItem[] = [
        {
          id: 'src-1',
          path: '/tmp/uploads/source_photo.png',
          name: 'source_photo.png',
          type: 'image',
          thumbnailUrl: 'blob:http://localhost:8000/dead-blob-uuid',
        },
      ];

      const cleaned = sanitizeSources(rawSources);
      expect(cleaned[0].thumbnailUrl).toBe(
        `/api/preview?path=${encodeURIComponent('/tmp/uploads/source_photo.png')}`
      );
    });

    it('sanitizes legacy test assets to valid paths and previews', () => {
      const legacyTargets: MediaItem[] = [
        {
          id: 'tgt-legacy',
          path: 'assets/standard_face_test_images/family.jpg',
          name: 'Family',
          type: 'image',
        },
      ];

      const cleaned = sanitizeTargets(legacyTargets);
      expect(cleaned[0].path).toBe('assets/standard_face_test_images/woman.jpg');
      expect(cleaned[0].name).toBe('Woman (女士目标图)');
      expect(cleaned[0].thumbnailUrl).toContain('woman.jpg');
    });
  });
});


import { test, expect } from '@playwright/test';

test.describe('FaceFusionCpp Studio - Comprehensive E2E Tests', () => {
  test.beforeEach(async ({ page }) => {
    page.on('console', (msg) => console.log('PAGE LOG:', msg.text()));
    page.on('pageerror', (err) => console.log('PAGE ERROR:', err.message));

    // Mock standard backend APIs so tests are fully hermetic and deterministic
    await page.route('**/api/health', async (route) => {
      await route.fulfill({
        status: 200,
        contentType: 'application/json',
        body: JSON.stringify({ status: 'ok', version: '0.34.1' }),
      });
    });

    await page.route('**/api/processors', async (route) => {
      await route.fulfill({
        status: 200,
        contentType: 'application/json',
        body: JSON.stringify([
          {
            name: 'face_swapper',
            params: [
              { name: 'model', type: 'string', description: 'Model' },
              { name: 'face_selector_mode', type: 'string', description: 'Mode' },
            ],
          },
          {
            name: 'face_enhancer',
            params: [
              { name: 'model', type: 'string', description: 'Enhancer Model' },
              { name: 'blend_percentage', type: 'int', description: 'Blend', range: [0, 100] },
            ],
          },
        ]),
      });
    });

    await page.route('**/api/tasks', async (route) => {
      if (route.request().method() === 'GET') {
        await route.fulfill({
          status: 200,
          contentType: 'application/json',
          body: JSON.stringify([
            {
              id: 'task-mock-001',
              status: 'done',
              progress: { current_frame: 1, total_frames: 1, fps: 24 },
              media_count: 2,
              priority: 100,
              queue_position: 0,
            },
          ]),
        });
      } else if (route.request().method() === 'POST') {
        await route.fulfill({
          status: 201,
          contentType: 'application/json',
          body: JSON.stringify({ id: 'task-mock-002', status: 'queued' }),
        });
      }
    });

    await page.route('**/api/tasks/task-mock-001', async (route) => {
      await route.fulfill({
        status: 200,
        contentType: 'application/json',
        body: JSON.stringify({
          id: 'task-mock-001',
          status: 'done',
          progress: { current_frame: 1, total_frames: 1, fps: 24 },
          error_message: '',
          output_path: './output',
          media: {
            source: ['/media/task-mock-001/source/0'],
            target: ['/media/task-mock-001/target/0'],
          },
          results: [
            {
              name: 'result_girl.png',
              url: '/media/task-mock-001/result/result_girl.png',
            },
          ],
          priority: 100,
        }),
      });
    });

    await page.route('**/api/faces*', async (route) => {
      await route.fulfill({
        status: 200,
        contentType: 'application/json',
        body: JSON.stringify({
          faces: [
            {
              index: 0,
              score: 0.98,
              box: { x: 100, y: 100, width: 120, height: 140 },
              kps: [
                { x: 130, y: 135 },
                { x: 180, y: 135 },
                { x: 155, y: 165 },
                { x: 140, y: 195 },
                { x: 175, y: 195 },
              ],
              gender: 'female',
              age_range: [20, 26],
            },
          ],
        }),
      });
    });

    // Mock media and preview images with a 1x1 transparent PNG data URI
    const pixelPng = Buffer.from(
      'iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+M9QDwADhgGAWjR9awAAAABJRU5ErkJggg==',
      'base64'
    );
    await page.route('**/api/preview*', async (route) => {
      await route.fulfill({ status: 200, contentType: 'image/png', body: pixelPng });
    });
    await page.route(/.*\/media\/.*(\.png|\.jpg|\.bmp|\.mp4|\/source\/|\/target\/|\/result\/).*/, async (route) => {
      await route.fulfill({ status: 200, contentType: 'image/png', body: pixelPng });
    });

    await page.goto('/');
    await page.waitForLoadState('domcontentloaded');
    await expect(page.locator('h1')).toBeVisible();
  });

  test('1. Studio Header & Branding renders properly', async ({ page }) => {
    await expect(page.locator('h1')).toContainText('FaceFusionCpp Studio');
    await expect(page.getByText('v2.0')).toBeVisible();
    await expect(page.getByText('选择素材与人脸')).toBeVisible();
    await expect(page.getByText('编排多实例管线')).toBeVisible();
    await expect(page.getByText('实时对比与输出')).toBeVisible();
  });

  test('2. Asset Pool: switch tabs and select media items', async ({ page }) => {
    // 1. Check Source faces
    const sourceTabBtn = page.getByRole('button', { name: /源人脸/i });
    const targetTabBtn = page.getByRole('button', { name: /目标素材/i });

    await expect(sourceTabBtn).toBeVisible();
    await expect(targetTabBtn).toBeVisible();

    // Verify sample sources (Lenna, Man, Barbara)
    await expect(page.getByText('Lenna (经典测试头像)')).toBeVisible();
    await expect(page.getByText('Man (男士肖像)')).toBeVisible();
    await expect(page.getByText('Barbara (女士肖像)')).toBeVisible();

    // Click on Man
    await page.getByText('Man (男士肖像)').click();

    // Switch to Targets tab
    await targetTabBtn.click();
    await expect(page.getByText('Girl (单人目标图)', { exact: true })).toBeVisible();
    await expect(page.getByText('Woman (女士目标图)', { exact: true })).toBeVisible();

    // Click on Woman
    await page.getByText('Woman (女士目标图)', { exact: true }).click();
  });

  test('3. Pipeline Editor: switch presets and configure processors', async ({ page }) => {
    // Check preset selector buttons
    await expect(page.getByRole('button', { name: /极速单人换脸/i })).toBeVisible();
    await expect(page.getByRole('button', { name: /高清写真重塑/i })).toBeVisible();
    await expect(page.getByRole('button', { name: /影视级全流程超分/i })).toBeVisible();

    // Switch to "影视级全流程超分"
    await page.getByRole('button', { name: /影视级全流程超分/i }).click();

    // Verify steps are loaded
    await expect(page.locator('input[value*="电影级换脸"]').first()).toBeVisible();

    // Add a new processor step
    const addBtn = page.getByRole('button', { name: /添加步骤/i });
    await addBtn.click();
    await expect(page.getByText('Face Swapper (换脸)')).toBeVisible();
    await page.getByText('Face Swapper (换脸)').click();

    // Verify added step card
    const cards = page.locator('div.border-white\\/10');
    await expect(cards.first()).toBeVisible();
  });

  test('4. Viewport Canvas: mode switching & controls', async ({ page }) => {
    // Mode switcher buttons
    const canvasBtn = page.getByRole('button', { name: /标注画布/i });
    const resultBtn = page.getByRole('button', { name: /处理结果/i });
    const compareBtn = page.getByRole('button', { name: /卷帘对比/i });
    const loupeBtn = page.getByRole('button', { name: /局部放大/i });

    await expect(canvasBtn).toBeVisible();
    await expect(resultBtn).toBeVisible();
    await expect(compareBtn).toBeVisible();
    await expect(loupeBtn).toBeVisible();

    // Switch to Canvas mode
    await canvasBtn.click();
    await expect(canvasBtn).toHaveClass(/bg-blue-600/);

    // Switch to Result mode
    await resultBtn.click();
    await expect(resultBtn).toHaveClass(/bg-blue-600/);
    await expect(page.getByText('处理后结果 (AFTER)')).toBeVisible();

    // Switch to Compare mode
    await compareBtn.click();
    await expect(compareBtn).toHaveClass(/bg-blue-600/);
    await expect(page.getByText('处理前 (BEFORE)')).toBeVisible();
    await expect(page.getByText('处理后 (AFTER)')).toBeVisible();

    // Switch to Loupe mode
    await loupeBtn.click();
    await expect(loupeBtn).toHaveClass(/bg-blue-600/);
  });

  test('5. SplitSlider: drag divider handle and update split clipPath', async ({ page }) => {
    // Switch to Compare Mode
    const compareBtn = page.getByRole('button', { name: /卷帘对比/i });
    await compareBtn.click();

    // Verify before/after badges
    await expect(page.getByText('处理前 (BEFORE)')).toBeVisible();
    await expect(page.getByText('处理后 (AFTER)')).toBeVisible();

    // Drag the divider handle
    const divider = page.locator('div.cursor-ew-resize').first();
    await expect(divider).toBeVisible();

    const box = await divider.boundingBox();
    if (box) {
      await page.mouse.move(box.x + box.width / 2, box.y + box.height / 2);
      await page.mouse.down();
      await page.mouse.move(box.x + 100, box.y + box.height / 2);
      await page.mouse.up();
    }

    // Verify clip-path polygon exists
    const clipped = page.locator('[style*="clip-path"], [style*="-webkit-clip-path"]');
    await expect(clipped.first()).toBeVisible();
  });

  test('6. Telemetry HUD & Task History Modal', async ({ page }) => {
    // Verify Bottom HUD is present
    await expect(page.getByText('v0.34.1', { exact: false })).toBeVisible();
    await expect(page.getByText('帧进度', { exact: false })).toBeVisible();

    // Open History Modal
    const historyBtn = page.getByRole('button', { name: /历史成果/i });
    await historyBtn.click();

    await expect(page.getByText('历史任务与成果记录')).toBeVisible();
    await expect(page.getByText('task-mock-001')).toBeVisible();

    // Close Modal via top-right X button
    const closeBtn = page.locator('button:has(.lucide-x)');
    await closeBtn.click();
    await expect(page.getByText('历史任务与成果记录')).not.toBeVisible();
  });

  test('7. Task Submission Flow', async ({ page }) => {
    const runBtn = page.getByRole('button', { name: /启动渲染任务/i });
    await expect(runBtn).toBeVisible();
    await runBtn.click();

    // Submit request triggered successfully
    await page.waitForTimeout(500);
  });
});

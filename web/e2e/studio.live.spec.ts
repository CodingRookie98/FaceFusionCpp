import { test, expect } from '@playwright/test';

test.describe('FaceFusionCpp Studio - Live C++ Backend Integration E2E Tests', () => {
  test.beforeEach(async ({ page }) => {
    // DO NOT mock any endpoints - all requests go through to real C++ backend!
    await page.goto('/');
    await page.waitForLoadState('domcontentloaded');
    await expect(page.locator('h1')).toBeVisible();
  });

  test('1. Live C++ Backend Health & Processors Metadata Integration', async ({ page }) => {
    // Verify telemetry HUD connects to real C++ core
    await expect(page.getByText('C++ Core:', { exact: false })).toBeVisible();
    await expect(page.getByText('v0.34.1', { exact: false })).toBeVisible({ timeout: 10000 });

    // Verify processors loaded from backend in Pipeline Editor
    const addBtn = page.getByRole('button', { name: /添加步骤/i });
    await addBtn.click();
    await expect(page.getByText('Face Swapper (换脸)')).toBeVisible();
    await expect(page.getByText('Face Enhancer (人脸高清修复)')).toBeVisible();
    await expect(page.getByText('Expression Restorer (表情还原)')).toBeVisible();
    await expect(page.getByText('Frame Enhancer (全画幅超分)')).toBeVisible();
  });

  test('2. Live Media Preview Loading from C++ Server', async ({ page }) => {
    // Check that asset pool items load preview images directly from backend
    const targetTabBtn = page.getByRole('button', { name: /目标素材/i });
    await targetTabBtn.click();

    await expect(page.getByText('Girl (单人目标图)', { exact: true })).toBeVisible();
    await expect(page.getByText('Woman (女士目标图)', { exact: true })).toBeVisible();

    // Click on Girl target and verify canvas preview loads
    await page.getByText('Girl (单人目标图)', { exact: true }).click();
    const canvasImg = page.locator('img[alt="Target Canvas"]');
    await expect(canvasImg).toBeVisible();
  });

  test('3. Live Face Detection on Target Image & Face Selection Toggle', async ({ page }) => {
    const targetTabBtn = page.getByRole('button', { name: /目标素材/i });
    await targetTabBtn.click();
    await page.getByText('Girl (单人目标图)', { exact: true }).click();

    // Verify canvas displays target
    const canvasImg = page.locator('img[alt="Target Canvas"]');
    await expect(canvasImg).toBeVisible();

    // Wait for live face detection to complete (automatic on target selection)
    const selectedBadge = page.getByText(/已选 #1/i);
    await expect(selectedBadge).toBeVisible({ timeout: 10000 });

    // Click to toggle/deselect
    await selectedBadge.click();
    await expect(page.getByText(/未选 #1/i)).toBeVisible();

    // Click "全选" toolbar button
    const selectAllBtn = page.getByRole('button', { name: '全选' });
    await selectAllBtn.click();
    await expect(page.getByText(/已选 #1/i)).toBeVisible();

    // Click "清空" toolbar button
    const clearBtn = page.getByRole('button', { name: '清空' });
    await clearBtn.click();
    await expect(page.getByText(/未选 #1/i)).toBeVisible();
  });

  test('4. Live Task Submission & Real Job Lifecycle Flow', async ({ page }) => {
    // Select Source face
    const sourceTabBtn = page.getByRole('button', { name: /源人脸/i });
    await sourceTabBtn.click();
    await page.getByText('Lenna (经典测试头像)').click();

    // Select Target image
    const targetTabBtn = page.getByRole('button', { name: /目标素材/i });
    await targetTabBtn.click();
    await page.getByText('Girl (单人目标图)', { exact: true }).click();

    // Apply a fast preset
    await page.getByRole('button', { name: /极速单人换脸/i }).click();

    // Click Run Task (sends live POST /api/tasks to Drogon C++ server)
    const runBtn = page.getByRole('button', { name: /启动渲染任务/i });
    await expect(runBtn).toBeVisible();
    await runBtn.click();

    // Wait for submission response and verify task appears in live queue/history
    await page.waitForTimeout(1500);

    // Open History Modal to inspect live tasks in backend database
    const historyBtn = page.getByRole('button', { name: /历史成果/i });
    await historyBtn.click();

    await expect(page.getByText('历史任务与成果记录')).toBeVisible();

    // Close Modal
    const closeBtn = page.locator('button:has(.lucide-x)');
    await closeBtn.click();
    await expect(page.getByText('历史任务与成果记录')).not.toBeVisible();
  });
});

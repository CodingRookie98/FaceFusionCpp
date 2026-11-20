import { defineConfig } from 'vite';
import react from '@vitejs/plugin-react';
import tailwindcss from '@tailwindcss/vite';

// https://vite.dev/config/
// 后端地址可经环境变量配置：FFC_WEB_HOST（默认 127.0.0.1）、FFC_WEB_PORT（默认 8000），
// 与 `ffc --web --web-port <port>` 保持一致，避免硬编码端口导致开发代理断连。
const webHost = process.env.FFC_WEB_HOST ?? '127.0.0.1';
const webPort = process.env.FFC_WEB_PORT ?? '8000';

export default defineConfig({
  plugins: [react(), tailwindcss()],
  server: {
    port: 5173,
    proxy: {
      // 开发模式：API 与 WebSocket 与媒体请求代理到 C++ 服务（ffc --web）
      '/api': { target: `http://${webHost}:${webPort}`, changeOrigin: true },
      '/media': { target: `http://${webHost}:${webPort}`, changeOrigin: true },
      '/ws': { target: `ws://${webHost}:${webPort}`, ws: true },
    },
  },
  build: {
    outDir: 'dist',
    emptyOutDir: true,
  },
});

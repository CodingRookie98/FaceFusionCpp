import { Sparkles } from 'lucide-react';
import { useStudioStore } from './store/studioState';
import { AssetPool } from './components/media/AssetPool';
import { ViewportCanvas } from './components/canvas/ViewportCanvas';
import { PipelineEditor } from './components/pipeline/PipelineEditor';
import { TelemetryHUD } from './components/hud/TelemetryHUD';

export default function App() {
  const store = useStudioStore();

  return (
    <div className="flex flex-col h-screen w-screen bg-[#080b11] text-slate-100 overflow-hidden select-none font-sans">
      {/* Top Application Header */}
      <header className="h-12 bg-[#0a0f1a] border-b border-white/10 px-4 flex items-center justify-between flex-shrink-0 z-30">
        {/* Brand & Logo */}
        <div className="flex items-center gap-3">
          <div className="w-7 h-7 rounded-lg bg-gradient-to-br from-blue-600 via-cyan-500 to-emerald-400 p-[1.5px] shadow-[0_0_12px_rgba(59,130,246,0.5)]">
            <div className="w-full h-full bg-[#090d16] rounded-[6px] flex items-center justify-center">
              <Sparkles className="w-4 h-4 text-cyan-400" />
            </div>
          </div>

          <div>
            <div className="flex items-center gap-2">
              <h1 className="text-xs font-bold tracking-wider uppercase bg-gradient-to-r from-white via-slate-200 to-slate-400 bg-clip-text text-transparent">
                FaceFusionCpp Studio
              </h1>
              <span className="text-[10px] font-mono font-semibold bg-blue-950/80 text-blue-400 px-1.5 py-0.2 rounded border border-blue-500/30">
                v2.0
              </span>
            </div>
          </div>
        </div>

        {/* Center: Workflow Breadcrumb / Status */}
        <div className="hidden md:flex items-center gap-6 text-xs text-slate-400">
          <div className="flex items-center gap-2">
            <span className="w-5 h-5 rounded-full bg-blue-600/20 text-blue-400 text-[10px] font-bold flex items-center justify-center border border-blue-500/30">
              1
            </span>
            <span>选择素材与人脸</span>
          </div>

          <div className="w-4 h-px bg-white/10" />

          <div className="flex items-center gap-2">
            <span className="w-5 h-5 rounded-full bg-cyan-600/20 text-cyan-400 text-[10px] font-bold flex items-center justify-center border border-cyan-500/30">
              2
            </span>
            <span>编排多实例管线</span>
          </div>

          <div className="w-4 h-px bg-white/10" />

          <div className="flex items-center gap-2">
            <span className="w-5 h-5 rounded-full bg-emerald-600/20 text-emerald-400 text-[10px] font-bold flex items-center justify-center border border-emerald-500/30">
              3
            </span>
            <span>实时对比与输出</span>
          </div>
        </div>

        {/* Right Info */}
        <div className="flex items-center gap-3">
          <span className="text-[11px] text-slate-400 font-mono hidden sm:inline-block">
            C++20 · TensorRT / ONNX Acceleration
          </span>
        </div>
      </header>

      {/* Main Studio 3-Column Workspace */}
      <main className="flex-1 flex overflow-hidden relative">
        {/* Left: Asset Pool Sidebar */}
        <AssetPool store={store} />

        {/* Center: Viewport Canvas & Comparison Stage */}
        <ViewportCanvas store={store} />

        {/* Right: Dynamic Pipeline Editor Sidebar */}
        <PipelineEditor store={store} />
      </main>

      {/* Bottom Telemetry & Queue Management HUD */}
      <TelemetryHUD store={store} />
    </div>
  );
}

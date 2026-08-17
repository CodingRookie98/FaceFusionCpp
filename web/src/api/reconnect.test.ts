import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest';
import { computeReconnectDelay, subscribeProgress } from './client';
import type { WsMessage } from './types';

// ─────────────────────────────────────────────────────────────────────────
// computeReconnectDelay: pure exponential backoff helper
// ─────────────────────────────────────────────────────────────────────────
describe('computeReconnectDelay', () => {
  it('returns baseMs for attempt 0', () => {
    expect(computeReconnectDelay(0)).toBe(1000);
  });

  it('doubles per attempt', () => {
    expect(computeReconnectDelay(1)).toBe(2000);
    expect(computeReconnectDelay(2)).toBe(4000);
    expect(computeReconnectDelay(3)).toBe(8000);
  });

  it('caps at maxMs', () => {
    expect(computeReconnectDelay(10)).toBe(30000);
    expect(computeReconnectDelay(100)).toBe(30000);
  });

  it('respects custom base/max', () => {
    expect(computeReconnectDelay(0, 500, 10000)).toBe(500);
    expect(computeReconnectDelay(5, 500, 10000)).toBe(10000);
  });
});

// ─────────────────────────────────────────────────────────────────────────
// subscribeProgress: auto-reconnect on close, backoff reset on open
// ─────────────────────────────────────────────────────────────────────────

class FakeWebSocket {
  static instances: FakeWebSocket[] = [];
  url: string;
  onopen: (() => void) | null = null;
  onclose: (() => void) | null = null;
  onerror: (() => void) | null = null;
  onmessage: ((ev: { data: string }) => void) | null = null;
  close = vi.fn(() => {
    // Browser semantics: close() fires onclose asynchronously-ish.
    this.onclose?.();
  });

  constructor(url: string) {
    this.url = url;
    FakeWebSocket.instances.push(this);
  }

  emitOpen(): void {
    this.onopen?.();
  }

  emitClose(): void {
    this.onclose?.();
  }

  emitMessage(data: string): void {
    this.onmessage?.({ data });
  }
}

const WS_URL = 'ws://127.0.0.1:5173/ws/tasks/test-id/progress';

beforeEach(() => {
  vi.useFakeTimers();
  FakeWebSocket.instances = [];
  vi.stubGlobal('WebSocket', FakeWebSocket as unknown as typeof WebSocket);
  vi.stubGlobal('window', {
    location: { protocol: 'http:', hostname: '127.0.0.1', port: '5173' },
  });
});

afterEach(() => {
  vi.useRealTimers();
  vi.unstubAllGlobals();
});

describe('subscribeProgress', () => {
  it('opens a WebSocket to the task progress endpoint', () => {
    const onMessage = vi.fn();
    const unsubscribe = subscribeProgress('test-id', onMessage);
    expect(FakeWebSocket.instances).toHaveLength(1);
    expect(FakeWebSocket.instances[0].url).toBe(WS_URL);
    unsubscribe();
  });

  it('parses incoming frames and forwards them', () => {
    const onMessage = vi.fn();
    const unsubscribe = subscribeProgress('test-id', onMessage);
    const ws = FakeWebSocket.instances[0];
    ws.emitMessage('{"type":"progress","frame":5,"total":10,"fps":2.5}');
    expect(onMessage).toHaveBeenCalledWith({
      type: 'progress',
      frame: 5,
      total: 10,
      fps: 2.5,
    } as WsMessage);
    unsubscribe();
  });

  it('reconnects automatically after close with exponential backoff', () => {
    const onMessage = vi.fn();
    const unsubscribe = subscribeProgress('test-id', onMessage);
    const ws1 = FakeWebSocket.instances[0];

    ws1.emitClose();
    // attempt 0 -> 1000ms; nothing before that
    vi.advanceTimersByTime(999);
    expect(FakeWebSocket.instances).toHaveLength(1);
    vi.advanceTimersByTime(1);
    expect(FakeWebSocket.instances).toHaveLength(2);

    // second close -> attempt 1 -> 2000ms
    const ws2 = FakeWebSocket.instances[1];
    ws2.emitClose();
    vi.advanceTimersByTime(1999);
    expect(FakeWebSocket.instances).toHaveLength(2);
    vi.advanceTimersByTime(1);
    expect(FakeWebSocket.instances).toHaveLength(3);
    unsubscribe();
  });

  it('resets backoff after a successful open', () => {
    const onMessage = vi.fn();
    const unsubscribe = subscribeProgress('test-id', onMessage);
    const ws1 = FakeWebSocket.instances[0];

    ws1.emitOpen();
    ws1.emitClose();
    // attempt reset to 0 -> delay 1000ms (not 2000ms)
    vi.advanceTimersByTime(1000);
    expect(FakeWebSocket.instances).toHaveLength(2);

    const ws2 = FakeWebSocket.instances[1];
    ws2.emitOpen();
    ws2.emitClose();
    vi.advanceTimersByTime(1000);
    expect(FakeWebSocket.instances).toHaveLength(3);
    unsubscribe();
  });

  it('stops reconnecting after unsubscribe', () => {
    const onMessage = vi.fn();
    const unsubscribe = subscribeProgress('test-id', onMessage);
    const ws1 = FakeWebSocket.instances[0];

    unsubscribe();
    ws1.emitClose(); // late close after unsubscribe must not reconnect
    vi.advanceTimersByTime(5000);
    expect(FakeWebSocket.instances).toHaveLength(1);
  });
});

#!/usr/bin/env python3
"""E2E test for the Web UI REST/WS surface.

Usage: python3 web_api_test.py <executable>
Starts ffc --web on a random port, then exercises:
  health -> submit task -> list -> detail -> cancel -> static page
"""

import json
import subprocess
import sys
import time
import urllib.error
import urllib.request
from pathlib import Path


def http(method: str, url: str, body: dict | None = None) -> tuple[int, str]:
    data = json.dumps(body).encode() if body is not None else None
    req = urllib.request.Request(url, data=data, method=method)
    if data is not None:
        req.add_header("Content-Type", "application/json")
    try:
        with urllib.request.urlopen(req, timeout=10) as resp:
            return resp.status, resp.read().decode()
    except urllib.error.HTTPError as e:
        return e.code, e.read().decode()


def main() -> int:
    if len(sys.argv) < 2:
        print("Usage: web_api_test.py <executable>")
        return 1
    exe = Path(sys.argv[1]).resolve()
    port = 19000 + (__import__("os").getpid() % 1000)
    base = f"http://127.0.0.1:{port}"

    proc = subprocess.Popen([str(exe), "--web", "--web-port", str(port),
                             "--web-host", "127.0.0.1"],
                            stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                            cwd=exe.parent)
    try:
        # wait for health
        for _ in range(30):
            try:
                code, body = http("GET", f"{base}/api/health")
                if code == 200:
                    break
            except OSError:
                time.sleep(0.5)
        else:
            print("[FAIL] server did not become ready")
            return 1
        health = json.loads(body)
        assert health["status"] == "ok", f"unexpected health: {health}"
        print("[PASS] /api/health")

        # static page
        code, body = http("GET", f"{base}/")
        assert code == 200 and "ffc Web UI" in body, "index.html not served"
        print("[PASS] static index served")

        # submit
        code, body = http("POST", f"{base}/api/tasks", {
            "source_paths": ["assets/standard_face_test_images/lenna.bmp"],
            "target_paths": ["assets/standard_face_test_images/girl.bmp"],
        })
        assert code == 201, f"submit failed: {code} {body}"
        task_id = json.loads(body)["id"]
        print(f"[PASS] submit -> {task_id[:8]}...")

        # list contains it
        code, body = http("GET", f"{base}/api/tasks")
        assert code == 200
        ids = [t["id"] for t in json.loads(body)]
        assert task_id in ids, "task missing from list"
        print("[PASS] task listed")

        # detail
        code, body = http("GET", f"{base}/api/tasks/{task_id}")
        assert code == 200
        detail = json.loads(body)
        assert detail["status"] in ("queued", "running", "failed", "done", "cancelled")
        assert detail["media"]["source"], "media urls missing"
        print("[PASS] task detail (status=" + detail["status"] + ")")

        # cancel (idempotent for terminal states)
        code, body = http("POST", f"{base}/api/tasks/{task_id}/cancel")
        assert code == 200 and json.loads(body)["ok"] is True
        print("[PASS] cancel")

        # 404 on unknown task
        code, _ = http("GET", f"{base}/api/tasks/no_such_task")
        assert code == 404
        print("[PASS] unknown task -> 404")

        # upload a file
        upload_data = b"e2e-upload-bytes"
        req = urllib.request.Request(
            f"{base}/api/upload", data=upload_data, method="POST")
        req.add_header("X-File-Name", "e2e_sample.jpg")
        with urllib.request.urlopen(req, timeout=10) as resp:
            assert resp.status == 201, f"upload failed: {resp.status}"
            upload = json.loads(resp.read().decode())
        assert upload["name"] == "e2e_sample.jpg" and upload["size"] == len(upload_data)
        print("[PASS] upload -> " + upload["path"])

        # submit with the uploaded path
        code, body = http("POST", f"{base}/api/tasks", {
            "source_paths": [upload["path"]],
            "target_paths": [upload["path"]],
        })
        assert code == 201
        task2_id = json.loads(body)["id"]
        print("[PASS] submit with uploaded path -> " + task2_id[:8] + "...")

        # priority endpoint (task may already be terminal; accept 200 or 409)
        code, body = http("POST", f"{base}/api/tasks/{task2_id}/priority",
                          {"priority": 3})
        assert code in (200, 409), f"priority failed: {code} {body}"
        print(f"[PASS] set priority -> {code}")

        # list carries priority/queue_position fields
        code, body = http("GET", f"{base}/api/tasks")
        assert code == 200
        for t in json.loads(body):
            assert "priority" in t and "queue_position" in t
        print("[PASS] list includes priority/queue_position")

        # detect faces POST endpoint
        code, body = http("POST", f"{base}/api/faces", {
            "image_path": upload["path"],
        })
        assert code == 200, f"faces detect failed: {code} {body}"
        faces_res = json.loads(body)
        assert "faces" in faces_res and faces_res["image"] == upload["path"]
        print("[PASS] /api/faces POST")

        # detect faces GET endpoint
        code, body = http("GET", f"{base}/api/faces?image={upload['path']}")
        assert code == 200
        assert "faces" in json.loads(body)
        print("[PASS] /api/faces GET")

        # submit with reference face selector mode
        code, body = http("POST", f"{base}/api/tasks", {
            "source_paths": [upload["path"]],
            "target_paths": [upload["path"]],
            "processors": ["face_swapper"],
            "processor_params": {
                "face_swapper": {
                    "face_selector_mode": "reference",
                    "reference_face_path": upload["path"],
                },
            },
        })
        assert code == 201
        print("[PASS] submit with reference face selector mode")

        print("\nAll web API tests passed!")
        return 0
    finally:
        proc.terminate()
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            proc.kill()


if __name__ == "__main__":
    sys.exit(main())

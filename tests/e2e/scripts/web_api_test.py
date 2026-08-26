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
import urllib.parse
import urllib.request
from pathlib import Path


def http(method: str, url: str, body: dict | None = None, timeout: float = 60.0) -> tuple[int, str]:
    data = json.dumps(body).encode() if body is not None else None
    req = urllib.request.Request(url, data=data, method=method)
    if data is not None:
        req.add_header("Content-Type", "application/json")
    try:
        with urllib.request.urlopen(req, timeout=timeout) as resp:
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
        body = ""
        for _ in range(60):
            try:
                code, body = http("GET", f"{base}/api/health")
                if code == 200:
                    break
            except Exception:
                pass
            time.sleep(0.5)
        else:
            print("[FAIL] server did not become ready")
            return 1
        health = json.loads(body)
        assert health["status"] == "ok", f"unexpected health: {health}"
        print("[PASS] /api/health")

        # static page
        code, body = http("GET", f"{base}/")
        assert code == 200 and ("ffc" in body.lower() or "root" in body), f"index.html not served properly: {body}"
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

        # processors metadata endpoint
        code, body = http("GET", f"{base}/api/processors")
        assert code == 200, f"/api/processors failed: {code} {body}"
        processors = json.loads(body)
        assert isinstance(processors, list) and len(processors) > 0
        proc_names = [p["name"] for p in processors]
        assert "face_swapper" in proc_names
        print(f"[PASS] /api/processors -> {len(processors)} processors found")

        # progress endpoint
        code, body = http("GET", f"{base}/api/tasks/{task_id}/progress")
        assert code == 200, f"/api/tasks/{task_id}/progress failed: {code} {body}"
        prog_res = json.loads(body)
        assert "progress" in prog_res and "status" in prog_res
        print(f"[PASS] /api/tasks/{task_id}/progress")

        # Range request check on media
        detail_media = detail["media"]["source"]
        if detail_media:
            first_media_url = detail_media[0] if isinstance(detail_media[0], str) else detail_media[0]["url"]
            range_req = urllib.request.Request(f"{base}{first_media_url}", method="GET")
            range_req.add_header("Range", "bytes=0-3")
            try:
                with urllib.request.urlopen(range_req, timeout=10) as rresp:
                    assert rresp.status == 206, f"expected 206 for Range request, got {rresp.status}"
                    partial_data = rresp.read()
                    assert len(partial_data) == 4, f"expected 4 bytes, got {len(partial_data)}"
                    print("[PASS] media HTTP Range (206) response")
            except urllib.error.HTTPError as he:
                assert he.code == 206, f"expected 206 for Range request, got {he.code}"
                print("[PASS] media HTTP Range (206) response")

        # cancel (idempotent for terminal states)
        code, body = http("POST", f"{base}/api/tasks/{task_id}/cancel")
        assert code == 200 and json.loads(body)["ok"] is True
        print("[PASS] cancel")

        # 404 on unknown task
        code, _ = http("GET", f"{base}/api/tasks/no_such_task")
        assert code == 404
        print("[PASS] unknown task -> 404")

        # upload a real image file with a face (lenna.bmp)
        lenna_path = Path("assets/standard_face_test_images/lenna.bmp")
        if lenna_path.exists():
            upload_data = lenna_path.read_bytes()
        else:
            upload_data = (exe.parent / "assets/standard_face_test_images/lenna.bmp").read_bytes()

        req = urllib.request.Request(
            f"{base}/api/upload", data=upload_data, method="POST")
        req.add_header("X-File-Name", "lenna.bmp")
        with urllib.request.urlopen(req, timeout=10) as resp:
            assert resp.status == 201, f"upload failed: {resp.status}"
            upload = json.loads(resp.read().decode())
        assert upload["name"] == "lenna.bmp" and upload["size"] == len(upload_data)
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

        # detect faces POST endpoint on real image
        code, body = http("POST", f"{base}/api/faces", {
            "image_path": upload["path"],
        })
        assert code == 200, f"faces detect failed: {code} {body}"
        faces_res = json.loads(body)
        assert "faces" in faces_res and faces_res["image"] == upload["path"]
        assert len(faces_res["faces"]) >= 1, f"expected at least 1 face detected, got {faces_res['faces']}"
        face0 = faces_res["faces"][0]
        assert "box" in face0 and "kps" in face0 and "score" in face0
        print(f"[PASS] /api/faces POST detected {len(faces_res['faces'])} face(s)")

        # detect faces GET endpoint
        code, body = http("GET", f"{base}/api/faces?image={upload['path']}")
        assert code == 200
        get_faces_res = json.loads(body)
        assert len(get_faces_res["faces"]) >= 1
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

        # submit with structured multi-instance pipeline_steps
        code, body = http("POST", f"{base}/api/tasks", {
            "source_paths": [upload["path"]],
            "target_paths": [upload["path"]],
            "pipeline_steps": [
                {
                    "step": "face_swapper",
                    "name": "主角换脸",
                    "enabled": True,
                    "params": {
                        "model": "inswapper_128",
                        "face_selector_mode": "reference",
                        "reference_face_path": upload["path"],
                    },
                },
                {
                    "step": "face_swapper",
                    "name": "配角换脸",
                    "enabled": True,
                    "params": {
                        "model": "inswapper_128",
                        "face_selector_mode": "one",
                    },
                },
                {
                    "step": "face_enhancer",
                    "name": "面部高清修复",
                    "enabled": True,
                    "params": {
                        "model": "codeformer",
                        "blend_factor": 0.85,
                    },
                },
            ],
        })
        assert code == 201, f"multi-instance submit failed: {code} {body}"
        multi_step_id = json.loads(body)["id"]
        print(f"[PASS] submit with multi-instance pipeline_steps -> {multi_step_id[:8]}...")

        # verify structured pipeline_steps validation rejection
        code, body = http("POST", f"{base}/api/tasks", {
            "source_paths": [upload["path"]],
            "target_paths": [upload["path"]],
            "pipeline_steps": [
                {"step": "unsupported_processor_xyz"},
            ],
        })
        assert code == 400, f"expected 400 for unknown processor, got {code}"
        print("[PASS] invalid pipeline_step rejected with 400")

        # verify non-existent face detection rejected
        code, body = http("POST", f"{base}/api/faces", {
            "image_path": "non_existent_xyz_123.jpg",
        })
        assert code == 404, f"expected 404 for invalid face detect path, got {code}"
        print("[PASS] invalid face detection rejected with 404")

        # verify /api/preview serves uploaded image
        preview_req = urllib.request.Request(f"{base}/api/preview?path={upload['path']}", method="GET")
        with urllib.request.urlopen(preview_req, timeout=10) as r:
            assert r.status == 200, f"preview failed: {r.status}"
            content = r.read()
            assert len(content) > 0
        print("[PASS] /api/preview serves media successfully")

        # verify upload above 1MB (e.g. 2MB video/image target)
        large_body = b"X" * (2 * 1024 * 1024)
        req = urllib.request.Request(
            f"{base}/api/upload",
            data=large_body,
            headers={
                "Content-Type": "application/octet-stream",
                "X-File-Name": urllib.parse.quote("large_target.mp4"),
            },
            method="POST",
        )
        with urllib.request.urlopen(req, timeout=10) as r:
            assert r.status == 201
            up_large = json.loads(r.read().decode("utf-8"))
            assert up_large["size"] == 2 * 1024 * 1024
        print("[PASS] upload >1MB (2MB target) succeeded without 413")

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

